#include <assert.h>
#include <fs/ext2.h>
#include <fs/vfs.h>
#include <stdint.h>
#include <string.h>

#define EXT2_SUPERBLOCK_SECTOR 2
#define EXT2_ROOT_INODE 2

#define BLOCKS_REQUIRED(_a, _b) ((_a) / (_b) + (((_a) % (_b)) != 0))

superblock_t *superblock;
uint32_t block_byte_size;
uint32_t inode_size;
uint32_t inodes_per_block;

#define BLOCK_SIZE (block_byte_size)

void ext2_close(vfs_fd_t *fd) {
  return; // There is nothing to clear
}

int read_inode(int inode_num, unsigned char *data, uint64_t size,
               uint64_t offset, uint64_t *file_size);

uint32_t old_block_num = -1;
uint8_t old_block[1024]; // FIXME: Not always 1024
void ext2_read_block(uint32_t block, void *address, size_t size,
                     size_t offset) {
  // Very simple cache, it you are reading the same block then don't
  // bother trying to read from the hard drive again, juse use the old
  // data.
  if (block == old_block_num) {
    memcpy(address, old_block + offset, size);
    return;
  }
  read_lba(block * block_byte_size / 512, address, size, offset);
  read_lba(block * block_byte_size / 512, old_block, 1024, 0);
  old_block_num = block;
}

void ext2_write_block(uint32_t block, void *address, size_t size,
                      size_t offset) {
  old_block_num = -1; // Invalidate the old block cache
  write_lba(block * block_byte_size / 512, address, size, offset);
}

void write_group_descriptor(uint32_t group_index, bgdt_t *block_group) {
  int starting_block = (1024 == block_byte_size) ? 2 : 1;
  ext2_write_block(starting_block, block_group, sizeof(bgdt_t),
                   group_index * sizeof(bgdt_t));
}

void get_group_descriptor(uint32_t group_index, bgdt_t *block_group) {
  int starting_block = (1024 == block_byte_size) ? 2 : 1;
  ext2_read_block(starting_block, block_group, sizeof(bgdt_t),
                  group_index * sizeof(bgdt_t));
}

uint32_t num_block_groups(void) {
  // Determining the Number of Block Groups

  // From the Superblock, extract the size of each block, the total
  // number of inodes, the total number of blocks, the number of blocks
  // per block group, and the number of inodes in each block group. From
  // this information we can infer the number of block groups there are
  // by:

  // Rounding up the total number of blocks divided by the number of
  // blocks per block group
  uint32_t num_blocks = superblock->num_blocks;
  uint32_t num_blocks_in_group = superblock->num_blocks_in_group;
  uint32_t b = num_blocks / num_blocks_in_group;
  if (num_blocks % num_blocks_in_group != 0)
    b++;

  // Rounding up the total number of inodes divided by the number of
  // inodes per block group
  uint32_t num_inodes = superblock->num_inodes;
  uint32_t num_inodes_in_group = superblock->num_inodes_in_group;
  uint32_t i = num_inodes / num_inodes_in_group;
  if (num_inodes % num_inodes_in_group != 0)
    i++;
  // Both (and check them against each other)
  assert(i == b);
  return i;
}

void ext2_block_containing_inode(uint32_t inode_index, uint32_t *block_index,
                                 uint32_t *offset) {
  assert(0 != inode_index);
  bgdt_t block_group;
  get_group_descriptor((inode_index - 1) / superblock->num_inodes_in_group,
                       &block_group);

  uint64_t full_offset =
      ((inode_index - 1) % superblock->num_inodes_in_group) * inode_size;
  *block_index = block_group.starting_inode_table +
                 (full_offset >> (superblock->block_size + 10));
  *offset = full_offset & (block_byte_size - 1);
}

int ext2_last_inode_read = -1;
inode_t ext2_last_inode;

void ext2_get_inode_header(int inode_index, inode_t *data) {
  // Very simple cache. If the inode_index is a inode already read then
  // just copy the old data.
  if (ext2_last_inode_read == inode_index) {
    memcpy(data, &ext2_last_inode, sizeof(inode_t));
    return;
  }
  uint32_t block_index;
  uint32_t block_offset;
  ext2_block_containing_inode(inode_index, &block_index, &block_offset);

  uint8_t mem_block[inode_size];
  ext2_read_block(block_index, mem_block, inode_size, block_offset);

  memcpy(data, mem_block, inode_size);
  memcpy(&ext2_last_inode, mem_block, sizeof(inode_t));
  ext2_last_inode_read = inode_index;
}

void ext2_write_inode(int inode_index, inode_t *data) {
  if (ext2_last_inode_read == inode_index)
    ext2_last_inode_read = -1; // Invalidate the cache
  uint32_t block_index;
  uint32_t block_offset;
  ext2_block_containing_inode(inode_index, &block_index, &block_offset);

  uint8_t mem_block[inode_size];
  memcpy(mem_block, data, inode_size);
  ext2_write_block(block_index, mem_block, inode_size, block_offset);
}

int ext2_get_inode_in_directory(int dir_inode, char *file,
                                direntry_header_t *entry) {
  // FIXME: Allocate sufficent size each time
  unsigned char *data = kmalloc(block_byte_size * 5);
  ASSERT_BUT_FIXME_PROPOGATE(
      -1 != read_inode(dir_inode, data, block_byte_size * 5, 0, 0));

  direntry_header_t *dir;
  unsigned char *data_p = data;
  for (; (dir = (direntry_header_t *)data_p)->inode; data_p += dir->size) {
    if (0 == dir->size)
      break;
    if (0 == dir->name_length)
      continue;
    if (0 ==
        memcmp(data_p + sizeof(direntry_header_t), file, dir->name_length)) {
      if (strlen(file) > dir->name_length)
        continue;
      if (entry)
        memcpy(entry, data_p, sizeof(direntry_header_t));
      return dir->inode;
    }
  }
  return 0;
}

int ext2_read_dir(int dir_inode, unsigned char *buffer, size_t len,
                  size_t offset) {
  unsigned char data[block_byte_size];
  read_inode(dir_inode, data, block_byte_size, 0, 0);

  direntry_header_t *dir;
  struct dirent tmp_entry;
  size_t n_dir = 0;
  int rc = 0;
  unsigned char *data_p = data;
  for (; (dir = (direntry_header_t *)data_p)->inode && len > 0;
       data_p += dir->size, n_dir++) {
    if (0 == dir->size)
      break;
    if (0 == dir->name_length)
      continue;
    if (n_dir < (offset / sizeof(struct dirent)))
      continue;

    memcpy(tmp_entry.d_name, data_p + sizeof(direntry_header_t),
           dir->name_length);
    tmp_entry.d_name[dir->name_length] = '\0';
    uint8_t *p = (uint8_t *)&tmp_entry;
    size_t l = sizeof(struct dirent);

    l = (len < l) ? len : l;
    memcpy(buffer, p, l);
    len -= l;
    rc += l;
  }
  return rc;
}

uint32_t ext2_find_inode(const char *file) {
  int cur_path_inode = EXT2_ROOT_INODE;

  if (*file == '/' && *(file + 1) == '\0')
    return cur_path_inode;

  char *str = copy_and_allocate_string(file);
  char *orig_str = str;

  char *start;
  for (;;) {
    int final = 0;
    start = str + 1;
    str++;

    for (; '/' != *str && '\0' != *str; str++)
      ;
    if ('\0' == *str)
      final = 1;

    *str = '\0';

    direntry_header_t a;
    if (0 == (cur_path_inode =
                  ext2_get_inode_in_directory(cur_path_inode, start, &a))) {
      kfree(orig_str);
      return 0;
    }

    if (final)
      break;

    // The expected returned entry is a directory
    if (TYPE_INDICATOR_DIRECTORY != a.type_indicator) {
      kfree(orig_str);
      kprintf("FAILED\n");
      return 0;
    }
  }
  kfree(orig_str);
  return cur_path_inode;
}

uint32_t get_singly_block_index(uint32_t singly_block_ptr, uint32_t i) {
  uint8_t block[block_byte_size];
  ext2_read_block(singly_block_ptr, block, block_byte_size, 0);
  uint32_t index = *(uint32_t *)(block + (i * (32 / 8)));
  return index;
}

int get_block(inode_t *inode, uint32_t i) {
  if (i < 12)
    return inode->block_pointers[i];

  i -= 12;
  uint32_t singly_block_size = block_byte_size / (32 / 8);
  uint32_t double_block_size = (singly_block_size * singly_block_size);
  if (i < singly_block_size) {
    return get_singly_block_index(inode->single_indirect_block_pointer, i);
  } else if (i < double_block_size) {
    i -= singly_block_size;
    uint32_t singly_entry = get_singly_block_index(
        inode->double_indirect_block_pointer, i / singly_block_size);
    uint32_t offset_in_entry = i % singly_block_size;
    int block = get_singly_block_index(singly_entry, offset_in_entry);
    return block;
  }
  assert(0);
  return 0;
}

int get_free_block(int allocate) {
  bgdt_t block_group;
  uint8_t bitmap[BLOCK_SIZE];
  assert(0 < superblock->num_blocks_unallocated);
  for (uint32_t g = 0; g < num_block_groups(); g++) {
    get_group_descriptor(g, &block_group);

    if (block_group.num_unallocated_blocks_in_group == 0) {
      kprintf("skip\n");
      continue;
    }

    ext2_read_block(block_group.block_usage_bitmap, bitmap, BLOCK_SIZE, 0);
    for (uint32_t i = 0; i < superblock->num_blocks_in_group; i++) {
      if (!(bitmap[i >> 3] & (1 << (i % 8)))) {
        if (allocate) {
          bitmap[i >> 3] |= (1 << (i % 8));
          ext2_write_block(block_group.block_usage_bitmap, bitmap, BLOCK_SIZE,
                           0);
          block_group.num_unallocated_blocks_in_group--;
          write_group_descriptor(g, &block_group);
          superblock->num_blocks_unallocated--;
          write_lba(EXT2_SUPERBLOCK_SECTOR, (void *)superblock, 2 * SECTOR_SIZE,
                    0);
        }
        return i + g * superblock->num_blocks_in_group + 1;
      }
    }
  }
  return -1;
}

int get_free_inode(int allocate) {
  bgdt_t block_group;
  assert(0 < superblock->num_inodes_unallocated);
  for (uint32_t g = 0; g < num_block_groups(); g++) {
    get_group_descriptor(g, &block_group);

    if (0 == block_group.num_unallocated_inodes_in_group)
      continue;

    uint8_t bitmap[BLOCK_SIZE];
    ext2_read_block(block_group.inode_usage_bitmap, bitmap, BLOCK_SIZE, 0);
    for (uint32_t i = 0; i < superblock->num_inodes_in_group; i++) {
      if (!(bitmap[i / 8] & (1 << (i % 8)))) {
        if (allocate) {
          bitmap[i / 8] |= (1 << (i % 8));
          ext2_write_block(block_group.inode_usage_bitmap, bitmap, BLOCK_SIZE,
                           0);
          block_group.num_unallocated_inodes_in_group--;
          write_group_descriptor(g, &block_group);
          superblock->num_inodes_unallocated--;
          write_lba(EXT2_SUPERBLOCK_SECTOR, (void *)superblock, 2 * SECTOR_SIZE,
                    0);
        }
        return i + g * superblock->num_inodes_in_group + 1;
      }
    }
  }
  return -1;
}

int write_inode(int inode_num, unsigned char *data, uint64_t size,
                uint64_t offset, uint64_t *file_size, int append) {
  (void)file_size;
  uint8_t inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, (inode_t *)inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  uint64_t fsize = (uint64_t)(((uint64_t)inode->_upper_32size << 32) |
                              (uint64_t)inode->low_32size);
  if (append)
    offset = fsize;

  uint32_t block_start = offset / block_byte_size;
  uint32_t block_offset = offset % block_byte_size;

  int num_blocks_used = inode->num_disk_sectors / (BLOCK_SIZE / SECTOR_SIZE);

  if (size + offset > fsize)
    fsize = size + offset;

  int num_blocks_required = BLOCKS_REQUIRED(fsize, BLOCK_SIZE);

  for (int i = num_blocks_used; i < num_blocks_required; i++) {
    if (i > 12)
      assert(0);
    int b = get_free_block(1 /*true*/);
    assert(-1 != b);
    inode->block_pointers[i] = b;
  }

  inode->num_disk_sectors = num_blocks_required * (BLOCK_SIZE / SECTOR_SIZE);

  int bytes_written = 0;
  for (int i = block_start; size; i++) {
    uint32_t block = get_block(inode, i);
    if (0 == block) {
      kprintf("block_not_found\n");
      break;
    }

    int write_len = ((size + block_offset) > block_byte_size)
                        ? (block_byte_size - block_offset)
                        : size;
    ext2_write_block(block, data + bytes_written, write_len, block_offset);
    block_offset = 0;
    bytes_written += write_len;
    size -= write_len;
  }
  inode->low_32size = fsize;
  inode->_upper_32size = (fsize >> 32);
  ext2_write_inode(inode_num, inode);
  return bytes_written;
}

int read_inode(int inode_num, unsigned char *data, uint64_t size,
               uint64_t offset, uint64_t *file_size) {
  // TODO: Fail if size is lower than the size of the file being read, and
  //       return the size of the file the callers is trying to read.
  uint8_t inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, (inode_t *)inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  uint64_t fsize = (uint64_t)(((uint64_t)inode->_upper_32size << 32) |
                              (uint64_t)inode->low_32size);

  if (file_size)
    *file_size = fsize;

  if (size > fsize - offset)
    size -= ((size + offset) - fsize);

  if (size == 0)
    return 0;

  if (offset > fsize)
    return 0;

  uint32_t block_start = offset / block_byte_size;
  uint32_t block_offset = offset % block_byte_size;

  int bytes_read = 0;
  for (int i = block_start; size; i++) {
    uint32_t block = get_block(inode, i);
    if (0 == block) {
      klog("Filesystem EXT2: Unable to find block", LOG_WARN);
      return -1;
    }

    int read_len = ((size + block_offset) > block_byte_size)
                       ? (block_byte_size - block_offset)
                       : size;
    ext2_read_block(block, data + bytes_read, read_len, block_offset);
    block_offset = 0;
    bytes_read += read_len;
    size -= read_len;
  }
  return bytes_read;
}

size_t ext2_read_file_offset(const char *file, unsigned char *data,
                             uint64_t size, uint64_t offset,
                             uint64_t *file_size) {
  // TODO: Fail if the file does not exist.
  uint32_t inode = ext2_find_inode(file);
  return read_inode(inode, data, size, offset, file_size);
}

size_t ext2_read_file(const char *file, unsigned char *data, size_t size,
                      uint64_t *file_size) {
  return ext2_read_file_offset(file, data, size, 0, file_size);
}

int resolve_link(int inode_num) {
  uint8_t tmp[inode_size];
  inode_t *inode = (inode_t *)tmp;
  uint64_t inode_size =
      (((uint64_t)inode->_upper_32size) << 32) & inode->low_32size;
  assert(inode_size <= 60);
  ext2_get_inode_header(inode_num, inode);
  char *path = (char *)(tmp + (10 * 4));
  path--;
  *path = '/';
  return ext2_find_inode(path);
}

int ext2_write(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  uint64_t file_size;
  int rc;
  int inode_num = fd->inode->inode_num;
  assert(fd->inode->type != FS_TYPE_DIRECTORY);
  if (fd->inode->type == FS_TYPE_LINK) {
    inode_num = resolve_link(inode_num);
  }
  rc = write_inode(inode_num, buffer, len, offset, &file_size, 0);
  return rc;
}

int ext2_read(uint8_t *buffer, uint64_t offset, uint64_t len, vfs_fd_t *fd) {
  uint64_t file_size;
  int rc;
  int inode_num = fd->inode->inode_num;
  if (fd->inode->type == FS_TYPE_DIRECTORY) {
    rc = ext2_read_dir(inode_num, buffer, len, offset);
    return rc;
  }
  if (fd->inode->type == FS_TYPE_LINK) {
    inode_num = resolve_link(inode_num);
  }
  rc = read_inode(inode_num, buffer, len, offset, &file_size);
  return rc;
}

vfs_inode_t *ext2_open(const char *path) {
  uint32_t inode_num = ext2_find_inode(path);
  if (0 == inode_num)
    return NULL;

  inode_t ext2_inode[inode_size];
  ext2_get_inode_header(inode_num, ext2_inode);
  uint64_t file_size =
      ((uint64_t)(ext2_inode->_upper_32size) << 32) | ext2_inode->low_32size;

  uint8_t type;
  switch ((ext2_inode->types_permissions / 0x1000)) {
  case 0xA:
    type = FS_TYPE_LINK;
    break;
  case 0x4:
    type = FS_TYPE_DIRECTORY;
    break;
  default:
    type = FS_TYPE_FILE;
    break;
  }

  return vfs_create_inode(inode_num, type, 1 /*has_data*/, 1 /*can_write*/,
                          1 /*is_open*/, NULL /*internal_object*/, file_size,
                          ext2_open, ext2_create_file, ext2_read, ext2_write,
                          ext2_close, ext2_create_directory,
                          NULL /*get_vm_object*/);
}

uint64_t end_of_last_entry_position(int dir_inode, uint64_t *entry_offset,
                                    direntry_header_t *meta) {
  // FIXME: Allocate sufficent size each time
  unsigned char data[block_byte_size * 5];
  uint64_t file_size = 0;
  read_inode(dir_inode, data, block_byte_size * 5, 0, &file_size);
  assert(block_byte_size * 5 > file_size);

  direntry_header_t *dir;
  unsigned char *data_p = data;
  uint64_t pos = 0;
  uint64_t prev = pos;
  for (; pos < file_size && (dir = (direntry_header_t *)data_p)->size;
       data_p += dir->size, prev = pos, pos += dir->size)
    ;
  if (entry_offset)
    *entry_offset = prev;
  if (meta)
    memcpy(meta, ((char *)data) + prev, sizeof(direntry_header_t));
  return pos;
}

void ext2_create_entry(int directory_inode, direntry_header_t entry_header,
                       const char *name) {
  uint64_t entry_offset = 0;
  direntry_header_t meta;
  end_of_last_entry_position(directory_inode, &entry_offset, &meta);

  uint32_t padding_in_use = block_byte_size - entry_offset;

  //  assert(padding_in_use == meta.size);
  assert(padding_in_use >=
         (sizeof(direntry_header_t) + entry_header.name_length));

  // Modify the entry to have its real size
  meta.size = sizeof(direntry_header_t) + meta.name_length;
  meta.size += (4 - (meta.size % 4));
  write_inode(directory_inode, (unsigned char *)&meta,
              sizeof(direntry_header_t), entry_offset, NULL, 0);

  // Create new entry
  uint32_t new_entry_offset = entry_offset + meta.size;
  entry_header.size = (sizeof(direntry_header_t) + entry_header.name_length);
  entry_header.size += (4 - (entry_header.size % 4));

  uint32_t length_till_next_block = 1024 - (new_entry_offset % 1024);
  if (0 == length_till_next_block)
    length_till_next_block = 1024;
  assert(entry_header.size < length_till_next_block);
  entry_header.size = length_till_next_block;

  uint8_t buffer[entry_header.size];
  memset(buffer, 0, entry_header.size);
  memcpy(buffer, &entry_header, sizeof(entry_header));
  memcpy(buffer + sizeof(entry_header), name, entry_header.name_length);
  write_inode(directory_inode, (unsigned char *)buffer, entry_header.size,
              new_entry_offset, NULL, 0);
}

int ext2_find_parent(char *path, uint32_t *parent_inode, char **filename) {
  char *e = path;
  for (; *e; e++)
    ;
  for (; *e != '/'; e--)
    ;
  *e = '\0';
  *filename = e + 1;
  if (*path == '\0') {
    *parent_inode = EXT2_ROOT_INODE;
    return 1;
  } else {
    int r = ext2_find_inode(path);
    if (0 == r)
      return 0;
    *parent_inode = r;
    return 1;
  }
  return 0;
}

int ext2_create_directory(const char *path, int mode) {
  (void)mode;
  // Check if the directory already exists
  uint32_t inode_num = ext2_find_inode(path);
  if (0 != inode_num) {
    klog("ext2_create_directory: Directory already exists", LOG_WARN);
    return inode_num;
  }

  uint32_t parent_inode;
  // Get the parent directory
  char path_buffer[strlen(path) + 1];
  char *filename;
  strcpy(path_buffer, path);
  if (!ext2_find_parent(path_buffer, &parent_inode, &filename)) {
    klog("ext2_create_file: Parent does not exist", LOG_WARN);
    return -1;
  }

  int new_file_inode = get_free_inode(1);
  if (-1 == new_file_inode) {
    klog("ext2_create_file: Unable to find free inode", LOG_WARN);
    return -1;
  }
  assert(0 != new_file_inode);

  direntry_header_t entry_header;
  entry_header.inode = new_file_inode;
  entry_header.name_length = strlen(filename);
  entry_header.type_indicator = TYPE_INDICATOR_DIRECTORY;
  entry_header.size = sizeof(entry_header) + entry_header.name_length;

  ext2_create_entry(parent_inode, entry_header, filename);
  // Create the inode header
  uint8_t inode_buffer[inode_size];
  inode_t *new_inode = (inode_t *)inode_buffer;
  memset(inode_buffer, 0, inode_size);
  new_inode->types_permissions = DIRECTORY;
  new_inode->num_hard_links = 2; // 2 since the directory references
                                 // itself with the "." entry
  ext2_write_inode(new_file_inode, new_inode);

  // Populate the new directory with "." and ".."
  {
    // "."
    direntry_header_t child_entry_header;
    child_entry_header.inode = new_file_inode;
    child_entry_header.name_length = 1;
    child_entry_header.type_indicator = TYPE_INDICATOR_DIRECTORY;
    child_entry_header.size = sizeof(entry_header) + entry_header.name_length;
    ext2_create_entry(new_file_inode, child_entry_header, ".");
    // ".."
    child_entry_header.inode = parent_inode;
    child_entry_header.name_length = 2;
    child_entry_header.type_indicator = TYPE_INDICATOR_DIRECTORY;
    child_entry_header.size = sizeof(entry_header) + entry_header.name_length;
    ext2_create_entry(new_file_inode, child_entry_header, "..");
  }
  return new_file_inode;
}

int ext2_create_file(const char *path, int mode) {
  // Check if the file already exists
  uint32_t inode_num = ext2_find_inode(path);
  if (0 != inode_num) {
    klog("ext2_create_file: File already exists", LOG_WARN);
    return inode_num;
  }

  uint32_t parent_inode;
  // Get the parent directory
  char path_buffer[strlen(path) + 1];
  char *filename;
  strcpy(path_buffer, path);
  if (!ext2_find_parent(path_buffer, &parent_inode, &filename)) {
    klog("ext2_create_file: Parent does not exist", LOG_WARN);
    return -1;
  }

  int new_file_inode = get_free_inode(1);
  if (-1 == new_file_inode) {
    klog("ext2_create_file: Unable to find free inode", LOG_WARN);
    return -1;
  }
  assert(0 != new_file_inode);

  direntry_header_t entry_header;
  entry_header.inode = new_file_inode;
  entry_header.name_length = strlen(filename);
  entry_header.type_indicator = TYPE_INDICATOR_REGULAR;
  entry_header.size = sizeof(entry_header) + entry_header.name_length;

  ext2_create_entry(parent_inode, entry_header, filename);
  // Create the inode header
  uint8_t inode_buffer[inode_size];
  inode_t *new_inode = (inode_t *)inode_buffer;
  memset(inode_buffer, 0, inode_size);
  new_inode->types_permissions = 0x8000;
  new_inode->num_hard_links = 1;
  ext2_write_inode(new_file_inode, new_inode);
  return new_file_inode;
}

vfs_inode_t *ext2_mount(void) {
  parse_superblock();
  return vfs_create_inode(0 /*inode_num*/, 0 /*type*/, 0 /*has_data*/,
                          0 /*can_write*/, 0 /*is_open*/,
                          NULL /*internal_object*/, 0 /*file_size*/, ext2_open,
                          ext2_create_file, ext2_read, ext2_write, ext2_close,
                          ext2_create_directory, NULL /*get_vm_object*/);
}

void parse_superblock(void) {
  superblock = ksbrk(2 * SECTOR_SIZE);
  read_lba(EXT2_SUPERBLOCK_SECTOR, (void *)superblock, 2 * SECTOR_SIZE, 0);
  block_byte_size = 1024 << superblock->block_size;

  if (0xEF53 != superblock->ext2_signature) {
    klog("Incorrect ext2 signature in superblock.", LOG_ERROR);
    for (;;)
      ; // TODO: Fail properly
  }

  if (1 <= superblock->major_version)
    inode_size = ((ext_superblock_t *)superblock)->inode_size;

  inodes_per_block = block_byte_size / inode_size;
}
