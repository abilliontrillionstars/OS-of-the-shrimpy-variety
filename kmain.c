#include "serial.c"

__asm__
(
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
    serial_putc('H');
    serial_putc('e');
    serial_putc('l');
    serial_putc('l');
    serial_putc('o');
    serial_putc('\n');
    while(1){
        __asm__("hlt");
    }
}