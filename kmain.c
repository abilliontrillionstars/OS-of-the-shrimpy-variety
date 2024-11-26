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
#include "file.h"

__asm__(".global _start\n""_start:\n""mov $0x10000,%esp\n""push %ebx\n""call _kmain");
void sweet();
static struct MultibootInfo bootInfo;


void printClusterCallback(int errno, void* buffer, void* callback_data)
{
    //kprintf("root directory data:  ");
    //struct VBR* vbr = getVbr();
    //for(int i=0; i<(vbr->bytes_per_sector* vbr->sectors_per_cluster); i++) //* vbr->sectors_per_cluster
    //    kprintf("%c", ((char*)buffer)[i]);
    
    kprintf("\nroot directory DirEntries: \n");
    struct DirEntry* dir = (struct DirEntry*)buffer;
    while(dir->base[0] || dir->attributes == 15)
    {
        if(dir->attributes != 15 && dir->base[0] != (char)0xe5) // regular DirEntry
        {
            for(int i=0; i<8; i++)
                if(dir->base[i] != ' ')
                    kprintf("%c", dir->base[i]);
            console_putc('.');
            for(int i=0; i<3; i++)
                if(dir->base[i] != ' ')
                    kprintf("%c", dir->ext[i]);
        }
        else // LFNEntry
        {
            struct LFNEntry* lfn = (struct LFNEntry*)dir;
            unsigned i=0;
            while((lfn + i)->attribute == 15) 
                i++;
            //kprintf("consecutive LFN's: %d\n", i);
            while(i)
            {
                i--;
                struct LFNEntry* entry = (lfn + i);
                // print the segment of the LFN
                for(int i=0; i<10; i+=2)
                    if(!entry->name0[i] || entry->name0[i] == -1)
                        break;
                    else 
                        kprintf("%c", entry->name0[i]);
                for(int i=0; i<12; i+=2)
                    if(!entry->name1[i] || entry->name1[i] == -1)
                        break;
                    else 
                        kprintf("%c", entry->name1[i]);
                for(int i=0; i<4; i+=2)
                    if(!entry->name2[i] || entry->name2[i] == -1)
                        break;
                    else
                        kprintf("%c", entry->name2[i]);
                // step over this entry so we're past them once we get out of the while loop
                // this also lets us see non-LFN entries instead of just ignoring them
                dir++;
            } 
            // now we should be on our base DirEntry
        }   
        // print the creation time
        kprintf("\tcreated on: ");
        u16 cd = dir->creationDate; // YYYY-MM-DD
        kprintf("%d-%02d-%02d ", (cd>>9)+1980, (cd>>5)&0xf, cd&0x1f);
        u16 ct = dir->creationTime; // HH:MM:SS
        //if(dir->creationTimeCentiseconds >= 100) ct++;
        kprintf("%0.2d:%0.2d:%0.2d", (ct>>11)&0x1f, (ct>>5)&0x2f, ct&0x1f);
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
    disk_read_sectors(vbr->first_sector + vbr->reserved_sectors + (vbr->num_fats * vbr->sectors_per_fat), vbr->sectors_per_cluster, printClusterCallback, NULL);
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