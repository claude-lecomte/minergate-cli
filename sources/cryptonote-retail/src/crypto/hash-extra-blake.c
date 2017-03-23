// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>

#include "common/int-util.h"
#include "hash-ops.h"

static const uint32_t init_state[8] = {
  0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static const uint8_t sigma[112][2] = {
  {0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}, {14, 15},
  {14, 10}, {4, 8}, {9, 15}, {13, 6}, {1, 12}, {0, 2}, {11, 7}, {5, 3},
  {11, 8}, {12, 0}, {5, 2}, {15, 13}, {10, 14}, {3, 6}, {7, 1}, {9, 4},
  {7, 9}, {3, 1}, {13, 12}, {11, 14}, {2, 6}, {5, 10}, {4, 0}, {15, 8},
  {9, 0}, {5, 7}, {2, 4}, {10, 15}, {14, 1}, {11, 12}, {6, 8}, {3, 13},
  {2, 12}, {6, 10}, {0, 11}, {8, 3}, {4, 13}, {7, 5}, {15, 14}, {1, 9},
  {12, 5}, {1, 15}, {14, 13}, {4, 10}, {0, 7}, {6, 3}, {9, 2}, {8, 11},
  {13, 11}, {7, 14}, {12, 1}, {3, 9}, {5, 0}, {15, 4}, {8, 6}, {2, 10},
  {6, 15}, {14, 9}, {11, 3}, {0, 8}, {12, 2}, {13, 7}, {1, 4}, {10, 5},
  {10, 2}, {8, 4}, {7, 6}, {1, 5}, {15, 11}, {9, 14}, {3, 12}, {13, 0},
  {0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}, {10, 11}, {12, 13}, {14, 15},
  {14, 10}, {4, 8}, {9, 15}, {13, 6}, {1, 12}, {0, 2}, {11, 7}, {5, 3},
  {11, 8}, {12, 0}, {5, 2}, {15, 13}, {10, 14}, {3, 6}, {7, 1}, {9, 4},
  {7, 9}, {3, 1}, {13, 12}, {11, 14}, {2, 6}, {5, 10}, {4, 0}, {15, 8}
};

static const uint32_t u[16] = {
  0x243f6a88, 0x85a308d3, 0x13198a2e, 0x03707344,
  0xa4093822, 0x299f31d0, 0x082efa98, 0xec4e6c89,
  0x452821e6, 0x38d01377, 0xbe5466cf, 0x34e90c6c,
  0xc0ac29b7, 0xc97c50dd, 0x3f84d5b5, 0xb5470917
};

static inline void g(uint32_t *v, const uint32_t *m, size_t i, size_t a, size_t b, size_t c, size_t d) {
  v[a] += (m[sigma[i][0]] ^ u[sigma[i][1]]) + v[b];
  v[d] = rol32(v[d] ^ v[a], 16);
  v[c] += v[d];
  v[b] = rol32(v[b] ^ v[c], 20);
  v[a] += (m[sigma[i][1]] ^ u[sigma[i][0]]) + v[b];
  v[d] = rol32(v[d] ^ v[a], 24);
  v[c] += v[d];
  v[b] = rol32(v[b] ^ v[c], 25);
}

static void compress(uint32_t *state, size_t t, const uint8_t *block) {
  uint32_t v[16];
#if BYTE_ORDER != BIG_ENDIAN
  uint32_t m[16];
#endif
  size_t i;

#if BYTE_ORDER == BIG_ENDIAN
#define m ((const uint32_t *) block)
#else
  memcpy_swap32be(&m, block, 16);
#endif
  memcpy(&v, state, 32);
  memcpy(&v[8], &u, 32);
  v[12] ^= (uint32_t) t;
  v[13] ^= (uint32_t) t;
  if (sizeof(size_t) > 4) {
    v[14] ^= (uint32_t) (t >> 32);
    v[15] ^= (uint32_t) (t >> 32);
  }

  for (i = 0; i < 112; i += 8) {
    g(v, m, i, 0, 4, 8, 12);
    g(v, m, i + 1, 1, 5, 9, 13);
    g(v, m, i + 2, 2, 6, 10, 14);
    g(v, m, i + 3, 3, 7, 11, 15);
    g(v, m, i + 4, 0, 5, 10, 15);
    g(v, m, i + 5, 1, 6, 11, 12);
    g(v, m, i + 6, 2, 7, 8, 13);
    g(v, m, i + 7, 3, 4, 9, 14);
  }

  for (i = 0; i < 16; ++i) {
    state[i % 8] ^= v[i];
  }
#undef m
}

void hash_extra_blake(const void *data, size_t length, char *hash) {
  uint32_t state[8];
  uint8_t buffer[64];
  size_t i;
  memcpy(&state, &init_state, 32);
  for (i = 0; i + 64 <= length; i += 64) {
    compress(state, 8 * (i + 64), cpadd(data, i));
  }
  memcpy(&buffer, cpadd(data, i), length - i);
  buffer[length - i] = 0x80;
  if (length - i < 56) {
    memset(padd(&buffer, length - i + 1), 0, 63 - sizeof(size_t) - (length - i));
  } else {
    memset(padd(&buffer, length - i + 1), 0, 63 - (length - i));
    compress(state, 8 * length, buffer);
    memset(&buffer, 0, 64 - sizeof(size_t));
  }
  buffer[55] |= 0x01;
  place_length(buffer, 64, 8 * length);
  compress(state, ((length - 1) & 63) >= 55 ? 0 : 8 * length, buffer);
  memcpy_swap32be(hash, &state, 8);
}
