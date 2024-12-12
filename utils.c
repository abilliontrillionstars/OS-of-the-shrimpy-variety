#include "utils.h"
#include "kprintf.h"
#include "errno.h"
#include "memory.h"

void outb(u16 port, u8 value){
    __asm__ volatile("outb %%al,%%dx" : : "a"(value), "d"(port) );
}

u8 inb(u16 port){
    u32 tmp;
    __asm__ volatile("inb %%dx,%%al" : "=a"(tmp) : "d"(port) );
    return (u8)tmp;
}



u16 inw(u16 port){
    u32 tmp;
    asm volatile ("inw %%dx,%%ax"
        : "=a"(tmp)         //one output
        : "d"(port)         //one input
    );
    return (u16)tmp;
}

void outw(u16 port, u16 value){
    asm volatile("outw %%ax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}

u32 inl(u16 port){
    u32 tmp;
    asm volatile ("inl %%dx,%%eax"
        : "=a"(tmp)         //one output
        : "d"(port)         //one input
    );
    return tmp;
}

void outl(u16 port, u32 value){
    asm volatile("outl %%eax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}

void kmemcpy(void* dest, void* src, unsigned n)
{
    char* d = (char*) dest;
    char* s = (char*) src;
    while(n--){
        *d++=*s++;
    }
}

 __asm__(    ".global _panic\n"
                "_panic:\n"
                "mov (%esp),%eax\n"     //eip -> eax
                "mov 4(%esp),%edx\n"    //string parameter
                "push %edx\n"
                "push %eax\n"
                "call _panic2\n"
);

void panic2(void* eip, const char* msg){
    kprintf("Kernel panic: At eip=%p: %s\n",
        eip,msg);
    while(1){
        __asm__("hlt");
    }
}

void halt(){
    __asm__ volatile("hlt");
}

int queue_put(struct Queue* Q, void* data){
    struct QueueNode* n = (struct QueueNode*) kmalloc(sizeof(struct QueueNode));
    if(!n){
        return ENOMEM;
    }
    n->next = NULL;
    n->item = data;
    if( Q->head == NULL ){
        Q->head = Q->tail = n;
    } else {
        Q->tail->next = n;
        Q->tail = n;
    }
    return SUCCESS;
}

void* queue_get(struct Queue* Q){
    if( Q->head == NULL )
        return NULL;
    struct QueueNode* n = Q->head;
    Q->head = Q->head->next;
    if( Q->head == NULL ){
        Q->tail = NULL;
    }
    void* p = n->item;
    kfree(n);
    return p;
}

int kmemcmp(void* p1, void* p2, unsigned count)
{
    char* c1 = (char*)p1;
    char* c2 = (char*)p2;
    while(count > 0 ){
        if( *c1 < *c2 )
            return -1;
        if( *c1 > *c2 )
            return 1;
        ++c1;
        ++c2;
        --count;
    }
    return 0;
}

void kmemset(void* dest, int value, unsigned count) {
    for(int i=0; i<count; i++)
        ((char*)dest)[i] = value;
}

void kstrcpy(char* dest, const char* src)
{
    while( (*dest++ = *src++) ){
    }
}

unsigned kstrlen(const char* s)
{
    unsigned c=0;
    while(*s++)
        c++;
    return c;
}

char toupper(char c)
{
    if( c >= 'a' && c <= 'z' )
        return c - ('a'-'A');
    else
        return c;
}

unsigned min(unsigned a, unsigned b){
    if(a<b)
        return a;
    else
        return b;
}
