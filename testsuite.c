#include "kprintf.h"


void sweet(){

    static int TEST = 1;
    ///MAGIC GOES HERE
    
    if(TEST == 1){
        //division by zero
        int a = 4;
        int b = 0;
        int c = a/b;
        if( c == 100 )
            return;
    } 
    if(TEST == 2) {
        //undefined opcode
        asm volatile ("ud2");
    }
    if(TEST == 3){
        //general protection fault (#13)
        asm volatile ("mov $0,%eax\n"
            "mov %ax,%ss\n"
        );
    }
    
}
