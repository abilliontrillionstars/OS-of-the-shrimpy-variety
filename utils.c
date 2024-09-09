#include "utils.h"


void outb(u16 port, u8 value)
{
    __asm__ volatile("outb %%al,%%dx" : : "a"(value), "d"(port) );
}

u8 inb(u16 port)
{
    u32 tmp;
    __asm__ volatile("inb %%dx,%%al" : "=a"(tmp) : "d"(port) );
    return (u8)tmp;
}