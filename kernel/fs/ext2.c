#include <assert.h>
#include <fs/ext2.h>
#include <fs/vfs.h>
#include <string.h>
#include <typedefs.h>

#define EXT2_SUPERBLOCK_SECTOR 2
#define EXT2_ROOT_INODE 2

#define BLOCKS_REQUIRED(_a, _b) ((_a) / (_b) + (((_a) % (_b)) != 0))

superblock_t *superblock;
u32 block_byte_size;
u32 inode_size;
u32 inodes_per_block;

#define BLOCK_SIZE (block_byte_size)

void ext2_close(vfs_fd_t *fd) {
  return; // There is nothing to clear
}

int read_inode(int inode_num, u8 *data, u64 size, u64 offset, u64 *file_size);

inline void get_inode_data_size(int inode_num, u64 *file_size) {
  read_inode(inode_num, NULL, 0, 0, file_size);
}

struct BLOCK_CACHE {
  u32 block_num;
  u8 block[1024];
};

#define NUM_BLOCK_CACHE 30
struct BLOCK_CACHE cache[NUM_BLOCK_CACHE] = {0};
u8 last_taken_cache = 0;

void cached_read_block(u32 block, void *address, size_t size, size_t offset) {
  int free_found = -1;
  for (int i = 0; i < NUM_BLOCK_CACHE; i++) {
    if (cache[i].block_num == block) {
      memcpy(address, cache[i].block + offset, size);
      return;
    }
    if (0 == cache[i].block_num)
      free_found = i;
  }

  if (-1 == free_found) {
    free_found = last_taken_cache;
    last_taken_cache++;
    if (last_taken_cache >= NUM_BLOCK_CACHE)
      last_taken_cache = 0;
  }

  struct BLOCK_CACHE *c = &cache[free_found];
  c->block_num = block;
  read_lba(block * block_byte_size / 512, c->block, 1024, 0);
  cached_read_block(block, address, size, offset);
}

void ext2_read_block(u32 block, void *address, size_t size, size_t offset) {
  cached_read_block(block, address, size, offset);
}

void ext2_write_block(u32 block, void *address, size_t size, size_t offset) {
  // Invalidate a old cache
  for (int i = 0; i < NUM_BLOCK_CACHE; i++) {
    if (cache[i].block_num == block) {
      cache[i].block_num = 0;
      break;
    }
  }
  write_lba(block * block_byte_size / 512, address, size, offset);
}

void write_group_descriptor(u32 group_index, bgdt_t *block_group) {
  int starting_block = (1024 == block_byte_size) ? 2 : 1;
  ext2_write_block(starting_block, block_group, sizeof(bgdt_t),
                   group_index * sizeof(bgdt_t));
}

void get_group_descriptor(u32 group_index, bgdt_t *block_group) {
  int starting_block = (1024 == block_byte_size) ? 2 : 1;
  ext2_read_block(starting_block, block_group, sizeof(bgdt_t),
                  group_index * sizeof(bgdt_t));
}

u32 num_block_groups(void) {
  // Determining the Number of Block Groups

  // From the Superblock, extract the size of each block, the total
  // number of inodes, the total number of blocks, the number of blocks
  // per block group, and the number of inodes in each block group. From
  // this information we can infer the number of block groups there are
  // by:

  // Rounding up the total number of blocks divided by the number of
  // blocks per block group
  u32 num_blocks = superblock->num_blocks;
  u32 num_blocks_in_group = superblock->num_blocks_in_group;
  u32 b = num_blocks / num_blocks_in_group;
  if (num_blocks % num_blocks_in_group != 0)
    b++;

  // Rounding up the total number of inodes divided by the number of
  // inodes per block group
  u32 num_inodes = superblock->num_inodes;
  u32 num_inodes_in_group = superblock->num_inodes_in_group;
  u32 i = num_inodes / num_inodes_in_group;
  if (num_inodes % num_inodes_in_group != 0)
    i++;
  // Both (and check them against each other)
  assert(i == b);
  return i;
}

void ext2_block_containing_inode(u32 inode_index, u32 *block_index,
                                 u32 *offset) {
  assert(0 != inode_index);
  bgdt_t block_group;
  get_group_descriptor((inode_index - 1) / superblock->num_inodes_in_group,
                       &block_group);

  u64 full_offset =
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
  u32 block_index;
  u32 block_offset;
  ext2_block_containing_inode(inode_index, &block_index, &block_offset);

  u8 mem_block[inode_size];
  ext2_read_block(block_index, mem_block, inode_size, block_offset);

  memcpy(data, mem_block, inode_size);
  memcpy(&ext2_last_inode, mem_block, sizeof(inode_t));
  ext2_last_inode_read = inode_index;
}

void ext2_write_inode(int inode_index, inode_t *data) {
  if (ext2_last_inode_read == inode_index)
    ext2_last_inode_read = -1; // Invalidate the cache
  u32 block_index;
  u32 block_offset;
  ext2_block_containing_inode(inode_index, &block_index, &block_offset);

  u8 mem_block[inode_size];
  memcpy(mem_block, data, inode_size);
  ext2_write_block(block_index, mem_block, inode_size, block_offset);
}

int ext2_get_inode_in_directory(int dir_inode, char *file,
                                direntry_header_t *entry) {
  u64 file_size;
  ASSERT_BUT_FIXME_PROPOGATE(-1 !=
                             read_inode(dir_inode, NULL, 0, 0, &file_size));
  u64 allocation_size = file_size;
  u8 *data = kmalloc(allocation_size);
  ASSERT_BUT_FIXME_PROPOGATE(
      -1 != read_inode(dir_inode, data, allocation_size, 0, NULL));

  direntry_header_t *dir;
  u8 *data_p = data;
  u8 *data_end = data + allocation_size;
  for (; data_p <= (data_end - sizeof(direntry_header_t)) &&
         (dir = (direntry_header_t *)data_p)->inode;
       data_p += dir->size) {
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

int ext2_read_dir(int dir_inode, u8 *buffer, size_t len, size_t offset) {
  u64 file_size;
  get_inode_data_size(dir_inode, &file_size);
  u8 *data = kmalloc(file_size);
  read_inode(dir_inode, data, file_size, 0, NULL);

  direntry_header_t *dir;
  struct dirent tmp_entry;
  size_t n_dir = 0;
  int rc = 0;
  u8 *data_p = data;
  u8 *data_end = data + file_size;
  for (; data_p <= (data_end - sizeof(direntry_header_t)) &&
         (dir = (direntry_header_t *)data_p)->inode && len > 0;
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
    u8 *p = (u8 *)&tmp_entry;
    size_t l = sizeof(struct dirent);

    l = (len < l) ? len : l;
    memcpy(buffer, p, l);
    len -= l;
    rc += l;
  }
  kfree(data);
  return rc;
}

u32 ext2_find_inode(const char *file) {
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

u32 get_singly_block_index(u32 singly_block_ptr, u32 i) {
  u8 block[block_byte_size];
  ext2_read_block(singly_block_ptr, block, block_byte_size, 0);
  u32 index = *(u32 *)(block + (i * (32 / 8)));
  return index;
}

int get_block(inode_t *inode, u32 i) {
  if (i < 12)
    return inode->block_pointers[i];

  i -= 12;
  u32 singly_block_size = block_byte_size / (32 / 8);
  u32 double_block_size = (singly_block_size * singly_block_size);
  if (i < singly_block_size) {
    return get_singly_block_index(inode->single_indirect_block_pointer, i);
  } else if (i < double_block_size) {
    i -= singly_block_size;
    u32 singly_entry = get_singly_block_index(
        inode->double_indirect_block_pointer, i / singly_block_size);
    u32 offset_in_entry = i % singly_block_size;
    int block = get_singly_block_index(singly_entry, offset_in_entry);
    return block;
  }
  assert(0);
  return 0;
}

int get_free_block(int allocate) {
  bgdt_t block_group;
  u8 bitmap[BLOCK_SIZE];
  assert(0 < superblock->num_blocks_unallocated);
  for (u32 g = 0; g < num_block_groups(); g++) {
    get_group_descriptor(g, &block_group);

    if (block_group.num_unallocated_blocks_in_group == 0) {
      kprintf("skip\n");
      continue;
    }

    ext2_read_block(block_group.block_usage_bitmap, bitmap, BLOCK_SIZE, 0);
    for (u32 i = 0; i < superblock->num_blocks_in_group; i++) {
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
  for (u32 g = 0; g < num_block_groups(); g++) {
    get_group_descriptor(g, &block_group);

    if (0 == block_group.num_unallocated_inodes_in_group)
      continue;

    u8 bitmap[BLOCK_SIZE];
    ext2_read_block(block_group.inode_usage_bitmap, bitmap, BLOCK_SIZE, 0);
    for (u32 i = 0; i < superblock->num_inodes_in_group; i++) {
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

int write_inode(int inode_num, u8 *data, u64 size, u64 offset, u64 *file_size,
                int append) {
  (void)file_size;
  u8 inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, (inode_t *)inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  u64 fsize = (u64)(((u64)inode->_upper_32size << 32) | (u64)inode->low_32size);
  if (append)
    offset = fsize;

  u32 block_start = offset / block_byte_size;
  u32 block_offset = offset % block_byte_size;

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
    u32 block = get_block(inode, i);
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

int read_inode(int inode_num, u8 *data, u64 size, u64 offset, u64 *file_size) {
  // TODO: Fail if size is lower than the size of the file being read, and
  //       return the size of the file the callers is trying to read.
  u8 inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, (inode_t *)inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  u64 fsize = (u64)(((u64)inode->_upper_32size << 32) | (u64)inode->low_32size);

  if (file_size)
    *file_size = fsize;

  if (size > fsize - offset)
    size -= ((size + offset) - fsize);

  if (size == 0)
    return 0;

  if (offset > fsize)
    return 0;

  u32 block_start = offset / block_byte_size;
  u32 block_offset = offset % block_byte_size;

  int bytes_read = 0;
  for (int i = block_start; size; i++) {
    u32 block = get_block(inode, i);
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

size_t ext2_read_file_offset(const char *file, u8 *data, u64 size, u64 offset,
                             u64 *file_size) {
  // TODO: Fail if the file does not exist.
  u32 inode = ext2_find_inode(file);
  return read_inode(inode, data, size, offset, file_size);
}

size_t ext2_read_file(const char *file, u8 *data, size_t size, u64 *file_size) {
  return ext2_read_file_offset(file, data, size, 0, file_size);
}

int resolve_link(int inode_num) {
  u8 tmp[inode_size];
  inode_t *inode = (inode_t *)tmp;
  u64 inode_size = (((u64)inode->_upper_32size) << 32) & inode->low_32size;
  assert(inode_size <= 60);
  ext2_get_inode_header(inode_num, inode);
  char *path = (char *)(tmp + (10 * 4));
  path--;
  *path = '/';
  return ext2_find_inode(path);
}

int ext2_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  u64 file_size;
  int rc;
  int inode_num = fd->inode->inode_num;
  assert(fd->inode->type != FS_TYPE_DIRECTORY);
  if (fd->inode->type == FS_TYPE_LINK) {
    inode_num = resolve_link(inode_num);
  }
  rc = write_inode(inode_num, buffer, len, offset, &file_size, 0);
  return rc;
}

int ext2_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  u64 file_size;
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

int ext2_truncate(vfs_fd_t *fd, size_t length) {
  // TODO: Blocks that are no longer used should be freed.
  u8 inode_buffer[inode_size];
  inode_t *ext2_inode = (inode_t *)inode_buffer;

  ext2_get_inode_header(fd->inode->inode_num, ext2_inode);

  // FIXME: ftruncate should support 64 bit lengths
  ext2_inode->_upper_32size = 0;
  ext2_inode->low_32size = length;

  ext2_write_inode(fd->inode->inode_num, ext2_inode);
  return 0;
}

vfs_inode_t *ext2_open(const char *path) {
  u32 inode_num = ext2_find_inode(path);
  if (0 == inode_num)
    return NULL;

  inode_t ext2_inode[inode_size];
  ext2_get_inode_header(inode_num, ext2_inode);
  u64 file_size =
      ((u64)(ext2_inode->_upper_32size) << 32) | ext2_inode->low_32size;

  u8 type;
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
                          NULL /*get_vm_object*/, ext2_truncate /*truncate*/);
}

u64 end_of_last_entry_position(int dir_inode, u64 *entry_offset,
                               direntry_header_t *meta) {
  u64 file_size;
  get_inode_data_size(dir_inode, &file_size);
  u8 *data = kmalloc(file_size);
  read_inode(dir_inode, data, file_size, 0, NULL);

  direntry_header_t *dir;
  u8 *data_p = data;
  u64 pos = 0;
  u64 prev = pos;
  for (; pos < file_size && (dir = (direntry_header_t *)data_p)->size;
       data_p += dir->size, prev = pos, pos += dir->size)
    ;
  if (entry_offset)
    *entry_offset = prev;
  if (meta)
    memcpy(meta, ((u8 *)data) + prev, sizeof(direntry_header_t));
  kfree(data);
  return pos;
}

void ext2_create_entry(int directory_inode, direntry_header_t entry_header,
                       const char *name) {
  u64 entry_offset = 0;
  direntry_header_t meta;
  end_of_last_entry_position(directory_inode, &entry_offset, &meta);

  u32 padding_in_use = block_byte_size - entry_offset;

  //  assert(padding_in_use == meta.size);
  assert(padding_in_use >=
         (sizeof(direntry_header_t) + entry_header.name_length));

  // Modify the entry to have its real size
  meta.size = sizeof(direntry_header_t) + meta.name_length;
  meta.size += (4 - (meta.size % 4));
  write_inode(directory_inode, (u8 *)&meta, sizeof(direntry_header_t),
              entry_offset, NULL, 0);

  // Create new entry
  u32 new_entry_offset = entry_offset + meta.size;
  entry_header.size = (sizeof(direntry_header_t) + entry_header.name_length);
  entry_header.size += (4 - (entry_header.size % 4));

  u32 length_till_next_block = 1024 - (new_entry_offset % 1024);
  if (0 == length_till_next_block)
    length_till_next_block = 1024;
  assert(entry_header.size < length_till_next_block);
  entry_header.size = length_till_next_block;

  u8 buffer[entry_header.size];
  memset(buffer, 0, entry_header.size);
  memcpy(buffer, &entry_header, sizeof(entry_header));
  memcpy(buffer + sizeof(entry_header), name, entry_header.name_length);
  write_inode(directory_inode, (u8 *)buffer, entry_header.size,
              new_entry_offset, NULL, 0);
}

int ext2_find_parent(char *path, u32 *parent_inode, char **filename) {
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
  u32 inode_num = ext2_find_inode(path);
  if (0 != inode_num) {
    klog("ext2_create_directory: Directory already exists", LOG_WARN);
    return inode_num;
  }

  u32 parent_inode;
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
  u8 inode_buffer[inode_size];
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
  u32 inode_num = ext2_find_inode(path);
  if (0 != inode_num) {
    klog("ext2_create_file: File already exists", LOG_WARN);
    return inode_num;
  }

  u32 parent_inode;
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
  u8 inode_buffer[inode_size];
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
                          ext2_create_directory, NULL /*get_vm_object*/,
                          ext2_truncate /*truncate*/);
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
