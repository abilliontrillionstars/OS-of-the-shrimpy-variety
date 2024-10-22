#include "utils.h"


void outb(u16 port, u8 value) { __asm__ volatile("outb %%al,%%dx" : : "a"(value), "d"(port)); }
u8 inb(u16 port)
{
    u32 tmp;
    __asm__ volatile("inb %%dx,%%al" : "=a"(tmp) : "d"(port) );
    return (u8)tmp;
}

void kmemcpy(void* dest, const void* start, unsigned length)
{
    for(int i=0; i<length; i++)
        ((char*)dest)[i] = ((char*)start)[i];
}

u16 inw(u16 port)
{
    u32 tmp;
    asm volatile ("inw %%dx,%%ax"
        : "=a"(tmp) //one output
        : "d"(port) //one input
    );
    return (u16)tmp;
}

void outw(u16 port, u16 value){
    asm volatile("outw %%ax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}

u32 inl(u16 port){
    u32 tmp;
    asm volatile ("inl %%dx,%%eax"
        : "=a"(tmp)         //one output
        : "d"(port)         //one input
    );
    return tmp;
}

void outl(u16 port, u32 value){
    asm volatile("outl %%eax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}


void halt()  { asm("hlt"); }