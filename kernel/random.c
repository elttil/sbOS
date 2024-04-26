// FIXME: This is mostlikely incredibly inefficent and insecure.
#include <assert.h>
#include <crypto/ChaCha20/chacha20.h>
#include <crypto/SHA1/sha1.h>
#include <crypto/xoshiro256plusplus/xoshiro256plusplus.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <random.h>
#include <stddef.h>
#include <string.h>

#define HASH_CTX SHA1_CTX
#define HASH_LEN SHA1_LEN

u32 internal_chacha_block[16] = {
    // Constant ascii values of "expand 32-byte k"
    0x61707865,
    0x3320646e,
    0x79622d32,
    0x6b206574,
    // The unique key
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    // Block counter
    0x00000000,
    // Nonce
    0x00000000,
    0x00000000,
};

void mix_chacha(void) {
  u8 rand_data[BLOCK_SIZE];
  get_random((BYTEPTR)rand_data, BLOCK_SIZE);
  memcpy(internal_chacha_block + KEY, rand_data, KEY_SIZE);
  memcpy(internal_chacha_block + NONCE, rand_data + KEY_SIZE, NONCE_SIZE);
  internal_chacha_block[COUNT] = 0;
}

void get_fast_insecure_random(u8 *buffer, u64 len) {
  static u8 is_fast_random_seeded = 0;
  if (!is_fast_random_seeded) {
    uint64_t seed[4];
    get_random((u8 *)&seed, sizeof(seed));
    seed_xoshiro_256_pp(seed);
    is_fast_random_seeded = 1;
  }
  for (; len >= 8; len -= 8, buffer += 8) {
    *((uint64_t *)buffer) = xoshiro_256_pp();
  }
  for (; len > 0; len--, buffer++) {
    *((uint8_t *)buffer) = xoshiro_256_pp() & 0xFF;
  }
}

void get_random(u8 *buffer, u64 len) {
  u8 rand_data[BLOCK_SIZE];
  for (; len > 0;) {
    if (COUNT_MAX - 1 == internal_chacha_block[COUNT]) {
      // The current block has used up all the 2^32 counts. If the
      // key and/or the nonce are not changed and the count
      // overflows back to zero then the random values would
      // repeate. This is of course not desiered behaviour. The
      // solution is to create a new nonce and key using the
      // already established chacha block.
      internal_chacha_block[COUNT]++;
      mix_chacha();
    }
    u32 read_len = (BLOCK_SIZE < len) ? (BLOCK_SIZE) : len;
    chacha_block((u32 *)rand_data, internal_chacha_block);
    internal_chacha_block[COUNT]++;
    memcpy(buffer, rand_data, read_len);
    buffer += read_len;
    len -= read_len;
  }
}

HASH_CTX hash_pool;
u32 hash_pool_size = 0;

void add_hash_pool(void) {
  u8 new_chacha_key[KEY_SIZE];
  get_random(new_chacha_key, KEY_SIZE);

  u8 hash_buffer[HASH_LEN];
  SHA1_Final(&hash_pool, hash_buffer);
  for (size_t i = 0; i < HASH_LEN; i++) {
    new_chacha_key[i % KEY_SIZE] ^= hash_buffer[i];
  }

  SHA1_Init(&hash_pool);
  SHA1_Update(&hash_pool, hash_buffer, HASH_LEN);

  u8 block[BLOCK_SIZE];
  get_random(block, BLOCK_SIZE);
  SHA1_Update(&hash_pool, block, BLOCK_SIZE);

  memcpy(internal_chacha_block + KEY, new_chacha_key, KEY_SIZE);

  mix_chacha();
}

void add_entropy(u8 *buffer, size_t size) {
  SHA1_Update(&hash_pool, buffer, size);
  hash_pool_size += size;
  if (hash_pool_size >= HASH_LEN * 2) {
    add_hash_pool();
  }
}

void setup_random(void) {
  SHA1_Init(&hash_pool);

  BYTE seed[1024];
  int rand_fd = vfs_open("/etc/seed", O_RDWR, 0);
  if (0 > rand_fd) {
    klog("/etc/seed not found", LOG_WARN);
    return;
  }

  size_t offset = 0;
  for (int rc; (rc = vfs_pread(rand_fd, seed, 1024, offset)); offset += rc) {
    if (0 > rc) {
      klog("/etc/seed read error", LOG_WARN);
      break;
    }
    add_entropy(seed, rc);
  }
  add_hash_pool();

  // Update the /etc/seed file to ensure we get a new state upon next
  // boot.
  get_random(seed, 1024);
  vfs_pwrite(rand_fd, seed, 1024, 0);
  vfs_close(rand_fd);
}

int random_write(BYTEPTR buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;
  add_entropy(buffer, len);
  return len; // add_entropy() never fails to recieve (len) amount of data.
}

int random_read(BYTEPTR buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  (void)fd;
  get_random(buffer, len);
  return len; // get_random() never fails to give "len" amount of data.
}

void add_random_devices(void) {
  devfs_add_file("/random", random_read, random_write, NULL, always_has_data,
                 always_can_write, FS_TYPE_CHAR_DEVICE);
  devfs_add_file("/urandom", random_read, random_write, NULL, always_has_data,
                 always_can_write, FS_TYPE_CHAR_DEVICE);
}
