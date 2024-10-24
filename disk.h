#pragma once
#include "utils.h"
#include "interrupt.h"
typedef void (*disk_callback_t)(int, void*, void*);

void disk_init();
void disk_interrupt(struct InterruptContext* ctx);
void getNativeResources(u32 addr);
void getLegacyResources();
void enable_busmaster(u32 addr);
