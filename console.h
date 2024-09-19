#pragma once
#include "utils.h"

#define SCREEN_HEIGHT 600
#define SCREEN_WIDTH 800


struct MultibootInfo;

static int escCharBuf;
static volatile u8* framebuffer;
static unsigned pitch; 
//PITCH is the amount of bytes needed to get to the next row
static unsigned width;
static unsigned height;
static u16 foregroundColor = 0b1010110101010101;
static u16 backgroundColor = 0b011111;


void console_init(struct MultibootInfo* mbi);
void console_putc(char ch);

void clear_screen();
void set_pixel(unsigned x, unsigned y, u16 color);
void draw_character(unsigned char ch, int x, int y);

void scroll();
