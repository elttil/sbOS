#include <assert.h>
#include <drivers/pit.h>
#include <errno.h>
#include <fcntl.h>
#include <fs/ext2.h>
#include <fs/vfs.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>
#include <timer.h>
#include <typedefs.h>

#define EXT2_SUPERBLOCK_SECTOR 2
#define EXT2_ROOT_INODE 2

#define BLOCKS_REQUIRED(_a, _b) ((_a) / (_b) + (((_a) % (_b)) != 0))

#define ALIGN(value, alignment)                                                \
  if (0 != (value % alignment)) {                                              \
    value += (alignment - (value % alignment));                                \
  }

struct ext2_open_file {
  u32 inode_num;
  u32 references;
  int is_removed;
  struct ext2_open_file *next;
  struct ext2_open_file *prev;
};

void remove_inode(int inode_num);
int ext2_unlink(const char *path);

struct ext2_open_file *files_head = NULL;

int add_open_inode(u32 inode_num) {
  struct ext2_open_file *ptr = files_head;
  for (; ptr; ptr = ptr->next) {
    if (inode_num == ptr->inode_num) {
      break;
    }
  }
  if (!ptr) {
    ptr = kmalloc(sizeof(struct ext2_open_file));
    if (!ptr) {
      return 0;
    }
    ptr->next = files_head;
    ptr->prev = NULL;
    if (ptr->next) {
      ptr->prev = ptr->next->prev;
      ptr->next->prev = ptr;
      if (ptr->prev) {
        ptr->prev->next = ptr;
      }
    }
    ptr->inode_num = inode_num;
    ptr->references = 0;
    ptr->is_removed = 0;
    files_head = ptr;
  }
  ptr->references++;
  return 1;
}

void remove_possibly_open_inode(u32 inode_num) {
  struct ext2_open_file *ptr = files_head;
  for (; ptr; ptr = ptr->next) {
    if (inode_num == ptr->inode_num) {
      break;
    }
  }
  if (!ptr) {
    remove_inode(inode_num);
    return;
  }
  ptr->is_removed = 1;
}

int deref_open_inode(u32 inode_num) {
  struct ext2_open_file *ptr = files_head;
  for (; ptr; ptr = ptr->next) {
    if (inode_num == ptr->inode_num) {
      break;
    }
  }
  if (!ptr) {
    return 1;
  }
  assert(ptr->references > 0);
  ptr->references--;
  if (0 != ptr->references) {
    return 0;
  }

  if (ptr->prev) {
    ptr->prev->next = ptr->next;
  }
  if (ptr->next) {
    ptr->next->prev = ptr->prev;
  }
  if (files_head == ptr) {
    assert(!ptr->prev);
    files_head = ptr->next;
  }
  if (ptr->is_removed) {
    remove_inode(inode_num);
  }
  kfree(ptr);
  return 1;
}

superblock_t *superblock;
u32 block_byte_size;
u32 inode_size;
u32 inodes_per_block;
int superblock_changed = 0;

vfs_fd_t *mount_fd = NULL;

void ext2_close(vfs_fd_t *fd) {
  deref_open_inode(fd->inode->inode_num);
  return; // There is nothing to clear
}

int read_inode(int inode_num, u8 *data, u64 size, u64 offset, u64 *file_size);

void get_inode_data_size(int inode_num, u64 *file_size) {
  read_inode(inode_num, NULL, 0, 0, file_size);
}

struct block_cache {
  int is_used;
  u32 last_use;
  u32 block_num;
  u8 *block;
  u8 has_write;
};

size_t num_block_cache = 4;
struct block_cache *cache;

u32 cold_cache_hits = 0;

void cached_read_block(u32 block, void *address, size_t size, size_t offset) {
  assert(offset + size <= block_byte_size);
  int free_found = -1;
  for (size_t i = 0; i < num_block_cache; i++) {
    if (!cache[i].is_used) {
      free_found = i;
      continue;
    }
    if (cache[i].block_num == block) {
      cache[i].last_use = timer_get_uptime();
      memcpy(address, cache[i].block + offset, size);
      return;
    }
  }

  if (-1 == free_found) {
    u32 min_last_used = U32_MAX;
    int min_index = 0;
    for (size_t i = 0; i < num_block_cache; i++) {
      if (cache[i].last_use < min_last_used) {
        min_last_used = cache[i].last_use;
        min_index = i;
      }
    }
    free_found = min_index;
  }

  struct block_cache *c = &cache[free_found];
  if (c->is_used && c->has_write) {
    raw_vfs_pwrite(mount_fd, c->block, block_byte_size,
                   c->block_num * block_byte_size);
  }
  c->is_used = 1;
  c->block_num = block;
  c->last_use = timer_get_uptime();
  c->has_write = 0;
  raw_vfs_pread(mount_fd, c->block, block_byte_size, block * block_byte_size);
  cached_read_block(block, address, size, offset);
}

void ext2_read_block(u32 block, void *address, size_t size, size_t offset) {
  cached_read_block(block, address, size, offset);
}

void ext2_flush_writes(void) {
  for (size_t i = 0; i < num_block_cache; i++) {
    if (!cache[i].is_used) {
      continue;
    }
    if (!cache[i].has_write) {
      continue;
    }
    raw_vfs_pwrite(mount_fd, cache[i].block, block_byte_size,
                   cache[i].block_num * block_byte_size);
    cache[i].has_write = 0;
  }
  if (superblock_changed) {
    raw_vfs_pwrite(mount_fd, superblock, 2 * SECTOR_SIZE, 0);
    superblock_changed = 0;
  }
}

void ext2_write_block(u32 block, u8 *address, size_t size, size_t offset) {
  assert(offset + size <= block_byte_size);
  int cache_index = -1;
  for (size_t i = 0; i < num_block_cache; i++) {
    if (!cache[i].is_used) {
      continue;
    }
    if (cache[i].block_num == block) {
      cache_index = i;
      break;
    }
  }
  if (-1 != cache_index) {
    memcpy(cache[cache_index].block + offset, address, size);
    cache[cache_index].has_write = 1;
    return;
  }
  raw_vfs_pwrite(mount_fd, address, size, block * block_byte_size + offset);
}

void write_group_descriptor(u32 group_index, bgdt_t *block_group) {
  int starting_block = (1024 == block_byte_size) ? 2 : 1;
  ext2_write_block(starting_block, (u8 *)block_group, sizeof(bgdt_t),
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
  if (num_blocks % num_blocks_in_group != 0) {
    b++;
  }

  // Rounding up the total number of inodes divided by the number of
  // inodes per block group
  u32 num_inodes = superblock->num_inodes;
  u32 num_inodes_in_group = superblock->num_inodes_in_group;
  u32 i = num_inodes / num_inodes_in_group;
  if (num_inodes % num_inodes_in_group != 0) {
    i++;
  }
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

void ext2_get_inode_header(int inode_index, u8 *data) {
  memset(data + sizeof(inode_t), 0, inode_size - sizeof(inode_t));
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
  if (ext2_last_inode_read == inode_index) {
    ext2_last_inode_read = -1; // Invalidate the cache
  }
  u32 block_index;
  u32 block_offset;
  ext2_block_containing_inode(inode_index, &block_index, &block_offset);

  u8 mem_block[inode_size];
  memcpy(mem_block, data, inode_size);
  ext2_write_block(block_index, mem_block, inode_size, block_offset);
}

int ext2_get_inode_in_directory(int dir_inode, struct sv file,
                                direntry_header_t *entry) {
  if (sv_isempty(file)) {
    return dir_inode;
  }
  u64 file_size;
  ASSERT_BUT_FIXME_PROPOGATE(-1 !=
                             read_inode(dir_inode, NULL, 0, 0, &file_size));
  u64 allocation_size = file_size;
  u8 data[allocation_size];
  ASSERT_BUT_FIXME_PROPOGATE(
      -1 != read_inode(dir_inode, data, allocation_size, 0, NULL));

  direntry_header_t *dir;
  u8 *data_p = data;
  u8 *data_end = data + allocation_size;
  for (; data_p <= (data_end - sizeof(direntry_header_t)) &&
         (dir = (direntry_header_t *)data_p)->inode;
       data_p += dir->size) {
    if (0 == dir->size) {
      break;
    }
    if (0 == dir->name_length) {
      continue;
    }
    if (sv_length(file) != dir->name_length) {
      continue;
    }
    assert(data_p + sizeof(direntry_header_t) + dir->name_length <= data_end);
    if (0 == memcmp(data_p + sizeof(direntry_header_t), sv_buffer(file),
                    dir->name_length)) {
      if (entry) {
        memcpy(entry, data_p, sizeof(direntry_header_t));
      }
      return dir->inode;
    }
  }
  return 0;
}

int ext2_read_dir(int dir_inode, u8 *buffer, size_t len, size_t offset) {
  u64 file_size;
  get_inode_data_size(dir_inode, &file_size);
  u8 data[file_size];
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
    if (0 == dir->size) {
      break;
    }
    if (0 == dir->name_length) {
      continue;
    }
    if (n_dir < (offset / sizeof(struct dirent))) {
      continue;
    }

    memcpy(tmp_entry.d_name, data_p + sizeof(direntry_header_t),
           dir->name_length);
    tmp_entry.d_name[dir->name_length] = '\0';
    u8 *p = (u8 *)&tmp_entry;
    size_t l = sizeof(struct dirent);

    l = min(len, l);
    memcpy(buffer + rc, p, l);
    len -= l;
    rc += l;
  }
  return rc;
}

u32 ext2_find_inode(const char *file, u32 *parent_directory) {
  int cur_path_inode = EXT2_ROOT_INODE;

  if (*file == '/' && *(file + 1) == '\0') {
    if (parent_directory) {
      *parent_directory = EXT2_ROOT_INODE;
    }
    return cur_path_inode;
  }

  struct sv p = C_TO_SV(file);
  sv_try_eat(p, &p, C_TO_SV("/"));
  for (;;) {
    int final = 0;
    struct sv start = sv_split_delim(p, &p, '/');
    if (sv_isempty(p)) {
      final = 1;
    }

    if (parent_directory) {
      *parent_directory = cur_path_inode;
    }

    direntry_header_t a;
    if (0 == (cur_path_inode =
                  ext2_get_inode_in_directory(cur_path_inode, start, &a))) {
      return 0;
    }

    if (final) {
      break;
    }

    // The expected returned entry is a directory
    if (TYPE_INDICATOR_DIRECTORY != a.type_indicator) {
      klog(LOG_WARN, "ext2: Expected diretory but got: %d", a.type_indicator);
      return 0;
    }
  }
  return cur_path_inode;
}

u32 get_singly_block_index(u32 singly_block_ptr, u32 i) {
  u8 block[block_byte_size];
  ext2_read_block(singly_block_ptr, block, block_byte_size, 0);
  u32 index = *(u32 *)(block + (i * (32 / 8)));
  return index;
}

int get_from_all_blocks(inode_t *inode, u32 i) {
  if (i < 12) {
    return inode->block_pointers[i];
  }

  i -= 12;
  u32 singly_block_byte_size = block_byte_size / (32 / 8);
  u32 double_block_byte_size =
      (singly_block_byte_size * singly_block_byte_size);

  if (0 == i) {
    return inode->single_indirect_block_pointer;
  }
  i--;

  if (i < singly_block_byte_size) {
    return get_singly_block_index(inode->single_indirect_block_pointer, i);
  } else if (i < double_block_byte_size) {
    i -= singly_block_byte_size;
    if (0 == i) {
      return inode->double_indirect_block_pointer;
    }
    i--;

    u32 singly_entry = get_singly_block_index(
        inode->double_indirect_block_pointer, i / singly_block_byte_size);
    u32 offset_in_entry = i % singly_block_byte_size;

    if (0 == offset_in_entry) {
      return singly_entry;
    }
    offset_in_entry--;

    return get_singly_block_index(singly_entry, offset_in_entry);
  }
  assert(0);
  return 0;
}

int get_block(inode_t *inode, u32 i) {
  if (i < 12) {
    return inode->block_pointers[i];
  }

  i -= 12;
  u32 singly_block_byte_size = block_byte_size / (32 / 8);
  u32 double_block_byte_size =
      (singly_block_byte_size * singly_block_byte_size);
  if (i < singly_block_byte_size) {
    return get_singly_block_index(inode->single_indirect_block_pointer, i);
  } else if (i < double_block_byte_size) {
    i -= singly_block_byte_size;
    u32 singly_entry = get_singly_block_index(
        inode->double_indirect_block_pointer, i / singly_block_byte_size);
    u32 offset_in_entry = i % singly_block_byte_size;
    int block = get_singly_block_index(singly_entry, offset_in_entry);
    return block;
  }
  assert(0);
  return 0;
}

int get_free_blocks(int allocate, int entries[], u32 num_entries) {
  u32 current_entry = 0;
  bgdt_t block_group;
  if (num_entries > superblock->num_blocks_unallocated) {
    return 0;
  }
  assert(0 == superblock->num_blocks_in_group % 8);
  for (u32 group = 0; group < num_block_groups() && current_entry < num_entries;
       group++) {
    get_group_descriptor(group, &block_group);

    if (0 == block_group.num_unallocated_blocks_in_group) {
      continue;
    }

    u8 bitmap[(superblock->num_blocks_in_group) / 8];
    ext2_read_block(block_group.block_usage_bitmap, bitmap,
                    (superblock->num_blocks_in_group) / 8, 0);
    int found_block = 0;
    for (u32 index = 0; index < superblock->num_blocks_in_group / 8 &&
                        current_entry < num_entries;
         index++) {
      if (0xFF == bitmap[index]) {
        continue;
      }
      for (u32 offset = 0; offset < 8 && current_entry < num_entries;
           offset++) {
        if (bitmap[index] & (1 << offset)) {
          continue;
        }
        u32 block_index =
            index * 8 + offset + group * superblock->num_blocks_in_group;
        bitmap[index] |= (1 << offset);
        entries[current_entry] = block_index;
        current_entry++;
        found_block = 1;
      }
    }
    if (allocate && found_block) {
      ext2_write_block(block_group.block_usage_bitmap, bitmap,
                       superblock->num_blocks_in_group / 8, 0);
      block_group.num_unallocated_blocks_in_group--;
      write_group_descriptor(group, &block_group);
      superblock->num_blocks_unallocated--;
      superblock_changed = 1;
    }
  }
  return current_entry;
}

int get_free_block(int allocate) {
  int entry[1];
  if (0 == get_free_blocks(allocate, entry, 1)) {
    return -1;
  }
  return entry[0];
}

void write_to_inode_bitmap(u32 inode_num, int value) {
  u32 group_index = inode_num / superblock->num_inodes_in_group;
  u32 index = inode_num % superblock->num_inodes_in_group;

  bgdt_t block_group;
  get_group_descriptor(group_index, &block_group);

  u8 bitmap[(superblock->num_inodes_in_group) / 8];
  ext2_read_block(block_group.inode_usage_bitmap, bitmap,
                  (superblock->num_inodes_in_group) / 8, 0);

  if (0 == value) {
    bitmap[index / 8] &= ~(1 << (index % 8));
    superblock->num_inodes_unallocated++;
    block_group.num_unallocated_inodes_in_group++;
  } else {
    bitmap[index / 8] |= (1 << (index % 8));
    superblock->num_inodes_unallocated--;
    block_group.num_unallocated_inodes_in_group--;
  }
  superblock_changed = 1;
  ext2_write_block(block_group.inode_usage_bitmap, bitmap,
                   superblock->num_inodes_in_group / 8, 0);
  write_group_descriptor(group_index, &block_group);
}

void write_to_block_bitmap(u32 block_num, int value) {
  u32 group_index = block_num / superblock->num_blocks_in_group;
  u32 index = block_num % superblock->num_blocks_in_group;

  bgdt_t block_group;
  get_group_descriptor(group_index, &block_group);

  u8 bitmap[(superblock->num_blocks_in_group) / 8];
  ext2_read_block(block_group.block_usage_bitmap, bitmap,
                  (superblock->num_blocks_in_group) / 8, 0);

  if (0 == value) {
    bitmap[index / 8] &= ~(1 << (index % 8));
    superblock->num_blocks_unallocated++;
    block_group.num_unallocated_blocks_in_group++;
  } else {
    bitmap[index / 8] |= (1 << (index % 8));
    superblock->num_blocks_unallocated--;
    block_group.num_unallocated_blocks_in_group--;
  }
  ext2_write_block(block_group.block_usage_bitmap, bitmap,
                   superblock->num_blocks_in_group / 8, 0);
  write_group_descriptor(group_index, &block_group);
  superblock_changed = 1;
}

int get_free_inode(int allocate) {
  bgdt_t block_group;
  if (0 == superblock->num_inodes_unallocated) {
    return -1;
  }
  assert(0 == superblock->num_inodes_in_group % 8);
  u8 bitmap[(superblock->num_inodes_in_group) / 8];
  for (u32 group = 0; group < num_block_groups(); group++) {
    get_group_descriptor(group, &block_group);

    if (0 == block_group.num_unallocated_inodes_in_group) {
      continue;
    }

    ext2_read_block(block_group.inode_usage_bitmap, bitmap,
                    (superblock->num_inodes_in_group) / 8, 0);
    for (u32 index = 0; index < superblock->num_inodes_in_group / 8; index++) {
      if (0xFF == bitmap[index]) {
        continue;
      }
      for (u32 offset = 0; offset < 8; offset++) {
        if (bitmap[index] & (1 << offset)) {
          continue;
        }
        u32 inode_index = index * 8 + offset +
                          group * superblock->num_inodes_in_group +
                          1 /* inodes are 1 addressed */;
        if (allocate) {
          bitmap[index] |= (1 << offset);
          ext2_write_block(block_group.inode_usage_bitmap, bitmap,
                           superblock->num_inodes_in_group / 8, 0);
          block_group.num_unallocated_inodes_in_group--;
          write_group_descriptor(group, &block_group);
          superblock->num_inodes_unallocated--;
          superblock_changed = 1;
          raw_vfs_pwrite(mount_fd, superblock, 2 * SECTOR_SIZE, 0);
        }
        return inode_index;
      }
    }
  }
  return -1;
}

#define INDIRECT_BLOCK_CAPACITY (block_byte_size / sizeof(u32))

void write_to_indirect_block(u32 indirect_block, u32 index, u32 new_block) {
  index %= INDIRECT_BLOCK_CAPACITY;
  ext2_write_block(indirect_block, (u8 *)&new_block, sizeof(u32),
                   index * sizeof(u32));
}

int ext2_allocate_block(inode_t *inode, u32 index, int block) {
  if (index < 12) {
    inode->block_pointers[index] = block;
    return 1;
  }
  index -= 12;
  if (index < INDIRECT_BLOCK_CAPACITY) {
    if (0 == index) {
      int n = get_free_block(1 /*true*/);
      if (-1 == n) {
        return 0;
      }
      inode->single_indirect_block_pointer = n;
    }
    write_to_indirect_block(inode->single_indirect_block_pointer, index, block);
    return 1;
  }
  index -= INDIRECT_BLOCK_CAPACITY;
  if (index < INDIRECT_BLOCK_CAPACITY * INDIRECT_BLOCK_CAPACITY) {
    if (0 == index) {
      int n = get_free_block(1 /*true*/);
      if (-1 == n) {
        return 0;
      }
      inode->double_indirect_block_pointer = n;
    }

    u32 value;
    if (0 == (index % INDIRECT_BLOCK_CAPACITY)) {
      int n = get_free_block(1 /*true*/);
      if (-1 == n) {
        return 0;
      }
      write_to_indirect_block(inode->double_indirect_block_pointer,
                              index / INDIRECT_BLOCK_CAPACITY, n);
      value = n;
    } else {
      value = get_singly_block_index(inode->double_indirect_block_pointer,
                                     index / INDIRECT_BLOCK_CAPACITY);
    }

    write_to_indirect_block(value, index, block);
    return 1;
  }
  return 0;
}

int write_inode(int inode_num, u8 *data, u64 size, u64 offset, u64 *file_size,
                int append) {
  (void)file_size;
  u8 inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  u64 fsize = (u64)(((u64)inode->_upper_32size << 32) | (u64)inode->low_32size);
  if (append) {
    offset = fsize;
  }

  u32 block_start = offset / block_byte_size;
  u32 block_offset = offset % block_byte_size;

  int num_blocks_used =
      inode->num_disk_sectors / (block_byte_size / SECTOR_SIZE);

  if (size + offset > fsize) {
    fsize = size + offset;
  }

  u32 num_blocks_required = BLOCKS_REQUIRED(fsize, block_byte_size);

  u32 delta = num_blocks_required - num_blocks_used;
  if (delta > 0) {
    int blocks[delta];
    get_free_blocks(1, blocks, delta);
    for (u32 i = num_blocks_used; i < num_blocks_required; i++) {
      assert(ext2_allocate_block(inode, i, blocks[i - num_blocks_used]));
    }
  }

  inode->num_disk_sectors =
      num_blocks_required * (block_byte_size / SECTOR_SIZE);

  int bytes_written = 0;
  for (int i = block_start; size; i++) {
    u32 block = get_block(inode, i);
    if (0 == block) {
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
  ext2_flush_writes();
  return bytes_written;
}

int read_inode(int inode_num, u8 *data, u64 size, u64 offset, u64 *file_size) {
  // TODO: Fail if size is lower than the size of the file being read, and
  //       return the size of the file the callers is trying to read.
  u8 inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  u64 fsize = (u64)(((u64)inode->_upper_32size << 32) | (u64)inode->low_32size);

  if (file_size) {
    *file_size = fsize;
  }

  if (size > fsize - offset) {
    size -= ((size + offset) - fsize);
  }

  if (size == 0) {
    return 0;
  }

  if (offset > fsize) {
    return 0;
  }

  u32 block_start = offset / block_byte_size;
  u32 block_offset = offset % block_byte_size;

  int bytes_read = 0;
  for (int i = block_start; size; i++) {
    int read_len = ((size + block_offset) > block_byte_size)
                       ? (block_byte_size - block_offset)
                       : size;

    u32 block = get_block(inode, i);
    if (0 == block) {
      memset(data + bytes_read, 0, read_len);
    } else {
      ext2_read_block(block, data + bytes_read, read_len, block_offset);
    }

    block_offset = 0;
    bytes_read += read_len;
    size -= read_len;
  }
  return bytes_read;
}

size_t ext2_read_file_offset(const char *file, u8 *data, u64 size, u64 offset,
                             u64 *file_size) {
  // TODO: Fail if the file does not exist.
  u32 inode = ext2_find_inode(file, NULL);
  return read_inode(inode, data, size, offset, file_size);
}

size_t ext2_read_file(const char *file, u8 *data, size_t size, u64 *file_size) {
  return ext2_read_file_offset(file, data, size, 0, file_size);
}

int resolve_link(int inode_num) {
  u8 tmp_inode_buffer[inode_size];
  inode_t *inode = (inode_t *)tmp_inode_buffer;
  u64 inode_size = (((u64)inode->_upper_32size) << 32) & inode->low_32size;
  assert(inode_size <= 60);
  ext2_get_inode_header(inode_num, tmp_inode_buffer);
  char *path = (char *)(tmp_inode_buffer + (10 * 4));
  path--;
  *path = '/';
  return ext2_find_inode(path, NULL);
}

int ext2_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  int inode_num = fd->inode->inode_num;
  assert(fd->inode->type != FS_TYPE_DIRECTORY);
  if (fd->inode->type == FS_TYPE_LINK) {
    inode_num = resolve_link(inode_num);
  }
  return write_inode(inode_num, buffer, len, offset, NULL, 0);
}

int ext2_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  int inode_num = fd->inode->inode_num;
  if (fd->inode->type == FS_TYPE_LINK) {
    inode_num = resolve_link(inode_num);
  }
  u8 inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  if (DIRECTORY & inode->types_permissions) {
    return ext2_read_dir(inode_num, buffer, len, offset);
  }

  return read_inode(inode_num, buffer, len, offset, NULL);
}

int ext2_stat(vfs_fd_t *fd, struct stat *buf) {
  u8 buffer[inode_size];
  ext2_get_inode_header(fd->inode->inode_num, buffer);
  inode_t *inode = (inode_t *)buffer;

  buf->st_uid = inode->user_id;
  buf->st_gid = inode->group_id;
  buf->st_size = (u64)inode->low_32size | ((u64)inode->_upper_32size);
  buf->st_mode = inode->types_permissions;
  return 0;
}

int ext2_truncate(vfs_fd_t *fd, size_t length) {
  // TODO: Blocks that are no longer used should be freed.
  u8 inode_buffer[inode_size];
  ext2_get_inode_header(fd->inode->inode_num, inode_buffer);
  inode_t *ext2_inode = (inode_t *)inode_buffer;

  // FIXME: ftruncate should support 64 bit lengths
  ext2_inode->_upper_32size = 0;
  ext2_inode->low_32size = length;

  ext2_write_inode(fd->inode->inode_num, ext2_inode);
  return 0;
}

vfs_inode_t *ext2_open(const char *path) {
  u32 inode_num = ext2_find_inode(path, NULL);
  if (0 == inode_num) {
    return NULL;
  }

  u8 buffer[inode_size];
  ext2_get_inode_header(inode_num, buffer);
  inode_t *ext2_inode = (inode_t *)buffer;
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

  if (!add_open_inode(inode_num)) {
    return NULL;
  }
  vfs_inode_t *inode = vfs_create_inode(
      inode_num, type, NULL, NULL, 1 /*is_open*/, 0, NULL /*internal_object*/,
      file_size, ext2_open, ext2_create_file, ext2_read, ext2_write, ext2_close,
      ext2_create_directory, NULL /*get_vm_object*/, ext2_truncate /*truncate*/,
      ext2_stat, NULL /*connect*/);
  if (!inode) {
    (void)deref_open_inode(inode_num);
    return NULL;
  }
  inode->unlink = ext2_unlink;
  return inode;
}

direntry_header_t *directory_ptr_next(direntry_header_t *ptr, u64 *left) {
  assert(left);
  if (0 == ptr->size) {
    return NULL;
  }
  if (ptr->size > *left) {
    return NULL;
  }
  direntry_header_t *next_entry =
      (direntry_header_t *)((uintptr_t)ptr + ptr->size);
  *left -= ptr->size;
  if (*left < sizeof(direntry_header_t)) {
    return NULL;
  }
  if (0 == next_entry->size) {
    return NULL;
  }
  return next_entry;
}

void remove_inode(int inode_num) {
  u8 inode_buffer[inode_size];
  ext2_get_inode_header(inode_num, inode_buffer);
  inode_t *inode = (inode_t *)inode_buffer;

  if (0 != inode->num_hard_links) {
    inode->num_hard_links--;
  }

  if (0 != inode->num_hard_links) {
    return;
  }

  int num_blocks_used =
      inode->num_disk_sectors / (block_byte_size / SECTOR_SIZE);

  for (int i = 0; i < num_blocks_used; i++) {
    u32 block = get_from_all_blocks(inode, i);
    if (0 == block) {
      continue;
    }
    write_to_block_bitmap(block, 0);
  }

  inode->deletion_time = timer_get_ms() / 1000;
  inode->num_disk_sectors = 0;

  ext2_write_inode(inode_num, inode);
  write_to_inode_bitmap(inode_num - 1, 0);
}

u64 find_free_position(int dir_inode, u64 *entry_offset,
                       direntry_header_t *meta, size_t entry_size,
                       size_t *entry_padding) {
  u64 file_size;
  get_inode_data_size(dir_inode, &file_size);
  u8 *data = kmalloc(file_size);
  read_inode(dir_inode, data, file_size, 0, NULL);

  direntry_header_t *dir;
  u8 *data_p = data;
  u64 pos = 0;
  u64 prev = pos;

  if (entry_padding) {
    *entry_padding = 0;
  }
  for (; pos < file_size && (dir = (direntry_header_t *)data_p)->size;
       data_p += dir->size, prev = pos, pos += dir->size) {
    u64 real_size = sizeof(direntry_header_t) + dir->name_length;
    ALIGN(real_size, 4);
    u64 padding = dir->size - real_size;
    if (padding >= entry_size) {
      prev = pos;
      if (entry_padding) {
        *entry_padding = padding;
      }
      break;
    }
  }

  if (entry_offset) {
    *entry_offset = prev;
  }
  if (meta) {
    memcpy(meta, ((u8 *)data) + prev, sizeof(direntry_header_t));
  }
  kfree(data);
  return pos;
}

void ext2_create_entry(int directory_inode, direntry_header_t entry_header,
                       const char *name) {
  entry_header.size = (sizeof(direntry_header_t) + entry_header.name_length);
  ALIGN(entry_header.size, 4);

  u64 entry_offset = 0;
  direntry_header_t meta;
  size_t entry_padding;
  find_free_position(directory_inode, &entry_offset, &meta, entry_header.size,
                     &entry_padding);
  if (0 == entry_padding) {
    entry_padding = block_byte_size;
  }
  entry_header.size = entry_padding;

  // Modify the entry to have its real size
  meta.size = sizeof(direntry_header_t) + meta.name_length;
  ALIGN(meta.size, 4);
  write_inode(directory_inode, (u8 *)&meta, sizeof(direntry_header_t),
              entry_offset, NULL, 0);

  // Create new entry
  u32 new_entry_offset = entry_offset + meta.size;

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
    int r = ext2_find_inode(path, NULL);
    if (0 == r) {
      return 0;
    }
    *parent_inode = r;
    return 1;
  }
  return 0;
}

int ext2_create_directory(const char *path, int mode) {
  (void)mode;
  // Check if the directory already exists
  u32 inode_num = ext2_find_inode(path, NULL);
  if (0 != inode_num) {
    klog(LOG_WARN, "ext2_create_directory: Directory already exists");
    return inode_num;
  }

  u32 parent_inode;
  // Get the parent directory
  char path_buffer[strlen(path) + 1];
  char *filename;
  strcpy(path_buffer, path);
  if (!ext2_find_parent(path_buffer, &parent_inode, &filename)) {
    klog(LOG_WARN, "ext2_create_file: Parent does not exist");
    return -1;
  }

  int new_file_inode = get_free_inode(1);
  if (-1 == new_file_inode) {
    klog(LOG_WARN, "ext2_create_file: Unable to find free inode");
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
  u32 inode_num = ext2_find_inode(path, NULL);
  if (0 != inode_num) {
    klog(LOG_WARN, "ext2_create_file: File already exists");
    return inode_num;
  }

  u32 parent_inode;
  // Get the parent directory
  char path_buffer[strlen(path) + 1];
  char *filename;
  strcpy(path_buffer, path);
  if (!ext2_find_parent(path_buffer, &parent_inode, &filename)) {
    klog(LOG_WARN, "ext2_create_file: Parent does not exist");
    return -1;
  }

  int new_file_inode = get_free_inode(1);
  if (-1 == new_file_inode) {
    klog(LOG_WARN, "ext2_create_file: Unable to find free inode");
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

int ext2_unlink(const char *path) {
  u32 parent_directory;
  u32 inode_num = ext2_find_inode(path, &parent_directory);
  if (0 == inode_num) {
    return -ENOENT;
  }
  if (EXT2_ROOT_INODE == inode_num) {
    return -EPERM;
  }

  u64 file_size;
  get_inode_data_size(parent_directory, &file_size);
  u8 *data = kmalloc(file_size);
  read_inode(parent_directory, data, file_size, 0, NULL);

  u64 left = file_size;
  direntry_header_t *current = (direntry_header_t *)data;

  for (;;) {
    if (inode_num == current->inode) {
      kfree(data);
      // This would be "." which can't be removed anyways
      return -ENOENT;
    }
    direntry_header_t *next = directory_ptr_next(current, &left);
    if (!next) {
      kfree(data);
      return -ENOENT;
    }
    if (inode_num == next->inode) {
      current->size += next->size;
      // Not required but done for testing purposes
      memset(next, 0, next->size);
      break;
    }
    current = next;
  }
  write_inode(parent_directory, data, file_size, 0, NULL, 0);
  kfree(data);

  remove_possibly_open_inode(inode_num);
  ext2_flush_writes();
  return 0;
}

vfs_inode_t *ext2_mount(void) {
  int fd = vfs_open("/dev/sda", O_RDWR, 0);
  if (0 > fd) {
    return NULL;
  }
  // TODO: Can this be done better? Maybe create a seperate function in
  // the VFS?
  mount_fd = get_vfs_fd(fd, NULL);
  // Remove the FD from the current task
  // FIXME: This is a hacky solution
  relist_remove(&current_task->file_descriptors, fd);
  parse_superblock();

  for (size_t i = 0; i < num_block_cache; i++) {
    cache[i].block = kmalloc(block_byte_size);
    if (!cache[i].block) {
      goto ext2_mount_error;
    }
  }
  vfs_inode_t *inode = vfs_create_inode(
      0 /*inode_num*/, 0 /*type*/, 0 /*has_data*/, 0 /*can_write*/,
      0 /*is_open*/, 0, NULL /*internal_object*/, 0 /*file_size*/, ext2_open,
      ext2_create_file, ext2_read, ext2_write, ext2_close,
      ext2_create_directory, NULL /*get_vm_object*/, ext2_truncate /*truncate*/,
      ext2_stat, NULL /*connect*/);
  if (!inode) {
    goto ext2_mount_error;
  }
  inode->unlink = ext2_unlink;
  return inode;
ext2_mount_error:
  vfs_close(fd);
  for (size_t i = 0; i < num_block_cache; i++) {
    kfree(cache[i].block);
  }
  return NULL;
}

void parse_superblock(void) {
  superblock = kmalloc(2 * SECTOR_SIZE);
  raw_vfs_pread(mount_fd, superblock, 2 * SECTOR_SIZE,
                EXT2_SUPERBLOCK_SECTOR * SECTOR_SIZE);

  block_byte_size = 1024 << superblock->block_size;

  if (0xEF53 != superblock->ext2_signature) {
    klog(LOG_ERROR, "Incorrect ext2 signature in superblock.");
    for (;;)
      ; // TODO: Fail properly
  }

  if (1 <= superblock->major_version) {
    inode_size = ((ext_superblock_t *)superblock)->inode_size;
  }

  inodes_per_block = block_byte_size / inode_size;

  cache = kcalloc(num_block_cache, sizeof(struct block_cache));
}
