#include "vmm.h"
#include "pmm.h"
#include "string.h"
#include "terminal.h"

extern void load_page_directory(u32);
extern void enable_paging();

page_directory_t* kernel_directory = 0;

void vmm_map_page(u32 virt, u32 phys) {
    u32 pd_index = virt / 0x400000;
    u32 pt_index = (virt / 0x1000) % 1024;

    // If the page table doesn't exist, create it
    if (!kernel_directory->tables_physical[pd_index]) {
        u32 phys_addr = pmm_alloc_frame();
        kernel_directory->tables_physical[pd_index] = phys_addr | 0x3; // Present, RW
        memset((void*)phys_addr, 0, 0x1000);
    }

    // Map the page
    page_table_t* table = (page_table_t*)(kernel_directory->tables_physical[pd_index] & ~0xFFF);
    table->pages[pt_index].present = 1;
    table->pages[pt_index].rw = 1;
    table->pages[pt_index].frame = phys / 0x1000;
}

void vmm_init() {
    // Allocate a page-aligned directory
    kernel_directory = (page_directory_t*)pmm_alloc_frame();
    memset(kernel_directory, 0, sizeof(page_directory_t));

    // Identity-map the first 4MB of memory
    // This is where our kernel and initial data resides
    for (u32 i = 0; i < 0x400000; i += 0x1000) {
        vmm_map_page(i, i);
    }

    // Load the page directory and enable paging
    load_page_directory((u32)kernel_directory->tables_physical);
    enable_paging();

    term_print("Paging enabled!\n");
}
