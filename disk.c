#include "utils.h"
#include "disk.h"
#include "pci.h"

#define PCI_DISK_CLASS 1
#define PCI_DISK_SUBCLASS 1

void disk_init(){
    u32 addr = pci_scan_for_device( PCI_DISK_CLASS,
                                    PCI_DISK_SUBCLASS);
    if( addr == 0 ){
        //panic("No disk devices");
    }
    u32 tmp = pci_read_addr(addr,2);
    if( tmp & 0x100 )
        getNativeResources(addr);
    else
        getLegacyResources();
    enable_busmaster(addr);
    //register_interrupt_handler( interruptNumber+32,
    //                            disk_interrupt );
}

static u32 portBase;        //First IO port to use
static u32 statusBase;      //IO port for getting status
static u32 interruptNumber;

static void getNativeResources(u32 addr){
    portBase = pci_read_addr(addr,4) & ~0x3f;
    statusBase = (pci_read_addr(addr,5) & ~0x3f) + 2;
    interruptNumber = pci_read_addr(addr,15) & 0xff;
}

static void getLegacyResources(){
    portBase = 0x1f0;
    statusBase = 0x3f6;
    interruptNumber = 14;
}

static u32 dmaBase;
static void enable_busmaster(u32 addr){
    // set the bit flag thingy. turns on the bus master
    u32 tmp = pci_read_addr(addr,1);
    tmp |= 4;
    pci_write_addr(addr,1,tmp);
    dmaBase = pci_read_addr( addr,8 ) & ~0x3;
}

