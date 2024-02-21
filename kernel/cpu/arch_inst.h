#include <stdint.h>
uintptr_t get_current_sp(void);
uintptr_t get_current_sbp(void);
__attribute__((__noreturn__)) void halt(void);
uintptr_t get_cr2(void);
void flush_tlb(void);
void set_sp(uintptr_t);
void set_sbp(uintptr_t);
void set_cr3(uintptr_t);
uintptr_t get_cr3(void);
void enable_paging(void);
