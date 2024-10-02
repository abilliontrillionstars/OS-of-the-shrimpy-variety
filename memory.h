#pragma once

#include "utils.h"

typedef struct _Header { u32 used, order, prev, next; } Header;

void memory_init(struct MultibootInfo* info);

void* kmalloc(u32 size);

//many a helper function!
Header* getBuddy(Header* h);

void bisect(unsigned index);
void coalesce(unsigned index);

void addToFreeList(unsigned order, unsigned index);
Header* removeFromFreeList(unsigned index);
