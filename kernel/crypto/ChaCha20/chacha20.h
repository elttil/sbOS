#ifndef CHACHA20_H
#define CHACHA20_H
#include <typedefs.h>

#define KEY 4
#define KEY_SIZE 8 * sizeof(u32)
#define COUNT 12
#define COUNT_SIZE sizeof(u32)
#define COUNT_MAX (0x100000000 - 1) // 2^32 - 1
#define NONCE 13
#define NONCE_SIZE 2 * sizeof(u32)
#define BLOCK_SIZE 16 * sizeof(u32)

void chacha_block(u32 out[16], u32 const in[16]);
#endif
