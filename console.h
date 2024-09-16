#pragma once
#include "utils.h"

struct MultibootInfo;

void console_init(struct MultibootInfo* mbi);
void console_putc(char ch);

void clear_screen();
void set_pixel(unsigned x, unsigned y, u16 color);
