#pragma once

#include "util.h"

void *virt_alloc(u64 len);

void *phys_alloc(u64 len);

void mem_map(void *phys, void *virt, u64 len);

inline void *virt_map(void *phys, u64 len) {
    len = (len + 8191) & -8192;
    void *virt = virt_alloc(len);
    mem_map((void *)((u64)phys & -(u64)4096), virt, len);
    return (u8 *)virt + (u64)phys % 4096;
}

inline void *mem_alloc(u64 len) {
    void *phys = phys_alloc(len);
    void *virt = virt_alloc(len);
    mem_map(phys, virt, len);
    return virt;
}
