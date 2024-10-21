#pragma once

#include "utils.h"

typedef struct _Header { u32 used, order, prev, next; } Header;

void memory_init(struct MultibootInfo* info);

void* kmalloc(u32 size);
void kfree(void* ptr);

//many a helper function!
void initHeader(Header* h, unsigned order);
Header* getBuddy(Header* h);
void bisect(unsigned order);

// that is, "sell" a chunk *back* to the freeList market
void addToFreeList(Header* h);
// that is, "buy" a chunk *from* the available real estate
Header* removeFromFreeList(unsigned order);

Header* getNext(Header* h);
void setNext(Header* h, Header* next);
Header* getPrev(Header* h);
void setPrev(Header* h, Header* prev);

void removeFirstNode(unsigned i);
void removeNode(Header* h);
void prependNode(Header* h);
