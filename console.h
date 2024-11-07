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
// 233 255 47, 93 116 20
// 156 198 27, 70 81 22
static u16 foregroundColor = 0b1001111000100011;//0b0111011111100101;
static u16 backgroundColor = 0b0101101110100010;


void console_init(struct MultibootInfo* mbi);
void console_putc(char ch);

void clear_screen();
void set_pixel(unsigned x, unsigned y, u16 color);
void console_invert_pixel(unsigned x, unsigned y);

void draw_character(unsigned char ch, int x, int y);

void scroll();
