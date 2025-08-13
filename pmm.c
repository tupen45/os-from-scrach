#include "pmm.h"
#include "string.h"

static u32* pmm_bitmap;
static u32  pmm_total_frames;
static u32  pmm_bitmap_size;

// Helper functions to set/clear bits in the bitmap
static void pmm_set_bit(u32 frame) {
    pmm_bitmap[frame / 32] |= (1 << (frame % 32));
}

static void pmm_clear_bit(u32 frame) {
    pmm_bitmap[frame / 32] &= ~(1 << (frame % 32));
}

static u32 pmm_test_bit(u32 frame) {
    return pmm_bitmap[frame / 32] & (1 << (frame % 32));
}

void pmm_init(u32 memory_size_kb, u32 bitmap_addr) {
    pmm_total_frames = memory_size_kb / 4;
    pmm_bitmap = (u32*)bitmap_addr;
    pmm_bitmap_size = (pmm_total_frames / 8);

    // Mark all memory as free initially
    memset(pmm_bitmap, 0x00, pmm_bitmap_size);
}

u32 pmm_alloc_frame() {
    for (u32 frame = 0; frame < pmm_total_frames; frame++) {
        if (!pmm_test_bit(frame)) {
            pmm_set_bit(frame);
            return frame * 0x1000; // Return physical address
        }
    }
    return 0; // Out of memory
}

void pmm_mark_region_used(u32 base_addr, u32 size_kb) {
    u32 base_frame = base_addr / 0x1000;
    u32 num_frames = size_kb / 4;

    for (u32 i = 0; i < num_frames; i++) {
        pmm_set_bit(base_frame + i);
    }
}

void pmm_free_frame(u32 addr) {
    u32 frame = addr / 0x1000;
    pmm_clear_bit(frame);
}
