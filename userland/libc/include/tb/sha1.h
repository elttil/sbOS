//
// Copyright (C) 2022-2023 by Anton Kling <anton@kling.gg>
//
// SPDX-License-Identifier: 0BSD
//
#ifndef SHA1
#define SHA1
#include <stddef.h>
#include <stdint.h>

#define BLOCK_BYTES (64) /* 512/8 */
#define SHA1_LEN (20)

typedef struct SHA1_CTX {
  uint32_t h[5];
  uint8_t block[BLOCK_BYTES];
  uint64_t active_len;
  uint64_t len;
} SHA1_CTX;

void SHA1_Init(SHA1_CTX *ctx);
void SHA1_Update(SHA1_CTX *ctx, const void *data, uint64_t len);
void SHA1_Final(SHA1_CTX *ctx, unsigned char *message_digest);
void SHA1_HMAC(unsigned char *message, uint64_t message_len, unsigned char *key,
               uint64_t key_len, uint8_t output[SHA1_LEN]);
#endif
