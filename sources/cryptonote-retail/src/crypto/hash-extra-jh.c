// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common/int-util.h"
#include "hash-ops.h"

static const uint64_t initial_state[16] = {
  SWAP64BE(0xeb98a3412c20d3eb), SWAP64BE(0x92cdbe7b9cb245c1), SWAP64BE(0x1c93519160d4c7fa), SWAP64BE(0x260082d67e508a03),
  SWAP64BE(0xa4239e267726b945), SWAP64BE(0xe0fb1a48d41a9477), SWAP64BE(0xcdb5ab26026b177a), SWAP64BE(0x56f024420fff2fa8),
  SWAP64BE(0x71a396897f2e4d75), SWAP64BE(0x1d144908f77de262), SWAP64BE(0x277695f776248f94), SWAP64BE(0x87d5b6574780296c),
  SWAP64BE(0x5c5e272dac8e0d6c), SWAP64BE(0x518450c657057a0f), SWAP64BE(0x7be4d367702412ea), SWAP64BE(0x89e3ab13d31cd769)
};

static const uint64_t round_constants[42][4] = {
  {SWAP64BE(0x72d5dea2df15f867), SWAP64BE(0x7b84150ab7231557), SWAP64BE(0x81abd6904d5a87f6), SWAP64BE(0x4e9f4fc5c3d12b40)},
  {SWAP64BE(0xea983ae05c45fa9c), SWAP64BE(0x03c5d29966b2999a), SWAP64BE(0x660296b4f2bb538a), SWAP64BE(0xb556141a88dba231)},
  {SWAP64BE(0x03a35a5c9a190edb), SWAP64BE(0x403fb20a87c14410), SWAP64BE(0x1c051980849e951d), SWAP64BE(0x6f33ebad5ee7cddc)},
  {SWAP64BE(0x10ba139202bf6b41), SWAP64BE(0xdc786515f7bb27d0), SWAP64BE(0x0a2c813937aa7850), SWAP64BE(0x3f1abfd2410091d3)},
  {SWAP64BE(0x422d5a0df6cc7e90), SWAP64BE(0xdd629f9c92c097ce), SWAP64BE(0x185ca70bc72b44ac), SWAP64BE(0xd1df65d663c6fc23)},
  {SWAP64BE(0x976e6c039ee0b81a), SWAP64BE(0x2105457e446ceca8), SWAP64BE(0xeef103bb5d8e61fa), SWAP64BE(0xfd9697b294838197)},
  {SWAP64BE(0x4a8e8537db03302f), SWAP64BE(0x2a678d2dfb9f6a95), SWAP64BE(0x8afe7381f8b8696c), SWAP64BE(0x8ac77246c07f4214)},
  {SWAP64BE(0xc5f4158fbdc75ec4), SWAP64BE(0x75446fa78f11bb80), SWAP64BE(0x52de75b7aee488bc), SWAP64BE(0x82b8001e98a6a3f4)},
  {SWAP64BE(0x8ef48f33a9a36315), SWAP64BE(0xaa5f5624d5b7f989), SWAP64BE(0xb6f1ed207c5ae0fd), SWAP64BE(0x36cae95a06422c36)},
  {SWAP64BE(0xce2935434efe983d), SWAP64BE(0x533af974739a4ba7), SWAP64BE(0xd0f51f596f4e8186), SWAP64BE(0x0e9dad81afd85a9f)},
  {SWAP64BE(0xa7050667ee34626a), SWAP64BE(0x8b0b28be6eb91727), SWAP64BE(0x47740726c680103f), SWAP64BE(0xe0a07e6fc67e487b)},
  {SWAP64BE(0x0d550aa54af8a4c0), SWAP64BE(0x91e3e79f978ef19e), SWAP64BE(0x8676728150608dd4), SWAP64BE(0x7e9e5a41f3e5b062)},
  {SWAP64BE(0xfc9f1fec4054207a), SWAP64BE(0xe3e41a00cef4c984), SWAP64BE(0x4fd794f59dfa95d8), SWAP64BE(0x552e7e1124c354a5)},
  {SWAP64BE(0x5bdf7228bdfe6e28), SWAP64BE(0x78f57fe20fa5c4b2), SWAP64BE(0x05897cefee49d32e), SWAP64BE(0x447e9385eb28597f)},
  {SWAP64BE(0x705f6937b324314a), SWAP64BE(0x5e8628f11dd6e465), SWAP64BE(0xc71b770451b920e7), SWAP64BE(0x74fe43e823d4878a)},
  {SWAP64BE(0x7d29e8a3927694f2), SWAP64BE(0xddcb7a099b30d9c1), SWAP64BE(0x1d1b30fb5bdc1be0), SWAP64BE(0xda24494ff29c82bf)},
  {SWAP64BE(0xa4e7ba31b470bfff), SWAP64BE(0x0d324405def8bc48), SWAP64BE(0x3baefc3253bbd339), SWAP64BE(0x459fc3c1e0298ba0)},
  {SWAP64BE(0xe5c905fdf7ae090f), SWAP64BE(0x947034124290f134), SWAP64BE(0xa271b701e344ed95), SWAP64BE(0xe93b8e364f2f984a)},
  {SWAP64BE(0x88401d63a06cf615), SWAP64BE(0x47c1444b8752afff), SWAP64BE(0x7ebb4af1e20ac630), SWAP64BE(0x4670b6c5cc6e8ce6)},
  {SWAP64BE(0xa4d5a456bd4fca00), SWAP64BE(0xda9d844bc83e18ae), SWAP64BE(0x7357ce453064d1ad), SWAP64BE(0xe8a6ce68145c2567)},
  {SWAP64BE(0xa3da8cf2cb0ee116), SWAP64BE(0x33e906589a94999a), SWAP64BE(0x1f60b220c26f847b), SWAP64BE(0xd1ceac7fa0d18518)},
  {SWAP64BE(0x32595ba18ddd19d3), SWAP64BE(0x509a1cc0aaa5b446), SWAP64BE(0x9f3d6367e4046bba), SWAP64BE(0xf6ca19ab0b56ee7e)},
  {SWAP64BE(0x1fb179eaa9282174), SWAP64BE(0xe9bdf7353b3651ee), SWAP64BE(0x1d57ac5a7550d376), SWAP64BE(0x3a46c2fea37d7001)},
  {SWAP64BE(0xf735c1af98a4d842), SWAP64BE(0x78edec209e6b6779), SWAP64BE(0x41836315ea3adba8), SWAP64BE(0xfac33b4d32832c83)},
  {SWAP64BE(0xa7403b1f1c2747f3), SWAP64BE(0x5940f034b72d769a), SWAP64BE(0xe73e4e6cd2214ffd), SWAP64BE(0xb8fd8d39dc5759ef)},
  {SWAP64BE(0x8d9b0c492b49ebda), SWAP64BE(0x5ba2d74968f3700d), SWAP64BE(0x7d3baed07a8d5584), SWAP64BE(0xf5a5e9f0e4f88e65)},
  {SWAP64BE(0xa0b8a2f436103b53), SWAP64BE(0x0ca8079e753eec5a), SWAP64BE(0x9168949256e8884f), SWAP64BE(0x5bb05c55f8babc4c)},
  {SWAP64BE(0xe3bb3b99f387947b), SWAP64BE(0x75daf4d6726b1c5d), SWAP64BE(0x64aeac28dc34b36d), SWAP64BE(0x6c34a550b828db71)},
  {SWAP64BE(0xf861e2f2108d512a), SWAP64BE(0xe3db643359dd75fc), SWAP64BE(0x1cacbcf143ce3fa2), SWAP64BE(0x67bbd13c02e843b0)},
  {SWAP64BE(0x330a5bca8829a175), SWAP64BE(0x7f34194db416535c), SWAP64BE(0x923b94c30e794d1e), SWAP64BE(0x797475d7b6eeaf3f)},
  {SWAP64BE(0xeaa8d4f7be1a3921), SWAP64BE(0x5cf47e094c232751), SWAP64BE(0x26a32453ba323cd2), SWAP64BE(0x44a3174a6da6d5ad)},
  {SWAP64BE(0xb51d3ea6aff2c908), SWAP64BE(0x83593d98916b3c56), SWAP64BE(0x4cf87ca17286604d), SWAP64BE(0x46e23ecc086ec7f6)},
  {SWAP64BE(0x2f9833b3b1bc765e), SWAP64BE(0x2bd666a5efc4e62a), SWAP64BE(0x06f4b6e8bec1d436), SWAP64BE(0x74ee8215bcef2163)},
  {SWAP64BE(0xfdc14e0df453c969), SWAP64BE(0xa77d5ac406585826), SWAP64BE(0x7ec1141606e0fa16), SWAP64BE(0x7e90af3d28639d3f)},
  {SWAP64BE(0xd2c9f2e3009bd20c), SWAP64BE(0x5faace30b7d40c30), SWAP64BE(0x742a5116f2e03298), SWAP64BE(0x0deb30d8e3cef89a)},
  {SWAP64BE(0x4bc59e7bb5f17992), SWAP64BE(0xff51e66e048668d3), SWAP64BE(0x9b234d57e6966731), SWAP64BE(0xcce6a6f3170a7505)},
  {SWAP64BE(0xb17681d913326cce), SWAP64BE(0x3c175284f805a262), SWAP64BE(0xf42bcbb378471547), SWAP64BE(0xff46548223936a48)},
  {SWAP64BE(0x38df58074e5e6565), SWAP64BE(0xf2fc7c89fc86508e), SWAP64BE(0x31702e44d00bca86), SWAP64BE(0xf04009a23078474e)},
  {SWAP64BE(0x65a0ee39d1f73883), SWAP64BE(0xf75ee937e42c3abd), SWAP64BE(0x2197b2260113f86f), SWAP64BE(0xa344edd1ef9fdee7)},
  {SWAP64BE(0x8ba0df15762592d9), SWAP64BE(0x3c85f7f612dc42be), SWAP64BE(0xd8a7ec7cab27b07e), SWAP64BE(0x538d7ddaaa3ea8de)},
  {SWAP64BE(0xaa25ce93bd0269d8), SWAP64BE(0x5af643fd1a7308f9), SWAP64BE(0xc05fefda174a19a5), SWAP64BE(0x974d66334cfd216a)},
  {SWAP64BE(0x35b49831db411570), SWAP64BE(0xea1e0fbbedcd549b), SWAP64BE(0x9ad063a151974072), SWAP64BE(0xf6759dbf91476fe2)}
};

static inline void sl(uint64_t *state, size_t i) {
  size_t j;
  for (j = 0; j < 4; j++) {
    uint64_t temp;
    state[j + 12] = ~state[j + 12];
    state[j] ^= ~state[j + 8] & round_constants[i][j];
    temp = round_constants[i][j] ^ (state[j] & state[j + 4]);
    state[j] ^= state[j + 8] & state[j + 12];
    state[j + 12] ^= ~state[j + 4] & state[j + 8];
    state[j + 4] ^= state[j] & state[j + 8];
    state[j + 8] ^= state[j] & ~state[j + 12];
    state[j] ^= state[j + 4] | state[j + 12];
    state[j + 12] ^= state[j + 4] & state[j + 8];
    state[j + 4] ^= temp & state[j];
    state[j + 8] ^= temp;
  }
  for (j = 0; j < 2; j++) {
    state[j + 2] ^= state[j + 4];
    state[j + 6] ^= state[j + 8];
    state[j + 10] ^= state[j] ^ state[j + 12];
    state[j + 14] ^= state[j];
    state[j] ^= state[j + 6];
    state[j + 4] ^= state[j + 10];
    state[j + 8] ^= state[j + 2] ^ state[j + 14];
    state[j + 12] ^= state[j + 2];
  }
}

static void compress(uint64_t *state, const uint8_t *buffer) {
  size_t i, j;
  for (i = 0; i < 8; i++) {
    state[i] ^= ((uint64_t *) buffer)[i];
  }
  for (i = 0; i < 42; i += 7) {
    sl(state, i);
    for (j = 2; j < 16; j = (j + 1) | 2) {
      state[j] = ((state[j] & 0x5555555555555555) << 1) | ((state[j] & 0xaaaaaaaaaaaaaaaa) >> 1);
    }
    sl(state, i + 1);
    for (j = 2; j < 16; j = (j + 1) | 2) {
      state[j] = ((state[j] & 0x3333333333333333) << 2) | ((state[j] & 0xcccccccccccccccc) >> 2);
    }
    sl(state, i + 2);
    for (j = 2; j < 16; j = (j + 1) | 2) {
      state[j] = ((state[j] & 0x0f0f0f0f0f0f0f0f) << 4) | ((state[j] & 0xf0f0f0f0f0f0f0f0) >> 4);
    }
    sl(state, i + 3);
    for (j = 2; j < 16; j = (j + 1) | 2) {
      state[j] = ((state[j] & 0x00ff00ff00ff00ff) << 8) | ((state[j] & 0xff00ff00ff00ff00) >> 8);
    }
    sl(state, i + 4);
    for (j = 2; j < 16; j = (j + 1) | 2) {
      state[j] = ((state[j] & 0x0000ffff0000ffff) << 16) | ((state[j] & 0xffff0000ffff0000) >> 16);
    }
    sl(state, i + 5);
    for (j = 2; j < 16; j = (j + 1) | 2) {
      state[j] = (state[j] << 32) | (state[j] >> 32);
    }
    sl(state, i + 6);
    for (j = 2; j < 16; j += 4) {
      uint64_t temp = state[j];
      state[j] = state[j + 1];
      state[j + 1] = temp;
    }
  }
  for (i = 0; i < 8; i++) {
    state[8 + i] ^= ((uint64_t *) buffer)[i];
  }
}

void hash_extra_jh(const void *data, size_t length, char *hash) {
  uint64_t state[16];
  uint8_t buffer[64];
  size_t i;
  memcpy(&state, &initial_state, 128);
  for (i = 0; i + 64 <= length; i += 64) {
    compress(state, cpadd(data, i));
  }
  memcpy(&buffer, cpadd(data, i), length - i);
  buffer[length - i] = 0x80;
  if (length - i != 0) {
    memset(padd(&buffer, length - i + 1), 0, 63 - (length - i));
    compress(state, buffer);
    memset(&buffer, 0, 64 - sizeof(size_t));
  } else {
    memset(padd(&buffer, 1), 0, 63 - sizeof(size_t));
  }
  place_length(buffer, 64, length * 8);
  compress(state, buffer);
  memcpy(hash, cpadd(&state, 96), 32);
}
