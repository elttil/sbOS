#include <assert.h>
#include <errno.h>
#include <fs/vfs.h>
#include <mmu.h>
#include <poll.h>

vfs_inode_t *root_dir;
vfs_mounts_t mounts[10];
int num_mounts = 0;

vfs_fd_t *get_vfs_fd(int fd) {
  if (fd >= 100) {
    klog("get_vfs_fd(): Tried to get out of range fd", LOG_WARN);
    dump_backtrace(12);
    return NULL;
  }
  if (fd < 0) {
    klog("get_vfs_fd(): Tried to get out of range fd", LOG_WARN);
    dump_backtrace(12);
    return NULL;
  }
  return get_current_task()->file_descriptors[fd];
}

vfs_inode_t *vfs_create_inode(
    int inode_num, int type, u8 has_data, u8 can_write, u8 is_open,
    void *internal_object, u64 file_size,
    vfs_inode_t *(*open)(const char *path),
    int (*create_file)(const char *path, int mode),
    int (*read)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    int (*write)(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd),
    void (*close)(vfs_fd_t *fd),
    int (*create_directory)(const char *path, int mode),
    vfs_vm_object_t *(*get_vm_object)(u64 length, u64 offset, vfs_fd_t *fd),
    int (*truncate)(vfs_fd_t *fd, size_t length),
    int (*stat)(vfs_fd_t *fd, struct stat *buf)) {
  vfs_inode_t *r = kmalloc(sizeof(inode_t));
  r->inode_num = inode_num;
  r->type = type;
  r->has_data = has_data;
  r->can_write = can_write;
  r->is_open = is_open;
  r->internal_object = internal_object;
  r->file_size = file_size;
  r->open = open;
  r->create_file = create_file;
  r->read = read;
  r->write = write;
  r->close = close;
  r->create_directory = create_directory;
  r->get_vm_object = get_vm_object;
  r->truncate = truncate;
  r->stat = stat;
  return r;
}

int vfs_create_fd(int flags, int mode, vfs_inode_t *inode, vfs_fd_t **fd) {
  process_t *p = (process_t *)get_current_task();
  int i;
  for (i = 0; i < 100; i++)
    if (!p->file_descriptors[i])
      break;
  if (p->file_descriptors[i])
    return -1;
  vfs_fd_t *r = kmalloc(sizeof(vfs_fd_t));
  r->flags = flags;
  r->mode = mode;
  r->inode = inode;
  r->reference_count = 1;
  r->offset = 0;
  p->file_descriptors[i] = r;
  if (fd)
    *fd = r;
  return i;
}

int vfs_create_file(const char *file) {
  vfs_mounts_t *file_mount = 0;
  int length = 0;
  for (int i = 0; i < num_mounts; i++) {
    int path_len = strlen(mounts[i].path);
    if (path_len <= length)
      continue;

    if (isequal_n(mounts[i].path, file, path_len)) {
      length = path_len;
      file_mount = &mounts[i];
    }
  }
  if (1 != length)
    file += length;

  if (!file_mount) {
    kprintf("vfs_internal_open could not find mounted path for file : %s\n",
            file);
    return 0;
  }
  assert(file_mount->local_root->create_file);
  return file_mount->local_root->create_file(file, 0);
}

vfs_inode_t *vfs_internal_open(const char *file) {
  vfs_mounts_t *file_mount = 0;
  int length = 0;
  for (int i = 0; i < num_mounts; i++) {
    int path_len = strlen(mounts[i].path);
    if (path_len <= length)
      continue;

    if (isequal_n(mounts[i].path, file, path_len)) {
      length = path_len;
      file_mount = &mounts[i];
    }
  }
  if (1 != length)
    file += length;

  if (!file_mount) {
    kprintf("vfs_internal_open could not find mounted path for file : %s\n",
            file);
    return NULL;
  }

  vfs_inode_t *ret = file_mount->local_root->open(file);
  return ret;
}

char *vfs_clean_path(const char *path, char *resolved_path) {
  char *clean = resolved_path;
  int prev_slash = 0;
  char *ptr = clean;
  for (; *path; path++) {
    if (prev_slash && '/' == *path) {
      continue;
    }
    prev_slash = ('/' == *path);
    *ptr = *path;
    ptr++;
  }
  *ptr = '\0';
  return clean;
}

char *vfs_resolve_path(const char *file, char *resolved_path) {
  if ('/' == *file) {
    return vfs_clean_path(file, resolved_path);
  }
  const char *cwd = get_current_task()->current_working_directory;
  size_t l = strlen(cwd);
  assert(l > 0);
  assert('/' == cwd[l - 1]);
  char r[256];
  strcpy(r, cwd);
  strcat(r, file);
  char *final = vfs_clean_path(r, resolved_path);
  return final;
}

int vfs_fstat(int fd, struct stat *buf) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd);
  if (!fd_ptr)
    return -EBADF;
  assert(fd_ptr->inode->stat);
  return fd_ptr->inode->stat(fd_ptr, buf);
}

int vfs_chdir(const char *path) {
  char tmp_path[256];
  char *resolved_path = vfs_resolve_path(path, tmp_path);
  {
    int tmp_fd = vfs_open(resolved_path, O_READ, 0);
    if (0 > tmp_fd)
      return tmp_fd;
    struct stat stat_result;
    vfs_fstat(tmp_fd, &stat_result);
    if (STAT_DIR != stat_result.st_mode) {
      kprintf("vfs_chdir: -ENOTDIR\n");
      return -ENOTDIR;
    }
    vfs_close(tmp_fd);
  }
  strcpy(get_current_task()->current_working_directory, resolved_path);
  if ('/' != resolved_path[strlen(resolved_path)])
    strcat(get_current_task()->current_working_directory, "/");
  return 0;
}

int vfs_mkdir(const char *path, int mode) {
  vfs_mounts_t *file_mount = 0;
  int length = 0;
  for (int i = 0; i < num_mounts; i++) {
    int path_len = strlen(mounts[i].path);
    if (path_len <= length)
      continue;

    if (isequal_n(mounts[i].path, path, path_len)) {
      length = path_len;
      file_mount = &mounts[i];
    }
  }
  if (1 != length)
    path += length;

  if (!file_mount) {
    kprintf("vfs_internal_open could not find mounted path for file : %s\n",
            path);
    return 0;
  }
  assert(file_mount->local_root->create_directory);
  // TODO: Error checking, don't just assume it is fine
  file_mount->local_root->create_directory(path, mode);
  return 0;
}

int vfs_open(const char *file, int flags, int mode) {
  char resolved_path[256] = {0};
  vfs_resolve_path(file, resolved_path);
  vfs_inode_t *inode = vfs_internal_open(resolved_path);
  if (0 == inode) {
    if (mode & O_CREAT) {
      if (vfs_create_file(resolved_path)) {
        klog("VFS: File created", LOG_NOTE);
        return vfs_open(file, flags, mode);
      }
      klog("VFS: Could not create file", LOG_WARN);
    }
    return -ENOENT;
  }
  if (inode->type == FS_TYPE_UNIX_SOCKET) {
    return uds_open(resolved_path);
  }

  return vfs_create_fd(flags, mode, inode, NULL);
}

int vfs_close(int fd) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd);
  if (NULL == fd_ptr) {
    return -1;
  }
  assert(0 < fd_ptr->reference_count);
  // Remove process reference
  fd_ptr->reference_count--;
  get_current_task()->file_descriptors[fd] = 0;
  // If no references left then free the contents
  if (0 == fd_ptr->reference_count) {
    if (fd_ptr->inode->close)
      fd_ptr->inode->close(fd_ptr);

    kfree(fd_ptr);
  }
  return 0;
}

int raw_vfs_pread(vfs_fd_t *vfs_fd, void *buf, u64 count,
                  u64 offset) {
  if (!(vfs_fd->flags & O_READ))
    return -EBADF;
  return vfs_fd->inode->read(buf, offset, count, vfs_fd);
}

int vfs_pread(int fd, void *buf, u64 count, u64 offset) {
  if (fd >= 100) {
    kprintf("EBADF : %x\n", fd);
    return -EBADF;
  }
  if (fd < 0) {
    dump_backtrace(12);
    kprintf("EBADF : %x\n", fd);
    return -EBADF;
  }
  vfs_fd_t *vfs_fd = get_current_task()->file_descriptors[fd];
  if (!vfs_fd)
    return -EBADF;
  int rc = raw_vfs_pread(vfs_fd, buf, count, offset);
  if (-EAGAIN == rc && count > 0) {
    if (!(vfs_fd->flags & O_NONBLOCK)) {
      struct pollfd fds;
      fds.fd = fd;
      fds.events = POLLIN;
      fds.revents = 0;
      poll(&fds, 1, 0);
      return vfs_pread(fd, buf, count, offset);
    }
  }
  return rc;
}

int raw_vfs_pwrite(vfs_fd_t *vfs_fd, void *buf, u64 count,
                   u64 offset) {
  assert(vfs_fd);
  assert(vfs_fd->inode);
  assert(vfs_fd->inode->write);
  return vfs_fd->inode->write(buf, offset, count, vfs_fd);
}

int vfs_pwrite(int fd, void *buf, u64 count, u64 offset) {
  vfs_fd_t *vfs_fd = get_vfs_fd(fd);
  if (!vfs_fd)
    return -EBADF;
  if (!(vfs_fd->flags & O_WRITE)) {
    return -EBADF;
  }
  return raw_vfs_pwrite(vfs_fd, buf, count, offset);
}

vfs_vm_object_t *vfs_get_vm_object(int fd, u64 length, u64 offset) {
  vfs_fd_t *vfs_fd = get_vfs_fd(fd);
  if (!vfs_fd)
    return NULL;
  vfs_vm_object_t *r = vfs_fd->inode->get_vm_object(length, offset, vfs_fd);
  return r;
}

int vfs_dup2(int org_fd, int new_fd) {
  get_current_task()->file_descriptors[new_fd] =
      get_current_task()->file_descriptors[org_fd];
  get_current_task()->file_descriptors[new_fd]->reference_count++;
  return 1;
}

int vfs_ftruncate(int fd, size_t length) {
  vfs_fd_t *fd_ptr = get_vfs_fd(fd);
  if (!fd_ptr)
    return -EBADF;
  if (!(fd_ptr->flags & O_READ))
    return -EINVAL;
  vfs_inode_t *inode = fd_ptr->inode;
  if (!inode)
    return -EINVAL;
  if (!inode->truncate)
    return -EINVAL;

  return inode->truncate(fd_ptr, length);
}

void vfs_mount(char *path, vfs_inode_t *local_root) {
  int len = strlen(path);
  mounts[num_mounts].path = kmalloc(len + 1);
  memcpy(mounts[num_mounts].path, path, len);
  mounts[num_mounts].path[len] = '\0';
  mounts[num_mounts].local_root = local_root;
  num_mounts++;
}
