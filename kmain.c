
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
#include "sched.h"
#include "errno.h"

static struct MultibootInfo bootInfo;

void kmain3();

void kmain2() 
{
    static int countdown=3;
    spawn("A.EXE", kmain3, &countdown);
    spawn("B.EXE", kmain3, &countdown);
    spawn("C.EXE", kmain3, &countdown);
}

void kmain3(int errorcode, int pid, void* callback_data){
    if( errorcode != SUCCESS )
        panic("Error spawning task!");

    int* countdown = (int*)callback_data;
    *countdown -= 1;
    if( *countdown == 0 )
        sched_enable();
    return;
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
    paging_init();
    sched_init();
    disk_read_metadata(kmain2);    

    while(1)  { halt(); }
}
