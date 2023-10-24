#include <stdint.h>

extern void outsw(uint16_t, uint32_t);
extern void outb(uint16_t, uint8_t);
extern void outw(uint16_t, uint16_t);
extern void outl(uint16_t, uint32_t);

extern uint32_t inl(uint16_t);
extern uint16_t inb(uint16_t);

extern void rep_outsw(uint16_t count, uint16_t port, volatile void *addy);
__attribute__((no_caller_saved_registers)) extern void
rep_insw(uint16_t count, uint16_t port, volatile void *addy);
extern void jump_usermode(void (*address)(), uint32_t stack_pointer);
