#include "heap.h"
#include "pmm.h"
#include "vmm.h"
#include "string.h"

// A simple kernel heap implementation using a linked list of free blocks.

// Header for each memory block (allocated or free)
typedef struct header {
    u32 magic;      // Magic number to identify a block
    u32 size;       // Size of the block, including the header
    u8  is_free;    // 1 if the block is free, 0 if allocated
    struct header* next; // Pointer to the next block in the list
} header_t;

#define HEAP_MAGIC 0x12345678

static header_t* heap_start = 0;

void heap_init() {
    // Allocate a single 4KB page for the initial heap.
    // For a real OS, you would want a more sophisticated way to grow the heap.
    heap_start = (header_t*)pmm_alloc_frame();
    if (!heap_start) {
        // Handle out of memory error. For now, we can't do much.
        return;
    }

    // The initial heap is one large free block.
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = 0x1000; // 4KB
    heap_start->is_free = 1;
    heap_start->next = 0;
}

void* kmalloc(u32 size) {
    if (!size) {
        return 0;
    }

    // We need space for the header and the requested size.
    // Also, align to 4 bytes.
    u32 total_size = sizeof(header_t) + size;
    if ((total_size & 0x3) != 0) { // Align to 4 bytes
        total_size &= ~0x3;
        total_size += 4;
    }

    header_t* current = heap_start;
    while (current) {
        if (current->is_free && current->size >= total_size) {
            // Found a suitable block. Should we split it?
            if (current->size > total_size + sizeof(header_t)) {
                // Split the block
                header_t* new_block = (header_t*)((u8*)current + total_size);
                new_block->magic = HEAP_MAGIC;
                new_block->size = current->size - total_size;
                new_block->is_free = 1;
                new_block->next = current->next;

                current->size = total_size;
                current->next = new_block;
            }

            current->is_free = 0;
            // Return a pointer to the memory right after the header
            return (void*)((u8*)current + sizeof(header_t));
        }
        current = current->next;
    }

    // TODO: Implement heap expansion if no suitable block is found.
    return 0; // Out of memory
}

void kfree(void* ptr) {
    if (!ptr) {
        return;
    }

    // Get the header from the pointer
    header_t* header = (header_t*)((u8*)ptr - sizeof(header_t));

    // Check the magic number to ensure it's a valid block
    if (header->magic != HEAP_MAGIC) {
        // Invalid pointer, or heap corruption
        return;
    }

    header->is_free = 1;

    // TODO: Implement coalescing with adjacent free blocks.
}
