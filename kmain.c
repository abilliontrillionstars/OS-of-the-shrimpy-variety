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


void printClusterCallback(int errno, void* buffer, void* callback_data) 
{
    //kprintf("woah! callback worked?? errno=%d, buffer at %p, callback_data at %p.\n", errno, buffer, callback_data);
    kprintf("root directory data: \n");
    struct VBR* vbr = getVbr();
    for(int i=0; i<(vbr->bytes_per_sector); i++) //* vbr->sectors_per_cluster
        kprintf("%c", ((char*)buffer)[i]);
    
    kprintf("\nroot DirEntries: \n");
    struct DirEntry* dir = (struct DirEntry*)buffer;
    while(dir->base[0] || dir->attributes != 15)
    {
        for(int i=0; i<8; i++)
            if(dir->base[i] != ' ')
                kprintf("%c", dir->base[i]);
        console_putc('.');
        for(int i=0; i<3; i++)
            if(dir->base[i] != ' ')
                kprintf("%c", dir->ext[i]);
        kprintf("\t%d", dir->attributes);

        console_putc('\n');
        dir++;
    }        

    const char* string2 = "\n\nDONE\n";
    for(int i=0; string2[i]; i++) serial_putc(string2[i]);
}

void kmain2()
{
    const char* string = "\nSTART\n";    
    for(int i=0; string[i]; i++) serial_putc(string[i]);

    struct VBR* vbr = (struct VBR*) getVbr();
    disk_read_sectors(vbr->first_sector + vbr->reserved_sectors + (vbr->num_fats * vbr->sectors_per_fat), 1, printClusterCallback, NULL);
}

void kmain(struct MultibootInfo* mbi)
{
    kprintf("Everyone's programmed differently.\n");

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
    // grab disk metadata & defer the rest to kmain2
    disk_read_metadata(kmain2);
    // and stop there
    while(1) halt();
}