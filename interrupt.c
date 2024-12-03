#include "utils.h"
#include "kprintf.h"
#include "interrupt.h"
#include "console.h"
#include "syscall.h"

asm (
    ".global _midlevel_handler\n"
    "_midlevel_handler:\n"
    "   push %ebp\n"
    "   push %edi\n"
    "   push %esi\n"
    "   push %edx\n"
    "   push %ecx\n"
    "   push %ebx\n"
    "   push %eax\n"
    "   push %esp\n"        //address of stack top = addr of eax
    "   cld\n"      //clear direction flag; C expects this
    "   call _highlevel_handler\n"
    "   addl $4, %esp\n"    //discard parameter for C
    "   pop %eax\n"
    "   pop %ebx\n"
    "   pop %ecx\n"
    "   pop %edx\n"
    "   pop %esi\n"
    "   pop %edi\n"
    "   pop %ebp\n"
    "   add $8,%esp\n"
    "   iret\n"
);

#pragma pack(push,1)
struct IDTEntry{
    u16 addrLow;     //address of lowlevel handler
    u16 selector;    //GDT entry
    u8 zero;         //must be zero
    u8 flags;        //0x8e for interrupt handler
    u16 addrHigh;    //address of lowlevel handler
};
#pragma pack(pop)


#pragma pack(push,1)
struct LIDT{
    u16 size;               //table size, bytes
    struct IDTEntry* addr;  //table start
};
#pragma pack(pop)


#define NUM_INTERRUPTS 49
static struct IDTEntry idt[NUM_INTERRUPTS];
extern void* lowlevel_addresses[];

#define MAX_HANDLERS 4
static InterruptHandler handlers[NUM_INTERRUPTS][MAX_HANDLERS];


void divideByZero(struct InterruptContext* ctx){
    kprintf("Divide by zero\n");
}

void illegalOpcode(struct InterruptContext* ctx){
    kprintf("Illegal opcode\n");
}

void generalFault(struct InterruptContext* ctx){
    kprintf("General fault\n");
}



void ackPic1(struct InterruptContext* ctx){
    outb( 0x20, 32 );
}
void ackPic2(struct InterruptContext* ctx){
    outb( 0x20, 32 );
    outb( 0xa0, 32 );
}

//demo code
//~ void timer0Handler(struct InterruptContext* ctx){
    //~ console_invert_pixel(400,300);
//~ }


void interrupt_init(){

    for(int i=0;i<NUM_INTERRUPTS;++i){
        u32 a = (u32)(lowlevel_addresses[i]);
        idt[i].addrLow = a&0xffff;
        idt[i].addrHigh = (a>>16);
        idt[i].selector = 8;    //code segment
        idt[i].zero = 0;
        idt[i].flags = 0x8e;
    }

    idt[48].flags = 0xee;   //1110 1110 = allow ring 3


    struct LIDT lidt;
    lidt.size = sizeof(idt);
    lidt.addr = idt;
    asm volatile("lidt (%%eax)" : : "a"(&lidt));

    register_interrupt_handler(0, divideByZero);
    register_interrupt_handler(6, illegalOpcode);
    register_interrupt_handler(13, generalFault);

    //set up primary pic
    outb(0x20,0x11);
    outb(0x21, 32);
    outb(0x21, 4);
    outb(0x21, 1);
    outb(0x21, 0);

    //set up secondary pic
    outb( 0xa0, 0x11 );
    outb( 0xa1, 40 );
    outb( 0xa1, 2 );
    outb( 0xa1, 1 );
    outb( 0xa1, 0 );


    for(int i=32;i<40;++i)
        register_interrupt_handler(i,ackPic1);
    for(int i=40;i<48;++i)
        register_interrupt_handler(i,ackPic2);

    register_interrupt_handler(48, syscall_handler);
    //for demo purposes
    //register_interrupt_handler(32, rtcHandler);


}


void interrupt_enable(){
    asm volatile ("sti");
}

void highlevel_handler(struct InterruptContext* ctx){
    int handled=0;
    unsigned interruptNumber = ctx->interruptNumber;
    for(int i=0;i<MAX_HANDLERS;++i){
        if( handlers[interruptNumber][i] ){
            handlers[interruptNumber][i](ctx);
            handled=1;
        }
    }
    if(!handled){
        kprintf("Warning: Unhandled interrupt: %d\n",interruptNumber);
    }
}

void register_interrupt_handler(unsigned interrupt,
                            InterruptHandler func){
    if( interrupt >= NUM_INTERRUPTS ){
        panic("Bad interrupt number\n");
    }
    for(int i=0;i<MAX_HANDLERS;++i){
        if( !handlers[interrupt][i] ){
            handlers[interrupt][i] = func;
            return;
        }
    }
    panic("Too many handlers!\n");
}
