#include "utils.h"
#include "interrupt.h"
#include "kprintf.h"
#define NUM_INTERRUPTS 49
typedef void (*InterruptHandler)(struct InterruptContext* ctx);

extern void* lowlevel_addresses[];
//global variable
static struct GDTEntry gdt[3]; 
static struct IDTEntry idt[NUM_INTERRUPTS];


void highlevel_handler(struct InterruptContext* ctx)
{ 
    switch(ctx->interruptNumber)
    {
        case 0:
            kprintf("Divide by zero"); break;
        case 6:
            kprintf("Undefined opcode"); break;
        case 13:
            kprintf("General fault"); break; // general fault! o7
        default:
            kprintf("waugh-errno-%d", ctx->errorcode); break;
    }
    while(1) asm("hlt");
}

asm ( // this is a declaration of the midlevel handler in asm
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


void interrupt_init()
{
    struct LIDT lidt;
    lidt.size = sizeof(idt);
    lidt.addr = idt;
    asm volatile("lidt (%%eax)" : : "a"(&lidt));

    for (int i=0; i<NUM_INTERRUPTS; i++)
    {
        u32 a = (u32)(lowlevel_addresses[i]);
        idt[i].addrLow = (u16)((a<<16)>>16);
        idt[i].addrHigh = (u16)(a>>16);
        idt[i].selector = 8;
        idt[i].zero = 0;
        idt[i].flags = 0x8e;
    }
}

void gdt_init()
{
    gdt[0].limitBits0to15 = 0;
    gdt[0].baseBits0to15 = 0;
    gdt[0].baseBits16to23 = 0;
    gdt[0].flags = 0;
    gdt[0].flagsAndLimitBits16to19 = 0;
    gdt[0].baseBits24to31 = 0;

    gdt[1].baseBits0to15 = 0;
    gdt[1].baseBits16to23 = 0;
    gdt[1].baseBits24to31 = 0;
    gdt[1].limitBits0to15 = 0xffff;
    gdt[1].flags = SEGMENT_TYPE_CODE |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_0 |
                   SEGMENT_PRESENT ;
    gdt[1].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;
    
    gdt[2].baseBits0to15=0;
    gdt[2].baseBits16to23=0;
    gdt[2].baseBits24to31=0;
    gdt[2].limitBits0to15=0xffff;
    gdt[2].flags = SEGMENT_TYPE_DATA |
                   SEGMENT_IS_CODE_OR_DATA |
                   SEGMENT_RING_0 |
                   SEGMENT_PRESENT ;
    gdt[2].flagsAndLimitBits16to19 =
                    SEGMENT_LIMIT_HIGH_NIBBLE |
                    SEGMENT_LIMIT_32_BIT |
                    SEGMENT_32_BIT;
    

    struct LGDT lgdt; // alphabet mafia has infiltrated your machine
    lgdt.size = sizeof(gdt);
    lgdt.addr = &gdt[0];
    void* tmp = &lgdt;
    asm volatile (
         "lgdt (%%eax)\n"       //load gdt register
         "mov $16,%%eax\n"      //set eax to 16: Offset to gdt[2]
         "mov %%eax,%%ds\n"     //store 16 to ds
         "mov %%eax,%%es\n"     //store 16 to es
         "mov %%eax,%%fs\n"     //store 16 to fs
         "mov %%eax,%%gs\n"     //store 16 to gs
         "mov %%eax,%%ss\n"     //store 16 to ss
         "jmpl $0x8,$reset_cs\n"    //Intel says we must do a jmp here
         "reset_cs:\n"
         "nop\n"                //no operation
         : "+a"(tmp)
         :
         : "memory"
    );
}