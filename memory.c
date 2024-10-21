#include "utils.h"
#include "console.h"
#include "memory.h"
#include "kprintf.h"

static char* heap = (char*) 0x10000;
#define HEAP_ORDER 19
Header* freeList[HEAP_ORDER+1];

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

    for(unsigned i=0; i<HEAP_ORDER; ++i)
    {
        freeList[i] = NULL;
        freeList[HEAP_ORDER] = (Header*)heap;
        initHeader(freeList[HEAP_ORDER], HEAP_ORDER);
    }
}


void* kmalloc(u32 size)
{
    size = size + sizeof(Header);
    if(size > (1<<HEAP_ORDER))
        return NULL;

    //minimum size = 64 bytes = 2**6
    unsigned order = 6;
    while((1<<order) < size) order++;
    unsigned i = order;
    while(i<=HEAP_ORDER && freeList[i]==NULL) i++;

    if(i > HEAP_ORDER)
        return NULL; //not enough memory

    while(i>order)
    {
        bisect(i);
        i--;
    }

    Header* h = freeList[i];
    removeFirstNode(i);
    h->used=1;
    char* c = (char*) h;
    return c + sizeof(Header);
}

void kfree(void* v)
{
    char* c = (char*) v;
    Header* h = (Header*)(c-sizeof(Header));
    h->used = 0;
    prependNode(h);
    while(1)
    {
        if(h->order==HEAP_ORDER)
            return;     //entire heap is free
        Header* b = getBuddy(h);
        if(b->used == 0 && b->order == h->order)
        {
            removeNode(h);
            removeNode(b);
            if(b<h) h=b;
            h->order++;
            prependNode(h);
        }
        else
            return;
    }
}
// header-helper functions (why is it always hamburger help her why is it never hamburger help me)
static void initHeader(Header* h, unsigned order)
{
    h->used = 0;
    h->order = order;
    setPrev(h,NULL);
    setNext(h,NULL);
}
Header* getBuddy(Header* h)
{
    char* c = (char*) h;
    unsigned offset = c-heap;
    offset ^= (1<<h->order);
    c = heap + offset;
    return (Header*)c;
}

void bisect(unsigned index) 
{
    Header* h = freeList[index];
    removeFirstNode(index);
    h->order = h->order-1;
    char* c = (char*) h;
    c += (1<<h->order);
    Header* h2 = (Header*) c;
    initHeader(h2,h->order);
    prependNode(h);
    prependNode(h2);
}

void addToFreeList(Header* h) 
{

}
Header* removeFromFreeList(unsigned order)
{
    return (Header*) NULL;
}

Header* getNext(Header* h)
{
    unsigned delta = (h->next) << 6;
    Header* h2 = (Header*)(heap+delta);
    if( h == h2 )
        return NULL;
    return h2;
}
static void setNext(Header* h, Header* next)
{
    if(next== NULL)
        next = h;
    char* c = (char*) next;
    unsigned delta = c-heap;
    delta >>= 6;
    h->next = delta;
}
static Header* getPrev(Header* h)
{
    unsigned delta = (h->prev) << 6;
    Header* h2 = (Header*)(heap+delta);
    if( h == h2 )
        return NULL;
    return h2;
}
static void setPrev(Header* h, Header* prev)
{
    if( prev == NULL )
        prev = h;
    char* c = (char*) prev;
    unsigned delta = c-heap;
    delta >>= 6;
    h->prev = delta;
}

void removeFirstNode(unsigned i)
{
    Header* h = freeList[i];
    Header* n = getNext(h);
    if(n)
        setPrev(n,NULL);
    freeList[i] = n;
}
void removeNode(Header* h)
{
    Header* p = getPrev(h);
    Header* n = getNext(h);
    if(p)
        setNext(p,n);
    if(n)
        setPrev(n,p);

    //if we're removing the first node,
    //need to update head
    if(freeList[h->order] == h)
        freeList[h->order] = n;
}
void prependNode(Header* h)
{
    unsigned i = h->order;
    Header* n = freeList[i];
    setPrev(h, NULL);
    setNext(h, n);
    if(n)
        setPrev(n,h);
    freeList[i] = h;
}