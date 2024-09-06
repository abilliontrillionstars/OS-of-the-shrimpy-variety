#pragma once
#pragma balls

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

void outb(u16 port, u8 value);
u8 inb(u16 port);