#pragma once
#include "utils.h"

void disk_init();

static void getNativeResources(u32 addr);
static void getLegacyResources();

static void enable_busmaster(u32 addr);

struct Queue 
{ 
    struct QueueNode* head; 
    struct QueueNode* tail; 
};
struct QueueNode
{
    struct QueueNode* next;
    void* data;
};

u32 queue_put(struct Queue* q, void* data);
u32 queue_get(struct Queue* q);