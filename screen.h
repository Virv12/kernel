#include "allocator.h"
#include "util.h"

struct framebuffer_info {
    void *addr;
    u32 pitch;
    u32 width;
    u32 height;
    u8 bpp;
    u8 type;
};

static struct framebuffer_info info;

static u32 WIDTH;
static u32 HEIGHT;

static void init_screen(struct framebuffer_info const *p) {
    info = *p;
    info.addr = virt_map(p->addr, p->pitch * p->height);
    WIDTH = info.width;
    HEIGHT = info.height;
}

static void set_char(u32 row, u32 col, u8 c) {
    ((u16 volatile *)info.addr)[row * WIDTH + col] = c | 0x0700;
}
