#include <sched/scheduler.h>
#include <syscalls.h>

process_t *tmp;
void *handler;

void jump_signal_handler(void *func, u32 esp);

void test_handler(reg_t *regs) {
  tmp->interrupt_handler = handler;
  signal_t sig;
  sig.handler_ip = (uintptr_t)handler;
  process_push_signal(tmp, sig);
  return;
}

int syscall_install_irq(void (*irq_handler)(), u8 irq) {
  // TODO: This should be able to fail if the handler is already set
  tmp = current_task;
  handler = irq_handler;
  install_handler(test_handler, INT_32_INTERRUPT_GATE(0x0), 0x20 + irq);
  return 1;
}
