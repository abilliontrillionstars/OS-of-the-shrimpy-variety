#pragma once
#include "utils.h"
#include "interrupt.h"

#define BUSY 0x80
#define CONTROLLER_ERROR 0x01
#define DISK_ERROR 0x20
#define DISK_READY 0x08
#define DATA (portBase)
#define ERROR (portBase)+1
#define COUNT (portBase)+2
#define SECTOR0 (portBase)+3
#define SECTOR1 (portBase)+4
#define SECTOR2 (portBase)+5
#define SECTOR3SEL (portBase)+6
#define CMDSTATUS (portBase)+7
#define FLAGS (statusBase)
#define COMMAND_IDENTIFY (0xec)
#define COMMAND_READ_DMA (0xc8)
#define COMMAND_WRITE_DMA (0xca)
#define COMMAND_FLUSH (0xe7)

typedef void (*disk_callback_t)(int, void*, void*);
struct Request
{
    unsigned sector;
    unsigned count;
    disk_callback_t callback;
    void* callback_data;
    char* buffer;
};

#pragma pack(push,1)
struct PhysicalRegionDescriptor
{
    void* address;
    u16 byteCount;
    u16 flags; //0x8000=last one, 0x0000=not last
};
#pragma pack(pop)

void disk_init();
void disk_interrupt(struct InterruptContext* ctx);
void disk_read_sectors(unsigned firstSector, unsigned numSectors, 
                    disk_callback_t callback, void* callback_data);
void dispatch_request(struct Request* req);

void getNativeResources(u32 addr);
void getLegacyResources();

void enable_busmaster(u32 addr);
