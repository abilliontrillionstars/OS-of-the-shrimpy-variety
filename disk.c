#include "disk.h"
#include "utils.h"
#include "interrupt.h"
#include "errno.h"
#include "pci.h"
#include "memory.h"
#include "kprintf.h"

u32 fat[ MAX_DISK_SIZE_MB*1024*1024/4096 ];

#define PCI_DISK_CLASS 1
#define PCI_DISK_SUBCLASS 1


#define BUSY                0x80
#define CONTROLLER_ERROR    0x01
#define DISK_ERROR          0x20
#define DISK_READY          0x08
#define DATA                (portBase)
#define ERROR               (portBase)+1
#define COUNT               (portBase)+2
#define SECTOR0             (portBase)+3
#define SECTOR1             (portBase)+4
#define SECTOR2             (portBase)+5
#define SECTOR3SEL          (portBase)+6
#define CMDSTATUS           (portBase)+7
#define FLAGS               (statusBase)
#define COMMAND_IDENTIFY    (0xec)
#define COMMAND_READ_DMA    (0xc8)
#define COMMAND_WRITE_DMA   (0xca)
#define COMMAND_FLUSH       (0xe7)

struct Request{
    unsigned sector;
    unsigned count;
    disk_callback_t callback;
    void* callback_data;
    char* buffer;
};

#pragma pack(push,1)
struct PhysicalRegionDescriptor{
    void* address;
    u16 byteCount;
    u16 flags;       //0x8000=last one, 0x0000=not last
};
#pragma pack(pop)


#pragma pack(push,1)
struct GUID {
    u32 time_low;
    u16 time_mid;
    u16 time_high_version;
    u16 clock;
    u8 node[6];
};
struct GPTEntry{
    struct GUID type;
    struct GUID guid;
    u32 firstSector;
    u32 firstSector64;
    u32 lastSector;
    u32 lastSector64;
    u32 attributes;
    u32 attributes64;
    u16 name[36];
};
#pragma pack(pop)


#pragma pack(push,1)
struct VBR{
    char jmp[3];
    char oem[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 num_fats;
    u16 UNUSED_num_root_dir_entries;
    u16 UNUSED_num_sectors_small;
    u8 id ;
    u16 UNUSED_sectors_per_fat_12_16;
    u16 sectors_per_track;
    u16 num_heads;
    u32 first_sector;
    u32 num_sectors;
    u32 sectors_per_fat;
    u16 flags;
    u16 version;
    u32 root_cluster;
    u16 fsinfo_sector;
    u16 backup_boot_sector;
    char reservedField[12];
    u8 drive_number;
    u8 flags2;
    u8 signature;
    u32 serial_number;
    char label[11];
    char identifier[8];
    char code[420];
    u16 checksum;
};
#pragma pack(pop)


static u32 portBase;        //First IO port to use
static u32 statusBase;      //IO port for getting status
static u32 interruptNumber;

static struct Queue requestQueue;
static struct Request* currentRequest;
static struct PhysicalRegionDescriptor* physicalRegionDescriptor;

static struct VBR vbr;

static void dispatch_request(struct Request* req);

unsigned clusterNumberToSectorNumber( unsigned clnum )
{
    if( clnum < 2 || clnum > 0xffffff )
        panic("Bad cluster number");

    unsigned dataArea = vbr.first_sector + vbr.reserved_sectors + vbr.num_fats*vbr.sectors_per_fat;
    return dataArea + (clnum-2) * vbr.sectors_per_cluster;
}

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
    u32 tmp = pci_read_addr(addr,1);
    tmp |= 4;
    pci_write_addr(addr,1,tmp);
    dmaBase = pci_read_addr( addr,8 ) & ~0x3;
}

void disk_interrupt(struct InterruptContext* ctx)
{
    int status = inb(dmaBase+2);
    int dmaError = status & 2;
    if( 0 == ( status & 4 ) ){
        panic("No disk IRQ");
    }

    //clear IRQ and error
    outb( dmaBase+2, (1<<2) | 2 );

    struct Request* req = currentRequest;
    currentRequest=0;
    struct Request* nextReq = (struct Request*) queue_get(
                                        &requestQueue
    );
    if( nextReq ){
        dispatch_request(nextReq);
    }

    if( req == 0 ){
        //BUG!
        panic("No current request?");
    } else {
        if( dmaError ){
            req->callback(EIO, NULL, req->callback_data);
        } else {
            req->callback(SUCCESS, req->buffer,  req->callback_data);
        }
        kfree(req->buffer);
        kfree(req);
    }

}


void disk_init(){

    physicalRegionDescriptor = (struct PhysicalRegionDescriptor*) kmalloc(
                            sizeof(struct PhysicalRegionDescriptor));
    unsigned seg1 = ((unsigned)(physicalRegionDescriptor))/65536;
    unsigned seg2 = ((unsigned)(physicalRegionDescriptor)+sizeof(physicalRegionDescriptor))/65536;
    if( seg1 != seg2 )
        panic("Physical region descriptor crosses 64KB boundary");
    if( seg1 % 4 )
        panic("kmalloc gave address that is not multiple of 4");


    u32 addr = pci_scan_for_device( PCI_DISK_CLASS,
                                    PCI_DISK_SUBCLASS);
    if( addr == 0 ){
        panic("No disk devices");
    }
    u32 tmp = pci_read_addr(addr,2);
    if( tmp & 0x100 ){
        //native IDE
        getNativeResources(addr);
    } else {
        //legacy IDE
        getLegacyResources();
    }
    enable_busmaster(addr);
    register_interrupt_handler( interruptNumber+32,
                                disk_interrupt );
}

static void dispatch_request(struct Request* req){
    if( currentRequest != 0 ){
        panic("BUG: Cannot dispatch when a request is outstanding\n");
    }
    req->buffer = kmalloc( req->count*512 ) ;
    if(!req->buffer){

        disk_callback_t callback = req->callback;
        void* data = req->callback_data;
        kfree(req);
        callback( ENOMEM, NULL, data );

        //~ kfree(req);
        //~ req->callback(ENOMEM, NULL, req->callback_data);

        //see if we can dispatch the next request, if there is one
        struct Request* nextReq = (struct Request*) queue_get(
                &requestQueue
        );
        if( nextReq ){
            dispatch_request(nextReq);
        }
        return;
    }

    currentRequest=req;
    while( inb(CMDSTATUS) & BUSY ){
    }
    outb(dmaBase,0);    //disable DMA
    outb(dmaBase,8);    //8=read,0=write

    physicalRegionDescriptor->address = req->buffer;
    physicalRegionDescriptor->byteCount = req->count*512;
    physicalRegionDescriptor->flags = 0x8000;   //this is the last PRD

    outl( dmaBase+4, (u32) physicalRegionDescriptor );
    outb( dmaBase+2, 4+2);  //clear interrupt and error bits

    while( inb(CMDSTATUS) & BUSY ){
    }

    outb(SECTOR3SEL, 0xe0 | (req->sector>>24) );

    outb(FLAGS,0);  //use interrupts
    outb(COUNT,req->count);
    outb(SECTOR0, req->sector & 0xff);
    outb(SECTOR1,(req->sector>>8)&0xff);
    outb(SECTOR2,(req->sector>>16)&0xff);
    outb(CMDSTATUS, COMMAND_READ_DMA );
    outb(dmaBase,9);  //start DMA: 9=read, 1=write

}


void disk_read_sectors(
                unsigned firstSector,
                unsigned numSectors,
                disk_callback_t callback,
                void* callback_data)
{
    if( !callback ){
        panic("BUG: disk_read_sectors with no callback\n");
    }
    if( numSectors == 0 || numSectors > 127 ){
        callback( EINVAL, NULL, callback_data );
        return;
    }
    struct Request* req = (struct Request*) kmalloc(
                sizeof(struct Request)
    );
    if( !req ){
        callback(ENOMEM, NULL, callback_data);
        return;
    }
    req->sector = firstSector;
    req->count = numSectors;
    req->callback = callback;
    req->callback_data = callback_data;
    if( currentRequest != NULL ){
        queue_put(&requestQueue, req );
    } else {
        dispatch_request(req);
    }
}


static int fatSectorsRemaining;
static disk_metadata_callback_t kmain_callback;


void read_fat_callback(int errorcode, void* data, void* p){
    if( errorcode != SUCCESS){
        panic("Cannot read FAT");
    }
    u32 i = (u32) p;
    //each sector has 128 FAT entries in it
    //and is 512 bytes in size
    kmemcpy( fat + 128*i, data, 512 );
    --fatSectorsRemaining;
    if( fatSectorsRemaining == 0 )
        kmain_callback();
}


static void read_fat(disk_metadata_callback_t f){
    kmain_callback=f;
    fatSectorsRemaining = vbr.sectors_per_fat;
    unsigned fatStart = vbr.first_sector + vbr.reserved_sectors;

    //this fires off a bunch of overlapped reads
    for(u32 i=0;i<vbr.sectors_per_fat;++i){
        disk_read_sectors(
            fatStart+i,
            1,
            read_fat_callback,
            (void*)i
        );
    }
}


static void read_vbr_callback( int errorcode,
                               void* sectorData,
                               void* kmain_callback
){
    if( errorcode != SUCCESS ){
        kprintf("Cannot read VBR: %d\n",errorcode);
        panic("Cannot continue");
        return;
    }
    kmemcpy(&vbr,sectorData,sizeof(vbr));
    disk_metadata_callback_t f = (disk_metadata_callback_t) kmain_callback;
    read_fat(f);
}

static void read_partition_table_callback(int errorcode,
                                              void* sectorData,
                                              void* kmain_callback
){
    if( errorcode != SUCCESS ){
        kprintf("Cannot read partition table: %d\n",errorcode);
        panic("Cannot continue");
    }

    struct GPTEntry* entry = (struct GPTEntry*) sectorData;
    disk_read_sectors( entry->firstSector, 1, read_vbr_callback,
                       kmain_callback );

}

void disk_read_metadata( disk_metadata_callback_t kmain_callback ){
    disk_read_sectors( 2, 1, read_partition_table_callback,
                       kmain_callback );
}


unsigned sectorsPerCluster(){
    return vbr.sectors_per_cluster;
}
