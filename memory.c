#include "utils.h"
#include "console.h"
#include "memory.h"
#include "kprintf.h"

void memory_init(struct MultibootInfo* info) 
{
    //number of regions
    u32 nr = info->map.length / sizeof(struct MB_MemInfo);
    kprintf("Num regions: %d\n", nr);

    struct MB_MemInfo* M = info->map.addr;

    for(int i=0; i<nr; ++i)
    {
        u32 end = M[i].addr + M[i].length;
        kprintf("Region %d: addr=0x%08x...0x%08x length%dKB type=%s\n",
            i, M[i].addr, end-1, M[i].length/1024, (M[i].type==1) ? "RAM":"Reserved");
    }
}

static void initHeader(Header* h, unsigned order)
{
    h->used = 0;
    h->order = order;
    h->prev = NULL;
    h->next = NULL; 
}

Header* getBuddy(Header* h)
{
    u32 delta = ((char*)h - heap);
    //flip the bit at position h->order
    delta = delta ^ 1<<h->order;
    return (Header*)(delta+heap);
}

void* kmalloc(u32 size)
{
    size = size+4;
    unsigned o = 6; //min is 64KB
    int i = o;
    
    //bring order up to the actual order that we need for the given size
    while((i<<o) < size) o++; 
    //scan through freeList (which is a Header array) until we find the right order
    while(i <= HEAP_ORDER && !freeList[i]) i++;
    if(i > HEAP_ORDER) return NULL;

    while(i>o)
    {
        //this is where we split our memory blocks
        bisect(i);
        i--;
    }
    //now we have an entry in freeList of our desired order,

    //TODO <--
    //Header* h = removeFromFreeList(o);
    //h->used = 0;
    return ((char*)h + sizeof(Header));
}

void bisect(unsigned index) {}
void coalesce(unsigned index) {}