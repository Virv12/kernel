#include "allocator.h"

extern u8 kernel_offset;

_Alignas(0x1000) static u8 buf[640 << 10];
static u8 *phys_free_page = buf;
static u8 *virt_free_page = &kernel_offset;

void *phys_alloc(u64 len) {
    phys_free_page = (u8 *)((u64)phys_free_page & ~(u64)&kernel_offset);

    len = (len + 0xfff) & ~(u64)0xfff;
    u8 *page = phys_free_page;
    phys_free_page += len;
    return page;
}

void *virt_alloc(u64 len) {
    len = (len + 0xfff) & ~(u64)0xfff;
    virt_free_page -= len;
    return (void *)virt_free_page;
}

static void mem_map_impl(u64 start, u64 end, u64 virt, u64 phys, u64 len) {
    u64 subpage = (end - start) / 512;

    u64 x = subpage * max(virt / subpage, start / subpage);
    u64 y = subpage * min((virt + len + subpage - 1) / subpage, end / subpage);

    for (u64 k = x; k < y; k += subpage) {
        u64 entry_stub = 8 * (k / subpage);
        entry_stub |= 0x804020100800 * (((u64)1 << 48) / subpage);

        u64 signext =
            (entry_stub & 0x0000800000000000) ? 0xffff000000000000 : 0;
        u64 *entry = (u64 *)(entry_stub | signext);

        if (subpage == 0x1000) {
            *entry = (phys + k - virt) | 0x3;
            continue;
        }

        if (*entry == 0) *entry = (u64)phys_alloc(0x1000) | 0x3;
        mem_map_impl(k, k + subpage, virt, phys, len);
    }
}

void mem_map(void *phys, void *virt, u64 len) {
    if (len == 0) return;
    mem_map_impl(0, (u64)1 << 48, (u64)virt & ((u64)1 << 48) - 1, (u64)phys,
                 len);
    // flush TLB
}

extern inline void *virt_map(void *phys, u64 len);
extern inline void *mem_alloc(u64 len);
