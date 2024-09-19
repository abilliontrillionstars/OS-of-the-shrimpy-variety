#include "kprintf.h"

const char* bigstring =
    "When in the course of human. No.\f"
    "We the people of. Oh crud.\f"
    "|       |       |       |       |       |       |       |       |       |       |       |       |\n"
    "Fourscore and\t^seven years ago, our\t^forefathers brought\t^forth upon\t^this continent\n"
    "a new \t^NATION conceived in libibertarian\x7f\x7f\x7f\x7f\x7fy and\t\t\t^dedicated to\t\t^the"
    "\t\t^NOTION\t^that\n"
    "\n"
    "all men\x7f\x7f\x7fpeople are created equal.\n"
    "\n"
    "\n"
    "Now\n"
    "\twe\n\t\tare\n\t\t\tengaged.................,..............in..............,.............a............,..........large,"
    "yooge, egregious, massive, overbearing grate\x7f\x7f\x7f""eat civil strife,\n"
    "~~~~~~~\t:-)\n"
    "\t\t\t\t\ttesting whether that "
    "nation or fooby"
    "ANY\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f" "any "
    "nation can so endure."
    ;


void sweet(){
    int i;
    const char* p = bigstring;
    while(*p){
        kprintf("%c",*p++);
    }
    kprintf("\n");
    for(i=0;i<201;++i)
        kprintf("$");
    for(i=0;i<200;++i)
        kprintf("\x7f");
    kprintf("\n");
    kprintf("A");
    kprintf("B");
    kprintf("\x7f");
    kprintf("C");
    kprintf("D");
    kprintf("\x7f");
    kprintf("E");
    kprintf("F");
    kprintf("\n");
    for(i=0;i<20;i++)
        kprintf("*");
    kprintf("\r");
    kprintf("!");
    kprintf("#");
    kprintf("\x7f");
}
