#include <sys/io.h>

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

extern u8 font_start[];

static u32 WIDTH;
static u32 HEIGHT;

static void init_screen(struct framebuffer_info const *p) {
    info = *p;
    info.addr = virt_map(p->addr, p->pitch * p->height);
    switch (info.type) {
        case 1:
            WIDTH = info.width / 8;
            HEIGHT = info.height / 16;
            break;
        case 2:
            WIDTH = info.width;
            HEIGHT = info.height;
            break;
        default:
            // TODO: not supported
            break;
    }
}

static void set_char(u32 row, u32 col, u8 ch) {
    switch (info.type) {
        case 1:
            for (u32 r = 0; r < 16; ++r) {
                for (u32 c = 0; c < 8; ++c) {
                    u32 index = (row * 16 + r) * WIDTH * 8 + (col * 8 + c);
                    u8 byte =
                        (font_start[ch * 16 + r] & (1 << (7 - c))) ? 0xff : 0;
                    u32 pixel_size = (info.bpp + 7) / 8;
                    for (u32 i = 0; i < pixel_size; ++i) {
                        ((u8 volatile *)info.addr)[index * pixel_size + i] =
                            byte;
                    }
                }
            }
            break;
        case 2:
            ((u16 volatile *)info.addr)[row * WIDTH + col] = ch | 0x0700;
            break;
        default:
            break;
    }
}

static void set_cursor(u32 row, u32 col) {
    switch (info.type) {
        case 2: {
            u16 pos = row * WIDTH + col;
            outb(0x0F, 0x03D4);
            outb(pos & 0xFF, 0x03D5);
            outb(0x0E, 0x03D4);
            outb((pos >> 8) & 0xFF, 0x03D5);
        } break;
        default:
            break;
    }
}
