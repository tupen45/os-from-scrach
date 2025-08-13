#ifndef VMM_H
#define VMM_H

#include "common.h"
#include "idt.h"

// A single entry in a page table
typedef struct {
    u32 present    : 1;   // Page is present in memory
    u32 rw         : 1;   // Read-only if 0, read-write if 1
    u32 user       : 1;   // Supervisor level only if 0
    u32 accessed   : 1;   // Has the page been accessed since last refresh?
    u32 dirty      : 1;   // Has the page been written to since last refresh?
    u32 unused     : 7;   // Amalgamation of unused and reserved bits
    u32 frame      : 20;  // Frame address (shifted right 12 bits)
} page_table_entry_t;

// A page table, containing 1024 entries
typedef struct {
    page_table_entry_t pages[1024];
} page_table_t;

// A page directory, containing 1024 pointers to page tables
typedef struct {
    u32 tables_physical[1024]; // The physical addresses of the page tables
} page_directory_t;

// Initializes the virtual memory manager.
void vmm_init();

#endif
