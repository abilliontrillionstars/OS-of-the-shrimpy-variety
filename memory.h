#pragma once

void memory_init();
void* kmalloc(unsigned size);
void kfree(void* p);
