#include "util.h"

extern inline void hlt(void);

extern inline void lidt(void *base, u16 size);

extern inline void sti(void);

extern inline void cli(void);

extern inline u64 rdmsr(u32 msr);

extern inline void wrmsr(u32 msr, u64 value);
