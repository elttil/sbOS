#ifndef CHACHA20_H
#define CHACHA20_H
#include <stdint.h>

#define KEY 4
#define KEY_SIZE 8*sizeof(uint32_t)
#define COUNT 12
#define COUNT_SIZE sizeof(uint32_t)
#define COUNT_MAX (0x100000000-1) // 2^32 - 1
#define NONCE 13
#define NONCE_SIZE 2*sizeof(uint32_t)
#define BLOCK_SIZE 16*sizeof(uint32_t)

void chacha_block(uint32_t out[16], uint32_t const in[16]);
#endif
