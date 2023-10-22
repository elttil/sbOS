#include <stdint.h>

__attribute__((no_caller_saved_registers)) extern void outsw(uint16_t,
                                                             uint32_t);
__attribute__((no_caller_saved_registers)) extern void outb(uint16_t, uint16_t);
__attribute__((no_caller_saved_registers)) extern uint16_t inb(uint16_t);
__attribute__((no_caller_saved_registers)) extern void
rep_outsw(uint16_t count, uint16_t port, volatile void *addy);
__attribute__((no_caller_saved_registers)) extern void
rep_insw(uint16_t count, uint16_t port, volatile void *addy);
extern void jump_usermode(void (*address)(), uint32_t stack_pointer);
