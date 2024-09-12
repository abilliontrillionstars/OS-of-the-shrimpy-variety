#include "serial.h"
#include "console.h"
#include "utils.h"
#include "kprintf.h"

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
    
    kprintf("\nWe the People of the United States\n");

    while(1){
        __asm__("hlt");
    }
}