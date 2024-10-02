#include "serial.h"
#include "utils.h"
#define SERIAL_PORT 0x3f8
#define SERIAL_OK (0x3fd>>5)&1

void serial_init() {}

void serial_putc(char ch)
{
    if(SERIAL_OK)
        outb(SERIAL_PORT, ch);
}

