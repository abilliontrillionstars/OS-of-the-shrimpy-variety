
__asm__(
    ".global _start\n"
    "_start:\n"
    "mov $0x10000,%esp\n"
    "push %ebx\n"
    "call _kmain"
);

#include "serial.h"
#include "kprintf.h"
#include "utils.h"
#include "console.h"
#include "gdt.h"
#include "interrupt.h"
#include "timer.h"
#include "disk.h"
#include "memory.h"
#include "exec.h"

static struct MultibootInfo bootInfo;


void kmain2(){
    exec("HELLO.EXE", 0x400000, exec_transfer_control, 0);
}

void kmain(struct MultibootInfo* mbi){
    kmemcpy(&bootInfo, mbi, sizeof(bootInfo) );
    serial_init();
    console_init(mbi);
    memory_init();
    gdt_init();
    interrupt_init();
    timer_init();
    disk_init();
    interrupt_enable();
    disk_read_metadata(kmain2);
    while(1)  { halt(); }
}
