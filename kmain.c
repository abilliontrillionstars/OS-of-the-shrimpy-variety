#include "serial.h"
#include "console.h"
#include "utils.h"
#include "kprintf.h"

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
    kmemcpy(&bootInfo, mbi, sizeof(bootInfo));
    serial_init();
    console_init(mbi);

    clear_screen();
    kprintf("Everyone's programmed differently.\n");
    for(int x=0; x<200; x++)
        for(int y=0; y<150; y++)
            set_pixel(x, y, foregroundColor);

    while(1) __asm__("hlt");
}