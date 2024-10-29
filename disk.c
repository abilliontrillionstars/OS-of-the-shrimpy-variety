#include "utils.h"
#include "disk.h"
#include "pci.h"
#include "kprintf.h"
#include "interrupt.h"
#include "errno.h"
#include "memory.h"
#include "filesys.h"

#define PCI_DISK_CLASS 1
#define PCI_DISK_SUBCLASS 1
static u32 portBase;        //First IO port to use
static u32 statusBase;      //IO port for getting status
static u32 interruptNumber;
static u32 dmaBase;

static struct Queue requestQueue;
static struct Request* currentRequest;

static struct PhysicalRegionDescriptor* physicalRegionDescriptor; 

void disk_init()
{
    u32 addr = pci_scan_for_device(PCI_DISK_CLASS, PCI_DISK_SUBCLASS);
    if(!addr) kprintf("No disk devices?\n");
    u32 tmp = pci_read_addr(addr,2);

    if(tmp & 0x100) getNativeResources(addr);
    else getLegacyResources();
    
    enable_busmaster(addr);
    register_interrupt_handler(interruptNumber+32, disk_interrupt);

    physicalRegionDescriptor = (struct PhysicalRegionDescriptor*) kmalloc(sizeof(struct PhysicalRegionDescriptor));
    unsigned seg1 = ((unsigned)(physicalRegionDescriptor))/65536;
    unsigned seg2 = ((unsigned)(physicalRegionDescriptor)+sizeof(physicalRegionDescriptor))/65536;
    
    if(seg1 != seg2) panic("Physical reion descriptor crosses 64KB boundary");
    if(seg1%4) panic("kmalloc gave address that is not multiple of 4");
}
void disk_interrupt(struct InterruptContext* ctx)  
{ 
    kprintf("HEY! I\"M DISKIN' HERE!!");

    int status = inb(dmaBase+2);
    int dmaError = status & 2;
    if(!(status&4))
        panic("No disk IRQ");

    //clear IRQ and error
    outb(dmaBase+2, (1<<2) | 2);

    struct Request* req = currentRequest;
    currentRequest=0;
    struct Request* nextReq = (struct Request*) queue_get(&requestQueue);
    if(nextReq) 
        dispatch_request(nextReq);

    if(!req) //BUG!
        panic("No current request?");
    else
    {
        if(dmaError)
            req->callback(EIO, NULL, req->callback_data);
        else
            req->callback(SUCCESS, req->buffer,  req->callback_data);
        kfree(req->buffer);
        kfree(req);
    }
}
void disk_read_sectors(unsigned firstSector, unsigned numSectors, disk_callback_t callback, void* callback_data)
{
    if(!callback)
        panic("BUG: disk_read_sectors with no callback\n");
    if(!numSectors || numSectors > 127)
    {
        callback(EINVAL, NULL, callback_data);
        return;
    }
    struct Request* req = (struct Request*) kmalloc(sizeof(struct Request));
    if(!req)
    {
        callback(ENOMEM, NULL, callback_data);
        return;
    }
    req->sector = firstSector;
    req->count = numSectors;
    req->callback = callback;
    req->callback_data = callback_data;
    if(currentRequest)
        queue_put(&requestQueue, req);
    else
        dispatch_request(req);
    
}

static void dispatch_request(struct Request* req)
{
    if(currentRequest)
        panic("BUG: Cannot dispatch when a request is outstanding\n");
    req->buffer = kmalloc(req->count*512);
    if(!req->buffer)
    {
        kfree(req);
        req->callback(ENOMEM, NULL, req->callback_data);

        //just because this request failed doesn't mean
        //we won't have more luck with the next one...
        struct Request* nextReq = (struct Request*) queue_get(&requestQueue);
        if(nextReq)
            dispatch_request(nextReq);
        return;
    }
    currentRequest = req;
    while(inb(CMDSTATUS)&BUSY) {}
    outb(dmaBase, 0); //disable DMA
    outb(dmaBase, 8); //8=read,0=write
    
    physicalRegionDescriptor->address = req->buffer;
    physicalRegionDescriptor->byteCount = req->count*512;
    physicalRegionDescriptor->flags = 0x8000; //this is the last PRD
    
    outl(dmaBase+4, (u32) physicalRegionDescriptor);
    outb(dmaBase+2, 4+2); //clear interrupt and error bits
    while(inb(CMDSTATUS) & BUSY) {}
    outb(SECTOR3SEL, 0xe0 | (req->sector>>24));
    outb(FLAGS,0);  //use interrupts
    outb(COUNT,req->count);
    outb(SECTOR0, req->sector & 0xff);
    outb(SECTOR1,(req->sector>>8)&0xff);
    outb(SECTOR2,(req->sector>>16)&0xff);
    outb(CMDSTATUS, COMMAND_READ_DMA);
    outb(dmaBase,9);  //start DMA: 9=read, 1=write

}

static void getNativeResources(u32 addr)
{
    portBase = pci_read_addr(addr,4) & ~0x3f;
    statusBase = (pci_read_addr(addr,5) & ~0x3f) + 2;
    interruptNumber = pci_read_addr(addr,15) & 0xff;
}
static void getLegacyResources()
{
    portBase = 0x1f0;
    statusBase = 0x3f6;
    interruptNumber = 14;
}

static void enable_busmaster(u32 addr)
{
    // set the bit flag thingy. turns on the bus master
    u32 tmp = pci_read_addr(addr,1);
    tmp |= 4;
    pci_write_addr(addr,1,tmp);
    dmaBase = pci_read_addr( addr,8 ) & ~0x3;
}

