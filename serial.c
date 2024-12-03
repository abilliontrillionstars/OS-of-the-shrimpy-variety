#include  "utils.h"

#define SERIAL_IO 0x3f8
#define SERIAL_FLAGS 0x3fd
#define SERIAL_READY (1<<5)

void serial_init(){
}

void serial_putc(char c){
    while( 1 ){
        int f = inb(SERIAL_FLAGS);
        if( f & SERIAL_READY)
            break;
    }
    outb(SERIAL_IO, c);
}
