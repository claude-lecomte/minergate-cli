
#ifndef _CUDADEFS_H_
#define _CUDADEFS_H_

#include <stdint.h>

struct cryptonight_gpu_ctx {
        uint32_t state[50];
        uint32_t a[4];
        uint32_t b[4];
        uint32_t key1[40];
        uint32_t key2[40];
        uint32_t text[32];
    };

#define MEMORY         (1 << 21) // 2 MiB / 2097152 B
#define ITER           (1 << 20) // 1048576
#define AES_BLOCK_SIZE  16
#define AES_KEY_SIZE    32
#define INIT_SIZE_BLK   8
#define INIT_SIZE_BYTE (INIT_SIZE_BLK * AES_BLOCK_SIZE) // 128 B

#ifndef ROTL64
    #if __CUDA_ARCH__ >= 350
        __forceinline__ __device__ uint64_t cuda_ROTL64(const uint64_t value, const int offset) {
            uint2 result;
            if(offset >= 32) {
                asm("shf.l.wrap.b32 %0, %1, %2, %3;" : "=r"(result.x) : "r"(__double2loint(__longlong_as_double(value))), "r"(__double2hiint(__longlong_as_double(value))), "r"(offset));
                asm("shf.l.wrap.b32 %0, %1, %2, %3;" : "=r"(result.y) : "r"(__double2hiint(__longlong_as_double(value))), "r"(__double2loint(__longlong_as_double(value))), "r"(offset));
            } else {
                asm("shf.l.wrap.b32 %0, %1, %2, %3;" : "=r"(result.x) : "r"(__double2hiint(__longlong_as_double(value))), "r"(__double2loint(__longlong_as_double(value))), "r"(offset));
                asm("shf.l.wrap.b32 %0, %1, %2, %3;" : "=r"(result.y) : "r"(__double2loint(__longlong_as_double(value))), "r"(__double2hiint(__longlong_as_double(value))), "r"(offset));
            }
            return  __double_as_longlong(__hiloint2double(result.y, result.x));
        }
        #define ROTL64(x, n) (cuda_ROTL64(x, n))
    #else
        #define ROTL64(x, n)        (((x) << (n)) | ((x) >> (64 - (n))))
    #endif
#endif

#ifndef ROTL32
    #if __CUDA_ARCH__ < 350 
        #define ROTL32(x, n) T32(((x) << (n)) | ((x) >> (32 - (n))))
    #else
        #define ROTL32(x, n) __funnelshift_l( (x), (x), (n) )
    #endif
#endif

#ifndef ROTR32
    #if __CUDA_ARCH__ < 350 
        #define ROTR32(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
    #else
        #define ROTR32(x, n) __funnelshift_r( (x), (x), (n) )
    #endif
#endif


#define MEMSET4(dst,what,cnt) { \
    int i_memset4; \
    uint32_t *out_memset4 = (uint32_t *)(dst); \
    for( i_memset4 = 0; i_memset4 < cnt; i_memset4++ ) \
        out_memset4[i_memset4] = (what); }

#define MEMSET8(dst,what,cnt) { \
    int i_memset8; \
    uint64_t *out_memset8 = (uint64_t *)(dst); \
    for( i_memset8 = 0; i_memset8 < cnt; i_memset8++ ) \
        out_memset8[i_memset8] = (what); }

#define MEMCPY8(dst,src,cnt) { \
    int i_memcpy8; \
    uint64_t *in_memcpy8 = (uint64_t *)(src); \
    uint64_t *out_memcpy8 = (uint64_t *)(dst); \
    for( i_memcpy8 = 0; i_memcpy8 < cnt; i_memcpy8++ ) \
        out_memcpy8[i_memcpy8] = in_memcpy8[i_memcpy8]; }

#define MEMCPY4(dst,src,cnt) { \
    int i_memcpy4; \
    uint32_t *in_memcpy4 = (uint32_t *)(src); \
    uint32_t *out_memcpy4 = (uint32_t *)(dst); \
    for( i_memcpy4 = 0; i_memcpy4 < cnt; i_memcpy4++ ) \
        out_memcpy4[i_memcpy4] = in_memcpy4[i_memcpy4]; }

#define XOR_BLOCKS_DST(x,y,z) { \
    ((uint64_t *)z)[0] = ((uint64_t *)(x))[0] ^ ((uint64_t *)(y))[0]; \
    ((uint64_t *)z)[1] = ((uint64_t *)(x))[1] ^ ((uint64_t *)(y))[1]; }

#define MUL_SUM_XOR_DST(a,c,dst) { \
    uint64_t hi, lo = cuda_mul128(((uint64_t *)a)[0], ((uint64_t *)dst)[0], &hi) + ((uint64_t *)c)[1]; \
    hi += ((uint64_t *)c)[0]; \
    ((uint64_t *)c)[0] = ((uint64_t *)dst)[0] ^ hi; \
    ((uint64_t *)c)[1] = ((uint64_t *)dst)[1] ^ lo; \
    ((uint64_t *)dst)[0] = hi; \
    ((uint64_t *)dst)[1] = lo; }

#ifdef _MSC_VER
void usleep(__int64 waitTime);
#endif

#endif

