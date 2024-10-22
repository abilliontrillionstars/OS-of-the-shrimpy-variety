#pragma once
#include "utils.h"
#include "interrupt.h"
typedef void (*disk_callback_t)(int, void*, void*);

void disk_init();
void disk_interrupt(struct InterruptContext* ctx);
void getNativeResources(u32 addr);
void getLegacyResources();
void enable_busmaster(u32 addr);

struct Queue { struct QueueNode* head; struct QueueNode* tail; };
struct QueueNode { struct QueueNode* next; void* data; };
u32 queue_put(struct Queue* q, void* data);
u32 queue_get(struct Queue* q);