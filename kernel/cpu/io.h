#include <typedefs.h>

extern void outsw(u16, u32);
extern void outb(u16, u8);
extern void outw(u16, u16);
extern void outl(u16, u32);

extern u32 inl(u16);
extern u16 inw(u16);
extern u16 inb(u16);

extern void rep_outsw(u16 count, u16 port, volatile void *addy);
__attribute__((no_caller_saved_registers)) extern void
rep_insw(u16 count, u16 port, volatile void *addy);
extern void jump_usermode(void (*address)(), u32 stack_pointer);
