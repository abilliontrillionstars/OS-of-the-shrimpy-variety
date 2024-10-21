#include "serial.h"
#include "console.h"
#include "utils.h"
#include "kprintf.h"
#include "font-default.h"
#include "memory.h"
#include "interrupt.h"


__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "push %ebx\n"
    "call _kmain"
);

void sweet();

static struct MultibootInfo bootInfo;

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

    interrupt_enable();
    // now do the things
    //kprintf("Everyone's programmed differently.\n");
    //sweet();

    const char* string = "\nDONE\n";
    for(int i=0; string[i]; i++)
        serial_putc(string[i]);

    // and stop there
    while(1) __asm__("hlt");
}