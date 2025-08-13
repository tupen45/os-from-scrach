#ifndef HEAP_H
#define HEAP_H

#include "common.h"

// Initializes the kernel heap.
void heap_init();

// Allocates a chunk of memory of a given size.
void* kmalloc(u32 size);

// Frees a previously allocated chunk of memory.
void kfree(void* ptr);

#endif
