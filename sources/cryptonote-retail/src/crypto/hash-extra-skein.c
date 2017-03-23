// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common/int-util.h"
#include "hash-ops.h"

static const uint64_t init_state[8] = {
  0xccd044a12fdb3e13, 0xe83590301a79a9eb, 0x55aea0614f816e6f, 0x2a2767a4ae9b94db,
  0xec06025e74dd7683, 0xe7a436cdc4746251, 0xc36fbaf9393ad185, 0x3eedba1833edfc13
};

static inline void arx(uint64_t *state, size_t a, size_t b, size_t r) {
  state[a] += state[b];
  state[b] = rol64(state[b], (int)r) ^ state[a];
}

static inline void ak(uint64_t *state, uint64_t *key, uint64_t *tweak, size_t i) {
  state[0] += key[i % 9];
  state[1] += key[(i + 1) % 9];
  state[2] += key[(i + 2) % 9];
  state[3] += key[(i + 3) % 9];
  state[4] += key[(i + 4) % 9];
  state[5] += key[(i + 5) % 9] + tweak[i % 3];
  state[6] += key[(i + 6) % 9] + tweak[(i + 1) % 3];
  state[7] += key[(i + 7) % 9] + i;
}

static void compress(uint64_t *state, uint64_t t0, uint64_t t1, const uint8_t *block) {
  uint64_t tweak[3];
  uint64_t w[8];
#if BYTE_ORDER != LITTLE_ENDIAN
  uint64_t wc[8];
#endif
  size_t i;

  state[8] = state[0] ^ state[1] ^ state[2] ^ state[3] ^ state[4] ^ state[5] ^ state[6] ^ state[7] ^ 0x1bd11bdaa9fc1a22;
  tweak[0] = t0;
  tweak[1] = t1;
  tweak[2] = t0 ^ t1;
  memcpy_swap64le(w, block, 8);
#if BYTE_ORDER == LITTLE_ENDIAN
#define wc ((const uint64_t *) block)
#else
  memcpy(&wc, &w, 64);
#endif

  for (i = 0; i < 9; i++) {
    ak(w, state, tweak, 2 * i);
    arx(w, 0, 1, 46);
    arx(w, 2, 3, 36);
    arx(w, 4, 5, 19);
    arx(w, 6, 7, 37);
    arx(w, 2, 1, 33);
    arx(w, 4, 7, 27);
    arx(w, 6, 5, 14);
    arx(w, 0, 3, 42);
    arx(w, 4, 1, 17);
    arx(w, 6, 3, 49);
    arx(w, 0, 5, 36);
    arx(w, 2, 7, 39);
    arx(w, 6, 1, 44);
    arx(w, 0, 7, 9);
    arx(w, 2, 5, 54);
    arx(w, 4, 3, 56);
    ak(w, state, tweak, 2 * i + 1);
    arx(w, 0, 1, 39);
    arx(w, 2, 3, 30);
    arx(w, 4, 5, 34);
    arx(w, 6, 7, 24);
    arx(w, 2, 1, 13);
    arx(w, 4, 7, 50);
    arx(w, 6, 5, 10);
    arx(w, 0, 3, 17);
    arx(w, 4, 1, 25);
    arx(w, 6, 3, 29);
    arx(w, 0, 5, 39);
    arx(w, 2, 7, 43);
    arx(w, 6, 1, 8);
    arx(w, 0, 7, 35);
    arx(w, 2, 5, 56);
    arx(w, 4, 3, 22);
  }
  ak(w, state, tweak, 18);

  state[0] = w[0] ^ wc[0];
  state[1] = w[1] ^ wc[1];
  state[2] = w[2] ^ wc[2];
  state[3] = w[3] ^ wc[3];
  state[4] = w[4] ^ wc[4];
  state[5] = w[5] ^ wc[5];
  state[6] = w[6] ^ wc[6];
  state[7] = w[7] ^ wc[7];
#undef wc
}

void hash_extra_skein(const void *data, size_t length, char *hash) {
  uint64_t state[9];
  uint8_t buffer[64];
  size_t i;
  memcpy(&state, init_state, 64);
  for (i = 0; i + 64 < length; i += 64) {
    compress(state, i + 64, i == 0 ? 0x7000000000000000 : 0x3000000000000000, cpadd(data, i));
  }
  memcpy(&buffer, cpadd(data, i), length - i);
  memset(padd(&buffer, length - i), 0, 64 - (length - i));
  compress(state, length, length <= 64 ? 0xf000000000000000 : 0xb000000000000000, buffer);
  memset(&buffer, 0, 64);
  compress(state, 8, 0xff00000000000000, buffer);
  memcpy_swap64le(hash, state, 4);
}
