#include "serial.h"
#include "console.h"
#include "utils.h"
#include "kprintf.h"
#include "font-default.h"
#include "memory.h"
#include "interrupt.h"
#include "disk.h"
#include "pci.h"
#include "filesys.h"

__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "push %ebx\n"
    "call _kmain"
);

void sweet();

static struct MultibootInfo bootInfo;

void kmain2()
{
    
}

void kmain(struct MultibootInfo* mbi)
{
    // set up the things
    kmemcpy(&bootInfo, mbi, sizeof(bootInfo));

    console_init(mbi);
    memory_init(mbi);
    serial_init();
    gdt_init();
    interrupt_init();
    timer_init();
    disk_init();
    
    interrupt_enable();
    disk_read_metadata(kmain2);
    // now do the things


    const char* string = "\nSTART\n";
    for(int i=0; string[i]; i++)
        serial_putc(string[i]);

    //kprintf("Everyone's programmed differently.\n");
    
    sweet();
    clusterNumberToSectorNumber(1);

    const char* string2 = "\nDONE\n";
    for(int i=0; string2[i]; i++)
        serial_putc(string2[i]);

    // and stop there
    while(1) __asm__("hlt");
}