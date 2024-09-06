

__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "call _kmain"
);

#include "serial.c"

void kmain(){
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