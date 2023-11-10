#ifndef EXT2_H
#define EXT2_H
#include <drivers/ata.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <typedefs.h>

typedef struct Superblock {
  u32 num_inodes;
  u32 num_blocks;
  u32 num_blocks_reserved;
  u32 num_blocks_unallocated;
  u32 num_inodes_unallocated;
  u32 superblock_block_num;
  u32 block_size;
  u32 fragment_size;
  u32 num_blocks_in_group;
  u32 num_fragments_in_group;
  u32 num_inodes_in_group;
  u32 last_mount;
  u32 last_write;
  u16 num_mounts_since_fsck;
  u16 num_mounts_allowed;
  u16 ext2_signature; // 0xEF53
  u16 fs_state;
  u16 when_error;
  u16 minor_version;
  u32 last_fsck;
  u32 interval_fsck;
  u32 os_id;
  u32 major_version;
  u16 userid_reserved_blocks;
  u16 groupid_reserved_blocks;
} __attribute__((packed)) superblock_t;

typedef struct ExtendedSuperblock {
  u32 num_inodes;
  u32 num_blocks;
  u32 num_blocks_reserved;
  u32 num_blocks_unallocated;
  u32 num_inodes_unallocated;
  u32 superblock_block_num;
  u32 block_size;
  u32 fragment_size;
  u32 num_blocks_group;
  u32 num_fragments_group;
  u32 num_inodes_group;
  u32 last_mount;
  u32 last_write;
  u16 num_mounts_since_fsck;
  u16 num_mounts_allowed;
  u16 ext2_signature; // 0xEF53
  u16 fs_state;
  u16 when_error;
  u16 minor_version;
  u32 last_fsck;
  u32 interval_fsck;
  u32 os_id;
  u32 major_version;
  u16 userid_reserved_blocks;
  u16 groupid_reserved_blocks;
  u32 pad;
  u16 inode_size;
} __attribute__((packed)) ext_superblock_t;

typedef struct BlockGroupDescriptorTable {
  u32 block_usage_bitmap;
  u32 inode_usage_bitmap;
  u32 starting_inode_table;
  u16 num_unallocated_blocks_in_group;
  u16 num_unallocated_inodes_in_group;
  u16 num_directories_group;
} __attribute__((packed)) bgdt_t;

typedef struct INode {
  u16 types_permissions;
  u16 user_id;
  u32 low_32size;
  u32 last_access_time;
  u32 creation_time;
  u32 last_modification_time;
  u32 deletion_time;
  u16 group_id;
  u16 num_hard_links;
  u32 num_disk_sectors;
  u32 flags;
  u32 os_specific;
  u32 block_pointers[12];
  u32 single_indirect_block_pointer;
  u32 double_indirect_block_pointer;
  u32 triple_indirect_block_pointer;
  u32 gen_number;
  u32 _extended_attribute_block;
  u32 _upper_32size;
  u32 address_fragment;
  u32 os_specific2;
} __attribute__((packed)) inode_t;

// 0    Unknown type
// 1    Regular file
// 2    Directory
// 3    Character device
// 4    Block device
// 5    FIFO
// 6    Socket
// 7    Symbolic link (soft link)
#define TYPE_INDICATOR_UNKOWN 0
#define TYPE_INDICATOR_REGULAR 1
#define TYPE_INDICATOR_DIRECTORY 2
#define TYPE_INDICATOR_CHARACTER_DEVICE 3
#define TYPE_INDICATOR_BLOCK_DEVICE 4
#define TYPE_INDICATOR_FIFO 5
#define TYPE_INDICATOR_SOCKET 6
#define TYPE_INDICATOR_SOFT_LINK 7

#define FIFO 0x1000
#define CHARACTER_DEVICE 0x2000
#define DIRECTORY 0x4000
#define BLOCK_DEVICE 0x6000
#define REGULAR_FILE 0x8000
#define SYMBOLIC_LINK 0xA000
#define UNIX_SOCKET 0xC000

typedef struct DirectoryEntryHeader {
  u32 inode;
  u16 size;
  u8 name_length;
  u8 type_indicator;
} __attribute__((packed)) direntry_header_t;

int ext2_create_file(const char *path, int mode);
vfs_inode_t *ext2_mount(void);
void parse_superblock(void);
size_t ext2_read_file_offset(const char *file, u8 *data, u64 size, u64 offset,
                             u64 *file_size);
size_t ext2_read_file(const char *file, u8 *data, size_t size, u64 *file_size);
int ext2_create_directory(const char *path, int mode);
#endif
