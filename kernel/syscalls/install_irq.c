#include <sched/scheduler.h>
#include <syscalls.h>

process_t *tmp;
void *handler;

void jump_signal_handler(void *func, u32 esp);

void test_handler(reg_t *regs) {
  kprintf("IRQ FIRED\n");
  kprintf("handler: %x\n", handler);
  tmp->interrupt_handler = handler;
  signal_t sig;
  sig.handler_ip = (uintptr_t)handler;
  process_push_signal(tmp, sig);
  return;
}

int syscall_install_irq(void (*irq_handler)(), u8 irq) {
  // TODO: This should be able to fail if the handler is already set
  tmp = get_current_task();
  handler = irq_handler;
  kprintf("IRQ INSTALLED\n");
  install_handler(test_handler, INT_32_INTERRUPT_GATE(0x0), 0x20 + irq);
  return 1;
}
