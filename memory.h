#pragma once

#define PAGE_DEVICE_MEMORY ( (1<<3) | (1<<4) )
#define PAGE_PRESENT 1
#define PAGE_MUST_BE_ONE (1<<7)
#define PAGE_USER_ACCESS (1<<2)
#define PAGE_WRITEABLE (1<<1)

struct PageTable
{
    unsigned table[1024] __attribute__((aligned(4096))); 
};

void paging_init();
void enable_paging();
void set_page_table(struct PageTable* p);
struct PageTable* get_page_table();
unsigned get_faulting_address();

void memory_init();
void* kmalloc(unsigned size);
void kfree(void* p);