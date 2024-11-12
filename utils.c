#include "utils.h"
#include "memory.h"
#include "errno.h"

// first in, first out. put onto the tail, get from the head.
// NOTE: may need to kmalloc the pointer to the data too. if stuff breaks that may be it (totally not because I don't remember Data Structures)
int queue_put(struct Queue* q, void* data)
{
    // empty queue. make the head and set up other pointers
    if (!q->head && !q->tail)
    {
        // make our head
        q->head = (struct QueueNode*) kmalloc(sizeof(struct QueueNode));
        if(!q->head) return ENOMEM;
        // point the tail to it (for now)
        q->tail = q->head;
        // add the data to the head (this may need kmalloc'd)
        q->head->data = data;
    }
    // one item in the queue. make the tail and point to it
    else if(q->head == q->tail)
    {
        // make our tail
        q->tail = (struct QueueNode*) kmalloc(sizeof(struct QueueNode));
        if(!q->tail) return ENOMEM;
        // point the head's *next* to it
        q->head->next = q->tail;
        //set the data
        q->tail->data = data;
    }
    // queue has stuff. add onto the tail, then set the new tail
    else
    {
        // make the new node
        q->tail->next = (struct QueueNode*) kmalloc(sizeof(struct QueueNode));
        if(!q->tail->next) return ENOMEM;
        // point the tail to it
        q->tail = q->tail->next;
        // set the data
        q->tail->next->data = data;
    }
    return SUCCESS;
}
void* queue_get(struct Queue* q) 
{
    // empty queue. do nothing
    if (!q->head && !q->tail)
        return NULL;
    // one item in the queue. free it of its shackles and liberate the head and tail.
    else if(q->head == q->tail)
    {
        // grab the value
        void* tmp = q->head->data;
        // "take care" of the queue
        kfree(q->head);
        q->head = NULL;
        q->tail = NULL;
        return tmp;
    }
    // two items in the queue. reject having multiple items, return to singletone
    else if(q->tail == q->head->next)
    {
        // grab the value
        void* tmp = q->head;
        // return to loneliness
        kfree(q->head);
        q->head->next = NULL;
        q->head = q->tail;
        return tmp;
    }
    // queue has stuff. decapitate and assign the new head 
    else
    {
        // grab the value
        void* tmp = q->head->data;
        // klopf klopf. wer ist da. pfort.
        struct QueueNode* newHead = q->head->next;
        kfree(q->head);
        q->head = newHead;
        return tmp;
    }
}

void kmemcpy(void* dest, const void* start, unsigned length)
{
    for(int i=0; i<length; i++)
        ((char*)dest)[i] = ((char*)start)[i];
}

int kstrlen(const char* str)
{
    int len=0;
    while(str[len]) len++;
    return len;
}
void kstrcpy(char* dest, const char* src)
{
    int len;
    if(kstrlen(dest) > kstrlen(src))
        len=kstrlen(dest);
    else
        len=kstrlen(src);
    for(int i=0; i<len; i++)
        dest[i] = src[i];
}

u8 inb(u16 port)
{
    u32 tmp;
    __asm__ volatile("inb %%dx,%%al" : "=a"(tmp) : "d"(port));
    return (u8)tmp;
}
void outb(u16 port, u8 value) 
{ 
    __asm__ volatile("outb %%al,%%dx" : : "a"(value), "d"(port)); 
}
u16 inw(u16 port)
{
    u32 tmp;
    asm volatile ("inw %%dx,%%ax"
        : "=a"(tmp) //one output
        : "d"(port) //one input
    );
    return (u16)tmp;
}
void outw(u16 port, u16 value)
{
    asm volatile("outw %%ax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}
u32 inl(u16 port)
{
    u32 tmp;
    asm volatile ("inl %%dx,%%eax"
        : "=a"(tmp)         //one output
        : "d"(port)         //one input
    );
    return tmp;
}
void outl(u16 port, u32 value)
{
    asm volatile("outl %%eax,%%dx"
        :   //no outputs
        : "a"(value), "d"(port) //two inputs
    );
}

void halt() {asm("hlt");}
