#ifndef UTIL_H
#define UTIL_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// this function can return if there's an interrupt
inline void hlt(void) {
    __asm("hlt");
}

inline void lidt(void *base, u16 size) {
    struct {
        u16 size;
        void *base;
    } __attribute__((packed)) idtr = {size, base};
    __asm("lidt %0" : : "m"(idtr));
}

inline void sti(void) {
    __asm("sti");
}

inline void cli(void) {
    __asm("cli");
}

inline void syscall(u64 rdi) {
    __asm("int $0x80" : : "D"(rdi));
}

#endif
