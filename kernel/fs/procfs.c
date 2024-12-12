#include <errno.h>
#include <fs/procfs.h>
#include <lib/sb.h>
#include <lib/sv.h>
#include <stdio.h>

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))

vfs_inode_t *procfs_open(const char *p);

void procfs_close(vfs_fd_t *fd) {
  return;
}

void process_close(vfs_fd_t *fd) {
  process_t *p = fd->inode->internal_object;
  if (!p) {
    return;
  }
  process_remove_reference(p);
}

int procfs_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  struct sb builder;
  sb_init_buffer(&builder, buffer, len);

  size_t read_amount = 0;

  process_t *p = ready_queue;
  for (; p; p = p->next) {
    struct dirent entry;
    entry.d_ino = p->pid;
    ksnprintf(entry.d_name, sizeof(entry.d_name), "%u", p->pid);

    if (read_amount >= offset) {
      if (0 == sb_append_buffer(&builder, (u8 *)&entry, sizeof(entry))) {
        break;
      }
    }
    read_amount += sizeof(struct dirent);
  }
  return sv_length(SB_TO_SV(builder));
}

#define PROCESS_ROOT 0
#define PROCESS_SIGNAL 1
#define PROCESS_NAME 2

struct dirent process_entries[] = {
    {
        .d_ino = PROCESS_ROOT,
        .d_name = "",
    },
    {
        .d_ino = PROCESS_ROOT,
        .d_name = ".",
    },
    {
        .d_ino = PROCESS_SIGNAL,
        .d_name = "signal",
    },
    {
        .d_ino = PROCESS_NAME,
        .d_name = "name",
    },
};

int process_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  process_t *p = fd->inode->internal_object;

  struct sb builder;
  sb_init_buffer(&builder, buffer, len);

  int id = fd->inode->inode_num;
  if (PROCESS_ROOT == id) {
    size_t read_amount = 0;
    for (size_t i = 0; i < ARRAY_LENGTH(process_entries); i++) {
      if (0 == strlen(process_entries[i].d_name)) {
        continue;
      }
      if (read_amount >= offset) {
        if (0 == sb_append_buffer(&builder, (u8 *)&process_entries[i],
                                  sizeof(struct dirent))) {
          break;
        }
      }
      read_amount += sizeof(struct dirent);
    }
    return sv_length(SB_TO_SV(builder));
  }
  if (PROCESS_NAME == id) {
    struct sv program_name = C_TO_SV(p->program_name);
    sv_take(program_name, &program_name, offset);
    sb_append_sv(&builder, program_name);
    return sv_length(SB_TO_SV(builder));
  }
  return -EBADF;
}

int process_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  process_t *p = fd->inode->internal_object;

  int id = fd->inode->inode_num;
  if (PROCESS_SIGNAL == id) {
    struct sv t = sv_init(buffer, len);
    int got_num;
    u64 signal = sv_parse_unsigned_number(t, NULL, &got_num);
    if (got_num) {
      signal_process(p, signal);
    }
    // TODO: Add the ability to write "SIGTERM", "SIGKILL" etc without
    // writing the number.
    return 0;
  }
  return -EBADF;
}

vfs_inode_t *open_process(u64 pid, int id) {
  process_t *process = NULL;
  process_t *t = ready_queue;
  for (; t; t = t->next) {
    if ((u64)t->pid == pid) {
      process = t;
      break;
    }
  }
  if (!process) {
    return NULL;
  }

  vfs_inode_t *inode = vfs_create_inode(
      id /*inode_num*/, 0 /*type*/, 0 /*has_data*/, 0 /*can_write*/,
      0 /*is_open*/, 0, process /*internal_object*/, 0 /*file_size*/,
      procfs_open, NULL /*create_file*/, process_read, process_write,
      process_close, NULL /*create_directory*/, NULL /*get_vm_object*/,
      NULL /*truncate*/, NULL /*stat*/, NULL /*connect*/);
  if (!inode) {
    return NULL;
  }
  process->reference_count++;
  return inode;
}

vfs_inode_t *procfs_open(const char *p) {
  struct sv path = C_TO_SV(p);

  if ((sv_try_eat(path, &path, C_TO_SV("/")) && 0 == sv_length(path)) ||
      0 == sv_length(path)) {
    return vfs_create_inode(
        0 /*inode_num*/, 0 /*type*/, 0 /*has_data*/, 0 /*can_write*/,
        0 /*is_open*/, 0, NULL /*internal_object*/, 0 /*file_size*/,
        procfs_open, NULL /*create_file*/, procfs_read, NULL /* write */,
        procfs_close, NULL /*create_directory*/, NULL /*get_vm_object*/,
        NULL /*truncate*/, NULL /*stat*/, NULL /*connect*/);
  }

  int got_num;
  u64 pid = sv_parse_unsigned_number(path, &path, &got_num);
  if (!got_num) {
    return NULL;
  }

  // NOTE: There should only be one '/' since the previous vfs_open will
  // call vfs_clean_path first. This is a bit hacky and that code might
  // change later and as a result introduce bugs here.
  sv_try_eat(path, &path, C_TO_SV("/"));
  for (size_t i = 0; i < ARRAY_LENGTH(process_entries); i++) {
    if (sv_eq(path, C_TO_SV(process_entries[i].d_name))) {
      return open_process(pid, process_entries[i].d_ino);
    }
  }
  return NULL;
}

vfs_inode_t *procfs_mount(void) {
  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, 0 /*type*/, 0 /*has_data*/, 0 /*can_write*/,
      0 /*is_open*/, 0, NULL /*internal_object*/, 0 /*file_size*/, procfs_open,
      NULL /*create_file*/, procfs_read, NULL /* write */, procfs_close,
      NULL /*create_directory*/, NULL /*get_vm_object*/, NULL /*truncate*/,
      NULL /*stat*/, NULL /*connect*/);
  return inode;
}