#ifndef PMM_H
#define PMM_H

#include "common.h"
#include "multiboot.h"

// Initializes the physical memory manager.
void pmm_init(u32 memory_size_kb, u32 bitmap_addr);

// Allocates a single 4KB frame of physical memory.
u32 pmm_alloc_frame();

// Marks a region of memory as in use.
void pmm_mark_region_used(u32 base_addr, u32 size_kb);

// Frees a 4KB frame of physical memory.
void pmm_free_frame(u32 addr);

#endif
