#include "serial.h"
#include "console.h"
#include "utils.h"
#include "kprintf.h"
#include "font-default.h"

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

    //now do the things
    clear_screen();
    kprintf("Everyone's programmed differently.\n");

    const char* string = "We the People of the United States";
    for(int i=0; string[i]; i++)
        draw_character(string[i], 100+(i*CHAR_WIDTH), 200);

    //and stop there
    while(1) __asm__("hlt");
}