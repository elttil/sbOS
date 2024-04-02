#include <assert.h>
#include <errno.h>
#include <sched/scheduler.h>

int syscall_open_process(int pid) {
  // TODO: Permission check
  process_t *process = (process_t *)ready_queue;
  for (; process; process = process->next) {
    if (pid == process->pid) {
      break;
    }
  }
  if (!process) {
    return -ESRCH;
  }

  vfs_inode_t *inode = vfs_create_inode(
      process->pid, 0 /*type*/, 0 /*has_data*/, 0 /*can_write*/, 1 /*is_open*/,
      process /*internal_object*/, 0 /*file_size*/, NULL /*open*/,
      NULL /*create_file*/, NULL /*read*/, NULL /*write*/, NULL /*close*/,
      NULL /*create_directory*/, NULL /*get_vm_object*/, NULL /*truncate*/,
      NULL /*stat*/, process_signal);
  int rc = vfs_create_fd(0, 0, 0, inode, NULL);
  assert(rc >= 0);
  return rc;
}
