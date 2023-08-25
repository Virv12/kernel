#pragma once

#include "util.h"

// TODO: this is the worst allocator ever

extern void *PDPT[512];

static u64 next_free_page = 510;

static void *virt_map(void const *addr, u64 len) {
    u64 const page_size = 1 << 30;
    u64 const page_bitmask = page_size - 1;

    u64 start = (u64)addr & ~page_bitmask;
    u64 end = ((u64)addr + len + page_bitmask) & ~page_bitmask;

    u64 pages = (end - start) / page_size;

    next_free_page -= pages;

    for (u64 i = 0; i < pages; ++i) {
        PDPT[next_free_page + i] = (void *)(start + i * page_size | 0x83);

        // TODO: invlpg
    }

    return (void *)(((u64)addr - start) + next_free_page * page_size +
                    (u64)0xffffff8000000000);
}

static u8 *phys_free_page = 0;

// TODO: we can allocate about 160 pages before we run out of space
static void *phys_alloc_page(void) {
    u8 *page = phys_free_page;
    phys_free_page += 0x1000;
    return page;
}

static void *alloc_page(void) {
    void *page = phys_alloc_page();
    return virt_map(page, 0x1000);
}
