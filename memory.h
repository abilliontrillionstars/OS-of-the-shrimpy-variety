#pragma once

#include "utils.h"

typedef struct _Header { u32 used, order, prev, next; } Header;

void memory_init(struct MultibootInfo* info);

void* kmalloc(u32 size);
void kfree(void* ptr);

//many a helper function!
Header* getBuddy(Header* h);
void bisect(unsigned order, unsigned index);
void coalesce(Header* h);
// that is, "sell" a chunk *back* to the freeList market
void addToFreeList(Header* h);
// that is, "buy" a chunk *from* the available real estate
Header* removeFromFreeList(unsigned order);
