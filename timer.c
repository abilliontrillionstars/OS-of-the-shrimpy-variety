#include "interrupt.h"
#include "timer.h"
#include "utils.h"
#include "kprintf.h"
#include "sched.h"

static volatile unsigned jiffies=0;

void rtcHandler(struct InterruptContext* ctx){
    //ack interrupt
    outb( 0x70, 0xc);
    inb( 0x71 );
    jiffies++;
    
    schedule(ctx);
    sched_check_wakeup();
}

unsigned get_uptime(){
    //16 interrupts per second
    return (jiffies * 1000)/16;
}

void timer_init()
{
    //set up rtc rate
    outb( 0x70, 0xa );
    unsigned tmp = inb(0x71);
    outb( 0x70, 0xa );
    outb( 0x71, 12 | (tmp & 0xf0) );    //12=16 interrupts per second

    //tell rtc to generate interrupts
    outb(0x70,11);
    unsigned sv=inb(0x71);
    outb(0x70,11);
    outb( 0x71, sv | 0x40);

    register_interrupt_handler(40, rtcHandler);
}
