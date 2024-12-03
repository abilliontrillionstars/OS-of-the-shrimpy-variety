#pragma once

#include "utils.h"

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

typedef void (*InterruptHandler)(struct InterruptContext* ctx);

void interrupt_init();
void register_interrupt_handler(unsigned interrupt,
                            InterruptHandler func);
void interrupt_enable();
