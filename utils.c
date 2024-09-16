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


void kmemcpy(const void* start, void* dest, unsigned length)
{
    for(int i=0; i<length; i++)
        *((char*)dest+i) = *((char*)start+i);
}