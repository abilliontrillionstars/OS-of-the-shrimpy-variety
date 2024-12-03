#pragma once

struct MultibootInfo;
void console_init(struct MultibootInfo* info);
void console_putc(char ch);
void console_invert_pixel(unsigned x, unsigned y);
