#include <cpuid.h>
#include <sys/io.h>

#include "screen.h"
#include "util.h"

static u32 chosen_row;
static u32 row;
static u32 col;
static u8 buf[32 << 10];

static void update_cursor(void) {
    u16 pos = (row - chosen_row) * WIDTH + col;
    outb(0x0F, 0x03D4);
    outb(pos & 0xFF, 0x03D5);
    outb(0x0E, 0x03D4);
    outb((pos >> 8) & 0xFF, 0x03D5);
}

static void set_chosen_row(u32 r) {
    chosen_row = r;

    for (u32 i = 0; i < HEIGHT; ++i) {
        for (u32 j = 0; j < WIDTH; ++j) {
            set_char(i, j, buf[(i + chosen_row) * WIDTH + j]);
        }
    }

    update_cursor();
}

static void putc_inner(char c) {
    if (c == '\n') {
        col = WIDTH;
    } else {
        buf[row * WIDTH + col] = c;
        if (chosen_row <= row && row < chosen_row + HEIGHT) {
            set_char(row - chosen_row, col, c);
        }
        col++;
    }

    if (col == WIDTH) {
        col = 0;
        ++row;
    }
}

static void putc(char c) {
    putc_inner(c);
    update_cursor();
}

static void puts(char const *s) {
    while (*s) {
        putc_inner(*s);
        ++s;
    }
    update_cursor();
}

static void sputs(char const *s, u32 n) {
    while (*s && n) {
        putc_inner(*s);
        ++s;
        --n;
    }
    update_cursor();
}

static void putu(u64 x) {
    char buf[32] = {0};
    int i = 31;

    do {
        buf[--i] = "0123456789"[x % 10];
        x /= 10;
    } while (x);

    puts(buf + i);
}

static void puti(i64 x) {
    if (x < 0) {
        putc('-');
        x = -x;
    }
    putu((u64)x);
}

static void putx(u64 x) {
    char buf[32] = {0};
    int i = 31;

    do {
        buf[--i] = "0123456789ABCDEF"[x % 16];
        x /= 16;
    } while (x);

    puts("0x");
    puts(buf + i);
}

struct sdt_header {
    char Signature[4];
    u32 Length;
    u8 Revision;
    u8 Checksum;
    char OEMID[6];
    char OEMTableID[8];
    u32 OEMRevision;
    u32 CreatorID;
    u32 CreatorRevision;
};

struct rsdt_header {
    struct sdt_header sdt;
    u32 PointerToOtherSDT[];
};

struct madt_header {
    struct sdt_header sdt;
    u32 LocalAPICAddress;
    u32 Flags;
    u8 Entries[];
};

static void print_sdt(struct sdt_header const *p) {
    puts("  sdt (");
    putx((u64)p);
    puts("):\n");
    puts("    signature: ");
    sputs(p->Signature, 4);
    putc('\n');
    puts("    length: ");
    putu(p->Length);
    putc('\n');
    puts("    revision: ");
    putu(p->Revision);
    putc('\n');
    puts("    checksum: ");
    putu(p->Checksum);
    putc('\n');
    puts("    oem_id: ");
    sputs(p->OEMID, 6);
    putc('\n');
    puts("    oem_table_id: ");
    sputs(p->OEMTableID, 8);
    putc('\n');
    puts("    oem_revision: ");
    putu(p->OEMRevision);
    putc('\n');
    puts("    creator_id: ");
    putu(p->CreatorID);
    putc('\n');
    puts("    creator_revision: ");
    putu(p->CreatorRevision);
    putc('\n');

    if (*(u32 *)&p->Signature == 0x43495041) {  // APIC
        struct madt_header const *q = (struct madt_header const *)p;
        puts("\n");
        puts("    local_apic_address: ");
        putx(q->LocalAPICAddress);
        putc('\n');
        puts("    flags: ");
        putx(q->Flags);
        putc('\n');

        u8 const *entries = q->Entries;
        while (entries - (u8 *)p < p->Length) {
            puts("    entry: ");
            putu(entries[0]);
            putc(' ');
            putu(entries[1]);
            putc('\n');

            if (entries[0] == 0) {
                puts("      apic processor id: ");
                putu(entries[2]);
                putc('\n');
                puts("      apic id: ");
                putu(entries[3]);
                putc('\n');
                puts("      flags: ");
                putx(*(u32 *)(entries + 4));
                putc('\n');
            }
            if (entries[0] == 1) {
                puts("      io apic id: ");
                putu(entries[2]);
                putc('\n');
                puts("      io apic address: ");
                putx(*(u32 *)(entries + 4));
                putc('\n');
                puts("      global system interrupt base: ");
                putx(*(u32 *)(entries + 8));
                putc('\n');
            }
            if (entries[0] == 2) {
                puts("      bus source: ");
                putu(entries[2]);
                putc('\n');
                puts("      irq source: ");
                putu(entries[3]);
                putc('\n');
                puts("      global system interrupt: ");
                putx(*(u32 *)(entries + 4));
                putc('\n');
                puts("      flags: ");
                putx(*(u16 *)(entries + 8));
                putc('\n');
            }

            entries += entries[1];
        }
    }
}

static void print_multiboot_info(u8 const *p) {
    puts("Multiboot info (");
    putx((u64)p);
    puts("):\n");

    u32 total_size = *(u32 *)p;
    puts("total_size: ");
    putu(total_size);
    putc('\n');

    for (u32 i = 8; i < total_size;) {
        u32 tag_type = *(u32 *)(p + i);
        u32 tag_size = *(u32 *)(p + i + 4);
        puts("tag: ");
        putu(i);
        putc(' ');
        putu(tag_type);
        putc(' ');
        putu(tag_size);
        putc('\n');
        if (tag_type == 0) {
            break;
        }
        if (tag_type == 1) {
            puts("  cmdline: ");
            sputs(p + i + 8, tag_size - 8);
            putc('\n');
        }
        if (tag_type == 2) {
            puts("  bootloader_name: ");
            sputs(p + i + 8, tag_size - 8);
            putc('\n');
        }
        if (tag_type == 4) {
            puts("  mem_lower: ");
            putu(*(u32 *)(p + i + 8));
            putc('\n');
            puts("  mem_upper: ");
            putu(*(u32 *)(p + i + 12));
            putc('\n');
        }
        if (tag_type == 6) {
            puts("  mmap:\n");
            u32 entry_size = *(u32 *)(p + i + 8);
            u32 entry_version = *(u32 *)(p + i + 12);
            puts("    entry_size: ");
            putu(entry_size);
            putc('\n');
            puts("    entry_version: ");
            putu(entry_version);
            putc('\n');

            u32 entry_count = (tag_size - 16) / entry_size;
            for (u32 j = 0; j < entry_count; ++j) {
                u64 base = *(u64 *)(p + i + 16 + j * entry_size);
                u64 length = *(u64 *)(p + i + 16 + j * entry_size + 8);
                u32 type = *(u32 *)(p + i + 16 + j * entry_size + 16);
                puts("    ");
                putx(base);
                puts(" ");
                putx(length);
                puts(" ");
                putu(type);
                putc('\n');
            }
        }
        if (tag_type == 8) {
            puts("  framebuffer:\n");
            puts("    address: ");
            putx(*(u64 *)(p + i + 8));
            putc('\n');
            puts("    pitch: ");
            putu(*(u32 *)(p + i + 16));
            putc('\n');
            puts("    width: ");
            putu(*(u32 *)(p + i + 20));
            putc('\n');
            puts("    height: ");
            putu(*(u32 *)(p + i + 24));
            putc('\n');
            puts("    bpp: ");
            putu(*(u8 *)(p + i + 28));
            putc('\n');
            puts("    type: ");
            putu(*(u8 *)(p + i + 29));
            putc('\n');
        }
        if (tag_type == 14) {
            puts("  rsdp:\n");
            u32 rsdt_address = *(u32 *)(p + i + 24);

            puts("    signature: ");
            sputs(p + i + 8, 8);
            putc('\n');
            puts("    checksum: ");
            putx(*(u8 *)(p + i + 16));
            putc('\n');
            puts("    oem_id: ");
            sputs(p + i + 17, 6);
            putc('\n');
            puts("    revision: ");
            putx(*(u8 *)(p + i + 23));
            putc('\n');
            puts("    rsdt_address: ");
            putx(rsdt_address);
            putc('\n');

            struct rsdt_header const *rsdt = (void *)(u64)rsdt_address;
            print_sdt((void *)rsdt);

            u32 count = (rsdt->sdt.Length - sizeof(*rsdt)) / 4;
            for (u32 j = 0; j < count; ++j) {
                print_sdt((void *)(u64)rsdt->PointerToOtherSDT[j]);
            }
        }
        if (tag_type == 21) {
            puts("  load_base_addr: ");
            putx(*(u32 *)(p + i + 8));
            putc('\n');
        }

        i = (i + tag_size + 7) & (u32)(~7);
    }
}

struct __attribute__((aligned(16))) interrupt_descriptor {
    u16 offset_1;  // offset bits 0..15
    u16 selector;  // a code segment selector in GDT or LDT
    u8 ist;        // bits 0..2 holds Interrupt Stack Table offset, rest of bits
                   // zero.
    u8 type_attributes;  // gate type, dpl, and p fields
    u16 offset_2;        // offset bits 16..31
    u32 offset_3;        // offset bits 32..63
    u32 zero;            // reserved
};

static struct interrupt_descriptor idt[256];

struct interrupt_frame {
    u64 ip;
    u64 cs;
    u64 flags;
    u64 sp;
    u64 ss;
};

static void set_idt(
    struct interrupt_descriptor *id,
    __attribute__((interrupt)) void handler(struct interrupt_frame *)) {
    id->offset_1 = (u16)((u64)handler & 0xffff);
    id->selector = 0x8;
    id->ist = 0;
    id->type_attributes = 0x8e;
    id->offset_2 = (u16)(((u64)handler >> 16) & 0xffff);
    id->offset_3 = (u32)(((u64)handler >> 32) & 0xffffffff);
    id->zero = 0;
}

__attribute__((interrupt)) static void keyboard_interrupt_handler(
    struct interrupt_frame *frame) {
    static char const shift_table[256] = {
        [0X10] = 'Q', [0X11] = 'W', [0X12] = 'E',  [0X13] = 'R', [0X14] = 'T',
        [0X15] = 'Y', [0X16] = 'U', [0X17] = 'I',  [0X18] = 'O', [0X19] = 'P',
        [0X1E] = 'A', [0X1F] = 'S', [0X20] = 'D',  [0X21] = 'F', [0X22] = 'G',
        [0X23] = 'H', [0X24] = 'J', [0X25] = 'K',  [0X26] = 'L', [0X2C] = 'Z',
        [0X2D] = 'X', [0X2E] = 'C', [0X2F] = 'V',  [0X30] = 'B', [0X31] = 'N',
        [0X32] = 'M', [0X39] = ' ', [0X1C] = '\n',
    };
    static char const normal_table[256] = {
        [0x10] = 'q', [0x11] = 'w', [0x12] = 'e',  [0x13] = 'r', [0x14] = 't',
        [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',  [0x18] = 'o', [0x19] = 'p',
        [0x1e] = 'a', [0x1f] = 's', [0x20] = 'd',  [0x21] = 'f', [0x22] = 'g',
        [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',  [0x26] = 'l', [0x2c] = 'z',
        [0x2d] = 'x', [0x2e] = 'c', [0x2f] = 'v',  [0x30] = 'b', [0x31] = 'n',
        [0x32] = 'm', [0x39] = ' ', [0x1c] = '\n',
    };

    static u32 mods = 0;

    u8 b = inb(0x60);

    // if ALT
    if (b == 0x38) {
        mods |= 1;
        goto end;
    }
    // if SHIFT
    if (b == 0x2a || b == 0x36) {
        mods |= 2;
        goto end;
    }
    // if CTRL
    if (b == 0x1d) {
        mods |= 4;
        goto end;
    }
    // if ALT released
    if (b == 0xb8) {
        mods &= ~1;
        goto end;
    }
    // if SHIFT released
    if (b == 0xaa || b == 0xb6) {
        mods &= ~2;
        goto end;
    }
    // if CTRL released
    if (b == 0x9d) {
        mods &= ~4;
        goto end;
    }

    switch (mods) {
        case 0:
            if (normal_table[b]) {
                putc(normal_table[b]);
            }
            break;
        case 2:
            if (shift_table[b]) {
                putc(shift_table[b]);
            }
            break;
        case 1:
            if (b == 0x24) {
                set_chosen_row(chosen_row + 1);
            }
            if (b == 0x25 && chosen_row > 0) {
                set_chosen_row(chosen_row - 1);
            }
            break;
    }

end:
    outb(0x20, 0x20);
}

u8 *local_apic_address;

static void init_apic(void) {
    u64 apic_base_msr = rdmsr(0x1B);
    puts("apic_base_msr: ");
    putx(apic_base_msr);
    putc('\n');

    *(u32 *)(local_apic_address + 0x320) = 0x40020;  // tsc-deadline mode
}

static void init_acpi(struct rsdt_header const *rsdt) {
    u32 count = (rsdt->sdt.Length - sizeof(*rsdt)) / 4;
    for (u32 j = 0; j < count; ++j) {
        struct sdt_header const *sdt = (void *)(u64)rsdt->PointerToOtherSDT[j];

        if (*(u32 *)&sdt->Signature == 0x43495041) {  // APIC
            struct madt_header const *q = (struct madt_header const *)sdt;
            local_apic_address = (void *)(u64)q->LocalAPICAddress;
            init_apic();
        }
    }
}

static void init(u8 const *const mbi) {
    u32 total_size = *(u32 *)mbi;
    for (u32 i = 8; i < total_size;) {
        u32 tag_type = *(u32 *)(mbi + i);
        u32 tag_size = *(u32 *)(mbi + i + 4);
        if (tag_type == 0) {
            break;
        }
        if (tag_type == 8) {
            init_screen(mbi + i + 8);
            set_chosen_row(0);
        }
        if (tag_type == 14) {
            u32 rsdt_address = *(u32 *)(mbi + i + 24);
            struct rsdt_header const *rsdt = (void *)(u64)rsdt_address;
            init_acpi(rsdt);
        }
        i = (i + tag_size + 7) & (u32)(~7);
    }
}

struct registers {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 rsp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 rip;
    u64 rflags;
};

struct thread {
    struct registers registers;

    u64 wake_at;
};

struct thread threads[3];

struct thread *current_thread;

__attribute__((noreturn)) void launch(struct thread *p);

__attribute__((noreturn)) void scheduler(void) {
    u64 now = __builtin_ia32_rdtsc();

    // puts("[sched] ");
    // putu(now * 5 / 11 / 1000000);
    // puts(" ms\n");

    if (current_thread && current_thread->wake_at < now) {
        current_thread->wake_at = now;
    }

    u32 min = 0;
    u32 min2 = 0;
    for (u32 i = 1; i < 3; ++i) {
        if (threads[i].wake_at <= threads[min].wake_at) {
            min2 = min;
            min = i;
        } else if (threads[i].wake_at <= threads[min2].wake_at) {
            min2 = i;
        }
    }

    if (threads[min].wake_at <= now) {
        u64 timer_at = threads[min2].wake_at;
        if (timer_at <= now) timer_at = now + 22000000;
        wrmsr(0x6E0, timer_at);

        launch(&threads[min]);
    }

    // set apic timer to fire at threads[min].wake_at
    wrmsr(0x6E0, threads[min].wake_at);

    // no thread to run
    current_thread = 0;
    sti();
    for (;;) hlt();
}

void timer_landpad();

void scheduler_trampoline(void);

__attribute__((noreturn)) void syscall_handler(void) {
    current_thread->wake_at = current_thread->registers.rdi;
    scheduler();
}

static void my_kthread1(void) {
    u64 now = 0;
    for (;;) {
        // TODO: calling puts() in a kthread is a race condition
        u64 woken_at = __builtin_ia32_rdtsc();
        puts("[kthread1] ");
        putu(woken_at * 5 / 11 / 1000000);
        puts(" ms\n");

        now += 2200000000;
        current_thread->wake_at = now;
        scheduler_trampoline();
    }
}

static u64 fib(u64 n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

struct mutex {
    u64 locked;
    struct thread *waiter;
};

static void mutex_lock(struct mutex *m) {
    for (;;) {
        cli();
        if (!m->locked) break;
        m->waiter = current_thread;
        current_thread->wake_at = (u64)1 << 50;
        scheduler_trampoline();
    }
    m->locked = 1;
    sti();
}

static void mutex_unlock(struct mutex *m) {
    cli();
    m->locked = 0;
    if (m->waiter) {
        m->waiter->wake_at = __builtin_ia32_rdtsc();
        m->waiter = 0;
    }
    sti();
}

static struct mutex m;

static void my_kthread2(void) {
    mutex_lock(&m);

    u64 start = __builtin_ia32_rdtsc();
    u64 f = fib(42);
    u64 end = __builtin_ia32_rdtsc();

    mutex_unlock(&m);

    puts("[kthread2] fib(42) = ");
    putu(f);
    puts(" [");
    putu(start * 5 / 11 / 1000000);
    putc(' ');
    putu(end * 5 / 11 / 1000000);
    puts("]\n");

    current_thread->wake_at = (u64)1 << 50;
    scheduler_trampoline();
}

static void my_kthread3(void) {
    mutex_lock(&m);

    u64 start = __builtin_ia32_rdtsc();
    u64 f = fib(42);
    u64 end = __builtin_ia32_rdtsc();

    mutex_unlock(&m);

    puts("[kthread3] fib(42) = ");
    putu(f);
    puts(" [");
    putu(start * 5 / 11 / 1000000);
    putc(' ');
    putu(end * 5 / 11 / 1000000);
    puts("]\n");

    current_thread->wake_at = (u64)1 << 50;
    scheduler_trampoline();
}

static char kthread_stack1[4096];
static char kthread_stack2[4096];
static char kthread_stack3[4096];

__attribute__((interrupt)) static void nop_handler(
    struct interrupt_frame *frame) {
    outb(0x20, 0x20);
}

__attribute__((noreturn)) void kmain(u8 const *const p) {
    init(p);

    // print_multiboot_info(p);

    set_idt(&idt[0x08], nop_handler);
    set_idt(&idt[0x09], keyboard_interrupt_handler);
    set_idt(&idt[0x20], timer_landpad);
    set_idt(&idt[0x21], keyboard_interrupt_handler);

    lidt(idt, sizeof(idt));

    // disable pic
    // outb(0xff, 0xa1);
    // outb(0xff, 0x21);

    threads[0].registers.rip = (u64)&my_kthread1;
    threads[0].registers.rsp = (u64)&kthread_stack1[4096];
    threads[0].registers.rflags = 0x200;
    threads[1].registers.rip = (u64)&my_kthread2;
    threads[1].registers.rsp = (u64)&kthread_stack2[4096];
    threads[1].registers.rflags = 0x200;
    threads[2].registers.rip = (u64)&my_kthread3;
    threads[2].registers.rsp = (u64)&kthread_stack3[4096];
    threads[2].registers.rflags = 0x200;
    scheduler();
}
