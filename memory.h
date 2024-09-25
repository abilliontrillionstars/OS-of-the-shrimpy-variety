#include "utils.h"

typedef struct _Header { u32 used, order, prev, next; } Header;

static char* heap = (char*) 0x10000;
#define HEAP_ORDER 19
Header* freeList[HEAP_ORDER+1];


void memory_init(struct MultibootInfo* info);

void* malloc(u32 size);

//many a helper function!
Header* getBuddy(Header* h);

void bisect(unsigned index);
void coalesce(unsigned index);

void addToFreeList(unsigned order, unsigned index);
Header* removeFromFreeList(unsigned order, unsigned index);
