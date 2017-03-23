#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "common/int-util.h"
#include "../hash-ops.h"
#include "../oaes_lib.h"

#include <emmintrin.h>

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#include <intrin.h>
#define STATIC
#define INLINE __inline
#if !defined(RDATA_ALIGN16)
#define RDATA_ALIGN16 __declspec(align(16))
#endif
#else
#include <wmmintrin.h>
#define STATIC static
#define INLINE inline
#if !defined(RDATA_ALIGN16)
#define RDATA_ALIGN16 __attribute__ ((aligned(16)))
#endif
#endif

#define MEMORY         (1 << 21) // 2MB scratchpad
#define ITER           (1 << 20)
#define AES_BLOCK_SIZE  16
#define AES_KEY_SIZE    32
#define INIT_SIZE_BLK   8
#define INIT_SIZE_BYTE (INIT_SIZE_BLK * AES_BLOCK_SIZE)

#define U64(x) ((uint64_t *) (x))
#define R128(x) ((__m128i *) (x))

extern int aesb_single_round(const uint8_t *in, uint8_t*out, const uint8_t *expandedKey);
extern int aesb_pseudo_round(const uint8_t *in, uint8_t *out, const uint8_t *expandedKey);

typedef __m128i elem;

#pragma pack(push, 1)
union cn_slow_hash_state
{
    union hash_state hs;
    struct
    {
        uint8_t k[64];
        uint8_t init[INIT_SIZE_BYTE];
    };
};
#pragma pack(pop)

STATIC INLINE void mul(const uint8_t *a, const uint8_t *b, uint8_t *res)
{
    uint64_t a0, b0;
    uint64_t hi, lo;

    a0 = U64(a)[0];
    b0 = U64(b)[0];
    lo = mul128(a0, b0, &hi);
    U64(res)[0] = hi;
    U64(res)[1] = lo;
}

STATIC INLINE void sum_half_blocks(uint8_t *a, const uint8_t *b)
{
    uint64_t a0, a1, b0, b1;
    a0 = U64(a)[0];
    a1 = U64(a)[1];
    b0 = U64(b)[0];
    b1 = U64(b)[1];
    a0 += b0;
    a1 += b1;
    U64(a)[0] = a0;
    U64(a)[1] = a1;
}

STATIC INLINE void swap_blocks(uint8_t *a, uint8_t *b)
{
    uint64_t t[2];
    U64(t)[0] = U64(a)[0];
    U64(t)[1] = U64(a)[1];
    U64(a)[0] = U64(b)[0];
    U64(a)[1] = U64(b)[1];
    U64(b)[0] = U64(t)[0];
    U64(b)[1] = U64(t)[1];
}

STATIC INLINE void xor_blocks(uint8_t *a, const uint8_t *b)
{
    U64(a)[0] ^= U64(b)[0];
    U64(a)[1] ^= U64(b)[1];
}

void cn_slow_hash_software(const void *data, size_t length, char *hash, elem *long_state_)
{
//    uint8_t long_state[MEMORY];
    uint8_t *long_state = (uint8_t*)long_state_;
    uint8_t text[INIT_SIZE_BYTE];
    uint8_t a[AES_BLOCK_SIZE];
    uint8_t b[AES_BLOCK_SIZE];
    uint8_t d[AES_BLOCK_SIZE];
//    uint8_t aes_key[AES_KEY_SIZE];
    RDATA_ALIGN16 uint8_t expandedKey[256];

    union cn_slow_hash_state state;

    size_t i, j;
    uint8_t *p = NULL;
    oaes_ctx *aes_ctx;

    static void (*const extra_hashes[4])(const void *, size_t, char *) =
    {
        hash_extra_blake, hash_extra_groestl, hash_extra_jh, hash_extra_skein
    };

    hash_process(&state.hs, data, length);
    memcpy(text, state.init, INIT_SIZE_BYTE);

    aes_ctx = (oaes_ctx *) oaes_alloc();
    oaes_key_import_data(aes_ctx, state.hs.b, AES_KEY_SIZE);

    // use aligned data
    memcpy(expandedKey, aes_ctx->key->exp_data, aes_ctx->key->exp_data_len);

    for(i = 0; i < MEMORY / INIT_SIZE_BYTE; i++)
    {
        for(j = 0; j < INIT_SIZE_BLK; j++)
            aesb_pseudo_round(&text[AES_BLOCK_SIZE * j], &text[AES_BLOCK_SIZE * j], expandedKey);

        memcpy(&long_state[i * INIT_SIZE_BYTE], text, INIT_SIZE_BYTE);
    }

    U64(a)[0] = U64(&state.k[0])[0] ^ U64(&state.k[32])[0];
    U64(a)[1] = U64(&state.k[0])[1] ^ U64(&state.k[32])[1];
    U64(b)[0] = U64(&state.k[16])[0] ^ U64(&state.k[48])[0];
    U64(b)[1] = U64(&state.k[16])[1] ^ U64(&state.k[48])[1];

    for(i = 0; i < ITER / 2; i++)
    {
				#define TOTALBLOCKS (MEMORY / AES_BLOCK_SIZE)
				#define state_index(x) (((*((uint64_t *)x) >> 4) & (TOTALBLOCKS - 1)) << 4)

        // Iteration 1
        p = &long_state[state_index(a)];

        aesb_single_round(p, p, a);

        xor_blocks(b, p);
        swap_blocks(b, p);
        swap_blocks(a, b);

        // Iteration 2
        p = &long_state[state_index(a)];

        mul(a, p, d);
        sum_half_blocks(b, d);
        swap_blocks(b, p);
        xor_blocks(b, p);
        swap_blocks(a, b);
    }

    memcpy(text, state.init, INIT_SIZE_BYTE);
    oaes_key_import_data(aes_ctx, &state.hs.b[32], AES_KEY_SIZE);
    memcpy(expandedKey, aes_ctx->key->exp_data, aes_ctx->key->exp_data_len);

    for(i = 0; i < MEMORY / INIT_SIZE_BYTE; i++)
    {
        for(j = 0; j < INIT_SIZE_BLK; j++)
        {
            xor_blocks(&text[j * AES_BLOCK_SIZE], &long_state[i * INIT_SIZE_BYTE + j * AES_BLOCK_SIZE]);
            aesb_pseudo_round(&text[AES_BLOCK_SIZE * j], &text[AES_BLOCK_SIZE * j], expandedKey);
        }
    }

    oaes_free((OAES_CTX **) &aes_ctx);
    memcpy(state.init, text, INIT_SIZE_BYTE);
    hash_permutation(&state.hs);
    extra_hashes[state.hs.b[0] & 3](&state, 200, hash);
}
