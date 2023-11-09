#ifndef EXT2_H
#define EXT2_H
#include <drivers/ata.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <stdint.h>

typedef struct Superblock {
  uint32_t num_inodes;
  uint32_t num_blocks;
  uint32_t num_blocks_reserved;
  uint32_t num_blocks_unallocated;
  uint32_t num_inodes_unallocated;
  uint32_t superblock_block_num;
  uint32_t block_size;
  uint32_t fragment_size;
  uint32_t num_blocks_in_group;
  uint32_t num_fragments_in_group;
  uint32_t num_inodes_in_group;
  uint32_t last_mount;
  uint32_t last_write;
  uint16_t num_mounts_since_fsck;
  uint16_t num_mounts_allowed;
  uint16_t ext2_signature; // 0xEF53
  uint16_t fs_state;
  uint16_t when_error;
  uint16_t minor_version;
  uint32_t last_fsck;
  uint32_t interval_fsck;
  uint32_t os_id;
  uint32_t major_version;
  uint16_t userid_reserved_blocks;
  uint16_t groupid_reserved_blocks;
} __attribute__((packed)) superblock_t;

typedef struct ExtendedSuperblock {
  uint32_t num_inodes;
  uint32_t num_blocks;
  uint32_t num_blocks_reserved;
  uint32_t num_blocks_unallocated;
  uint32_t num_inodes_unallocated;
  uint32_t superblock_block_num;
  uint32_t block_size;
  uint32_t fragment_size;
  uint32_t num_blocks_group;
  uint32_t num_fragments_group;
  uint32_t num_inodes_group;
  uint32_t last_mount;
  uint32_t last_write;
  uint16_t num_mounts_since_fsck;
  uint16_t num_mounts_allowed;
  uint16_t ext2_signature; // 0xEF53
  uint16_t fs_state;
  uint16_t when_error;
  uint16_t minor_version;
  uint32_t last_fsck;
  uint32_t interval_fsck;
  uint32_t os_id;
  uint32_t major_version;
  uint16_t userid_reserved_blocks;
  uint16_t groupid_reserved_blocks;
  uint32_t pad;
  uint16_t inode_size;
} __attribute__((packed)) ext_superblock_t;

typedef struct BlockGroupDescriptorTable {
  uint32_t block_usage_bitmap;
  uint32_t inode_usage_bitmap;
  uint32_t starting_inode_table;
  uint16_t num_unallocated_blocks_in_group;
  uint16_t num_unallocated_inodes_in_group;
  uint16_t num_directories_group;
} __attribute__((packed)) bgdt_t;

typedef struct INode {
  uint16_t types_permissions;
  uint16_t user_id;
  uint32_t low_32size;
  uint32_t last_access_time;
  uint32_t creation_time;
  uint32_t last_modification_time;
  uint32_t deletion_time;
  uint16_t group_id;
  uint16_t num_hard_links;
  uint32_t num_disk_sectors;
  uint32_t flags;
  uint32_t os_specific;
  uint32_t block_pointers[12];
  uint32_t single_indirect_block_pointer;
  uint32_t double_indirect_block_pointer;
  uint32_t triple_indirect_block_pointer;
  uint32_t gen_number;
  uint32_t _extended_attribute_block;
  uint32_t _upper_32size;
  uint32_t address_fragment;
  uint32_t os_specific2;
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
  uint32_t inode;
  uint16_t size;
  uint8_t name_length;
  uint8_t type_indicator;
} __attribute__((packed)) direntry_header_t;

int ext2_create_file(const char *path, int mode);
vfs_inode_t *ext2_mount(void);
void parse_superblock(void);
size_t ext2_read_file_offset(const char *file, char *data, uint64_t size,
                             uint64_t offset, uint64_t *file_size);
size_t ext2_read_file(const char *file, char *data, size_t size,
                      uint64_t *file_size);
int ext2_create_directory(const char *path, int mode);
#endif
