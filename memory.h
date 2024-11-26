#pragma once
#include "utils.h"

#define PAGE_DEVICE_MEMORY ( (1<<3) | (1<<4) )
#define PAGE_PRESENT 1
#define PAGE_MUST_BE_ONE (1<<7)
#define PAGE_USER_ACCESS (1<<2)
#define PAGE_WRITEABLE (1<<1)

typedef struct _Header { u32 used, order, prev, next; } Header;

void memory_init(struct MultibootInfo* info);
void paging_init();

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
