#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cuda.h"
#include "cuda_runtime.h"
#include "cudadefs.h"

#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include <Windows.h>
#endif

extern int device_arch[8][2];
extern int device_bfactor[8];
extern int device_bsleep[8];

#include "cuda_cryptonight_aes.cuh"

__device__ __forceinline__ uint64_t cuda_mul128(uint64_t multiplier, uint64_t multiplicand, uint64_t* product_hi)
{
    *product_hi = __umul64hi(multiplier, multiplicand);
    return(multiplier * multiplicand);
}

__global__ void cryptonight_core_gpu_phase1(int threads, uint32_t *long_state, struct cryptonight_gpu_ctx *ctx)
{
  __shared__ uint32_t sharedMemory[1024];

    cn_aes_gpu_init(sharedMemory);

  __syncthreads();

    int thread = (blockDim.x * blockIdx.x + threadIdx.x) >> 3;
    int sub = (threadIdx.x & 7) << 2;

    if (thread < threads)
    {
        uint32_t key[40], text[4], i;

        MEMCPY8(key, ctx[thread].key1, 20);
        MEMCPY8(text, &ctx[thread].state[sub + 16], 2);

        for(i = 0; i < 0x80000; i += 32)
        {
            cn_aes_pseudo_round_mut(sharedMemory, text, key);
            MEMCPY8(&long_state[(thread << 19) + sub + i], text, 2);
        }
    }
}

__global__ void cryptonight_core_gpu_phase2(int threads, int bfactor, int partidx, uint32_t *d_long_state, struct cryptonight_gpu_ctx *d_ctx)
{
  __shared__ uint32_t sharedMemory[1024];

    cn_aes_gpu_init(sharedMemory);

  __syncthreads();

#if __CUDA_ARCH__ >= 300

    int thread = (blockDim.x * blockIdx.x + threadIdx.x) >> 2;
    int sub = threadIdx.x & 3;

    if (thread < threads)
    {
        int i, j, k;
        int batchsize = ITER >> (2+bfactor);
        int start = partidx * batchsize;
        int end = start + batchsize;
        uint32_t * __restrict__ long_state = &d_long_state[thread << 19];
        struct cryptonight_gpu_ctx *ctx = &d_ctx[thread];
        uint32_t a, b, c, x[4];
        uint32_t t1[4], t2[4], res;
        uint64_t reshi, reslo;

        a = ctx->a[sub];
        b = ctx->b[sub];

        for (i = start; i < end; ++i) {

            //j = ((uint32_t *)a)[0] & 0x1FFFF0;
            j = (__shfl((int)a, 0, 4) & 0x1FFFF0) >> 2;

            //cn_aes_single_round(sharedMemory, &long_state[j], c, a);
            x[0] = long_state[j + sub];
            x[1] = __shfl((int)x[0], sub+1, 4);
            x[2] = __shfl((int)x[0], sub+2, 4);
            x[3] = __shfl((int)x[0], sub+3, 4);
            c = a ^
                t_fn0(x[0] & 0xff) ^
                t_fn1((x[1] >> 8) & 0xff) ^
                t_fn2((x[2] >> 16) & 0xff) ^
                t_fn3((x[3] >> 24) & 0xff);

            //XOR_BLOCKS_DST(c, b, &long_state[j]);
            long_state[j + sub] = c ^ b;

            //MUL_SUM_XOR_DST(c, a, &long_state[((uint32_t *)c)[0] & 0x1FFFF0]);
            j = (__shfl((int)c, 0, 4) & 0x1FFFF0) >> 2;
            for( k = 0; k < 2; k++ ) t1[k] = __shfl((int)c, k, 4);
            for( k = 0; k < 4; k++ ) t2[k] = __shfl((int)a, k, 4);
            asm(
                "mad.lo.u64 %0, %2, %3, %4;\n\t"
                "mad.hi.u64 %1, %2, %3, %5;\n\t"
                : "=l"(reslo), "=l"(reshi)
                : "l"(((uint64_t *)t1)[0]), "l"(((uint64_t *)long_state)[j >> 1]), "l"(((uint64_t *)t2)[1]), "l"(((uint64_t *)t2)[0]));
            res = (sub & 2 ? reslo : reshi) >> (sub&1 ? 32 : 0);
            a = long_state[j + sub] ^ res;
            long_state[j + sub] = res;

            //j = ((uint32_t *)a)[0] & 0x1FFFF0;
            j = (__shfl((int)a, 0, 4) & 0x1FFFF0) >> 2;

            //cn_aes_single_round(sharedMemory, &long_state[j], b, a);
            x[0] = long_state[j + sub];
            x[1] = __shfl((int)x[0], sub+1, 4);
            x[2] = __shfl((int)x[0], sub+2, 4);
            x[3] = __shfl((int)x[0], sub+3, 4);
            b = a ^
                t_fn0(x[0] & 0xff) ^
                t_fn1((x[1] >> 8) & 0xff) ^
                t_fn2((x[2] >> 16) & 0xff) ^
                t_fn3((x[3] >> 24) & 0xff);

            //XOR_BLOCKS_DST(b, c, &long_state[j]);
            long_state[j + sub] = c ^ b;

            //MUL_SUM_XOR_DST(b, a, &long_state[((uint32_t *)b)[0] & 0x1FFFF0]);
            j = (__shfl((int)b, 0, 4) & 0x1FFFF0) >> 2;
            for( k = 0; k < 2; k++ ) t1[k] = __shfl((int)b, k, 4);
            for( k = 0; k < 4; k++ ) t2[k] = __shfl((int)a, k, 4);
            asm(
                "mad.lo.u64 %0, %2, %3, %4;\n\t"
                "mad.hi.u64 %1, %2, %3, %5;\n\t"
                : "=l"(reslo), "=l"(reshi)
                : "l"(((uint64_t *)t1)[0]), "l"(((uint64_t *)long_state)[j >> 1]), "l"(((uint64_t *)t2)[1]), "l"(((uint64_t *)t2)[0]));
            res = (sub & 2 ? reslo : reshi) >> (sub&1 ? 32 : 0);
            a = long_state[j + sub] ^ res;
            long_state[j + sub] = res;
        }

        if( bfactor > 0 ) {

            ctx->a[sub] = a;
            ctx->b[sub] = b;
        }
    }

#else // __CUDA_ARCH__ < 300

    int thread = blockDim.x * blockIdx.x + threadIdx.x;

    if (thread < threads)
    {
        int i, j;
        int batchsize = ITER >> (2+bfactor);
        int start = partidx * batchsize;
        int end = start + batchsize;
        uint32_t *long_state = &d_long_state[thread << 19];
        struct cryptonight_gpu_ctx *ctx = &d_ctx[thread];
        uint32_t a[4], b[4], c[4];

        MEMCPY8(a, ctx->a, 2);
        MEMCPY8(b, ctx->b, 2);

        for (i = start; i < end; ++i) {

            j = (a[0] & 0x1FFFF0) >> 2;
            cn_aes_single_round(sharedMemory, (uint8_t *)&long_state[j], c, a);
            XOR_BLOCKS_DST(c, b, &long_state[j]);
            MUL_SUM_XOR_DST(c, a, (uint8_t *)&long_state[(c[0] & 0x1FFFF0) >> 2]);
            j = (a[0] & 0x1FFFF0) >> 2;
            cn_aes_single_round(sharedMemory, (uint8_t *)&long_state[j], b, a);
            XOR_BLOCKS_DST(b, c, &long_state[j]);
            MUL_SUM_XOR_DST(b, a, &long_state[(b[0] & 0x1FFFF0) >> 2]);
        }

        if( bfactor > 0 ) {

            MEMCPY8(ctx->a, a, 2);
            MEMCPY8(ctx->b, b, 2);
        }
    }

#endif // __CUDA_ARCH__ >= 300
}

__global__ void cryptonight_core_gpu_phase3(int threads, uint32_t *long_state, struct cryptonight_gpu_ctx *ctx)
{
  __shared__ uint32_t sharedMemory[1024];

    cn_aes_gpu_init(sharedMemory);

  __syncthreads();

    int thread = (blockDim.x * blockIdx.x + threadIdx.x) >> 3;
    int sub = (threadIdx.x & 7) << 2;

    if (thread < threads)
    {
        uint32_t key[40], text[4], i, j;
        MEMCPY8(key, ctx[thread].key2, 20);
        MEMCPY8(text, &ctx[thread].state[sub + 16], 2);

        for (i = 0; i < 0x80000; i += 32)
        {
            for(j = 0; j < 4; ++j)
                text[j] ^= long_state[(thread << 19) + sub + i + j];

            cn_aes_pseudo_round_mut(sharedMemory, text, key);
        }

        MEMCPY8(&ctx[thread].state[sub + 16], text, 2);
    }
}


__host__ bool cryptonight_core_cpu_init(int deviceNo)
{
  cudaSetDevice(deviceNo);
  cn_aes_cpu_init();
    return true;
}

__host__ void cryptonight_core_cpu_hash(int deviceNo, int blocks, int threads, uint32_t *d_long_state, struct cryptonight_gpu_ctx *d_ctx, int device_bfactor, int cuda_major)
{
    cudaSetDevice(deviceNo);
    dim3 grid(blocks);
    dim3 block(threads);
    dim3 block4(threads << 2);
    dim3 block8(threads << 3);

    size_t shared_size = 1024;
    int i, partcount = 1 << device_bfactor;

    cryptonight_core_gpu_phase1<<<grid, block8, shared_size>>>(blocks*threads, d_long_state, d_ctx);
    cudaDeviceSynchronize();
    if( partcount > 1 ) usleep(100);

    for( i = 0; i < partcount; i++ ) {
        cryptonight_core_gpu_phase2<<<grid, (cuda_major >= 3 ? block4 : block), shared_size>>>(blocks*threads, device_bfactor, i, d_long_state, d_ctx);
        cudaDeviceSynchronize();
        if( partcount > 1 ) usleep(100);
    }

    cryptonight_core_gpu_phase3<<<grid, block8, shared_size>>>(blocks*threads, d_long_state, d_ctx);
    cudaDeviceSynchronize();
}

#ifdef _MSC_VER
void usleep(__int64 waitTime)
{
    if (waitTime > 0)
    {
        if (waitTime > 100)
        {
            // use a waitable timer for larger intervals > 0.1ms

            HANDLE timer;
            LARGE_INTEGER ft;

            ft.QuadPart = -(10*waitTime); // Convert to 100 nanosecond interval, negative value indicates relative time

            timer = CreateWaitableTimer(NULL, TRUE, NULL);
            SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
            WaitForSingleObject(timer, INFINITE);
            CloseHandle(timer);
        }
        else
        {
            // use a polling loop for short intervals <= 100ms

            LARGE_INTEGER perfCnt, start, now;
            __int64 elapsed;

            QueryPerformanceFrequency(&perfCnt);
            QueryPerformanceCounter(&start);
            do {
                QueryPerformanceCounter((LARGE_INTEGER*) &now);
                elapsed = (__int64)((now.QuadPart - start.QuadPart) / (float)perfCnt.QuadPart * 1000 * 1000);
            } while ( elapsed < waitTime );
        }
    }
}
#endif
