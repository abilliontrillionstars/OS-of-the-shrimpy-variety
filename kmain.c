#include "serial.h"
#include "console.h"
#include "utils.h"
#include "kprintf.h"
#include "font-default.h"

void sweet();

__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "push %ebx\n"
    "call _kmain"
);

static struct MultibootInfo bootInfo;

void kmain(struct MultibootInfo* mbi)
{
    //set up the things
    kmemcpy(&bootInfo, mbi, sizeof(bootInfo));
    serial_init();
    console_init(mbi);

    // now do the things
    //clear_screen();
    kprintf("Everyone's programmed differently.\n");
    kprintf("this will be overwritten! @\x7f \rthat\e\e\e\n\n");

    //sweet();
    const char* string = "\nDONE\n";
    for(int i=0; string[i]; i++)
        serial_putc(string[i]);

    for(int i=0; i<400; i++)
        kprintf("test%d!", i);
        
    // and stop there
    while(1) __asm__("hlt");
}