#include "utils.h"

struct GDT 
{

};

struct LGDT //slay
{

};

struct InterruptContext{
    //we pushed these in midlevel handler
    u32 eax, ebx, ecx, edx, esi, edi, ebp;
    //we pushed this in lowlevel handler
    u32 interruptNumber;
    //cpu pushed these (except maybe errorcode)
    u32 errorcode, eip, cs, eflags;
    //cpu pushed these if there was a ring transition
    u32 esp, ss;
};

void highlevel_handler(struct InterruptContext* ctx)
{
    
}

void interrupt_init()
{
    //gdt_init();
}