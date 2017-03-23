#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable

#ifndef WOLF_AES_CL
#define WOLF_AES_CL

// AES table - the other three are generated on the fly

static const __constant uint AES0_C[256] =
{
    0xA56363C6U, 0x847C7CF8U, 0x997777EEU, 0x8D7B7BF6U,
    0x0DF2F2FFU, 0xBD6B6BD6U, 0xB16F6FDEU, 0x54C5C591U,
    0x50303060U, 0x03010102U, 0xA96767CEU, 0x7D2B2B56U,
    0x19FEFEE7U, 0x62D7D7B5U, 0xE6ABAB4DU, 0x9A7676ECU,
    0x45CACA8FU, 0x9D82821FU, 0x40C9C989U, 0x877D7DFAU,
    0x15FAFAEFU, 0xEB5959B2U, 0xC947478EU, 0x0BF0F0FBU,
    0xECADAD41U, 0x67D4D4B3U, 0xFDA2A25FU, 0xEAAFAF45U,
    0xBF9C9C23U, 0xF7A4A453U, 0x967272E4U, 0x5BC0C09BU,
    0xC2B7B775U, 0x1CFDFDE1U, 0xAE93933DU, 0x6A26264CU,
    0x5A36366CU, 0x413F3F7EU, 0x02F7F7F5U, 0x4FCCCC83U,
    0x5C343468U, 0xF4A5A551U, 0x34E5E5D1U, 0x08F1F1F9U,
    0x937171E2U, 0x73D8D8ABU, 0x53313162U, 0x3F15152AU,
    0x0C040408U, 0x52C7C795U, 0x65232346U, 0x5EC3C39DU,
    0x28181830U, 0xA1969637U, 0x0F05050AU, 0xB59A9A2FU,
    0x0907070EU, 0x36121224U, 0x9B80801BU, 0x3DE2E2DFU,
    0x26EBEBCDU, 0x6927274EU, 0xCDB2B27FU, 0x9F7575EAU,
    0x1B090912U, 0x9E83831DU, 0x742C2C58U, 0x2E1A1A34U,
    0x2D1B1B36U, 0xB26E6EDCU, 0xEE5A5AB4U, 0xFBA0A05BU,
    0xF65252A4U, 0x4D3B3B76U, 0x61D6D6B7U, 0xCEB3B37DU,
    0x7B292952U, 0x3EE3E3DDU, 0x712F2F5EU, 0x97848413U,
    0xF55353A6U, 0x68D1D1B9U, 0x00000000U, 0x2CEDEDC1U,
    0x60202040U, 0x1FFCFCE3U, 0xC8B1B179U, 0xED5B5BB6U,
    0xBE6A6AD4U, 0x46CBCB8DU, 0xD9BEBE67U, 0x4B393972U,
    0xDE4A4A94U, 0xD44C4C98U, 0xE85858B0U, 0x4ACFCF85U,
    0x6BD0D0BBU, 0x2AEFEFC5U, 0xE5AAAA4FU, 0x16FBFBEDU,
    0xC5434386U, 0xD74D4D9AU, 0x55333366U, 0x94858511U,
    0xCF45458AU, 0x10F9F9E9U, 0x06020204U, 0x817F7FFEU,
    0xF05050A0U, 0x443C3C78U, 0xBA9F9F25U, 0xE3A8A84BU,
    0xF35151A2U, 0xFEA3A35DU, 0xC0404080U, 0x8A8F8F05U,
    0xAD92923FU, 0xBC9D9D21U, 0x48383870U, 0x04F5F5F1U,
    0xDFBCBC63U, 0xC1B6B677U, 0x75DADAAFU, 0x63212142U,
    0x30101020U, 0x1AFFFFE5U, 0x0EF3F3FDU, 0x6DD2D2BFU,
    0x4CCDCD81U, 0x140C0C18U, 0x35131326U, 0x2FECECC3U,
    0xE15F5FBEU, 0xA2979735U, 0xCC444488U, 0x3917172EU,
    0x57C4C493U, 0xF2A7A755U, 0x827E7EFCU, 0x473D3D7AU,
    0xAC6464C8U, 0xE75D5DBAU, 0x2B191932U, 0x957373E6U,
    0xA06060C0U, 0x98818119U, 0xD14F4F9EU, 0x7FDCDCA3U,
    0x66222244U, 0x7E2A2A54U, 0xAB90903BU, 0x8388880BU,
    0xCA46468CU, 0x29EEEEC7U, 0xD3B8B86BU, 0x3C141428U,
    0x79DEDEA7U, 0xE25E5EBCU, 0x1D0B0B16U, 0x76DBDBADU,
    0x3BE0E0DBU, 0x56323264U, 0x4E3A3A74U, 0x1E0A0A14U,
    0xDB494992U, 0x0A06060CU, 0x6C242448U, 0xE45C5CB8U,
    0x5DC2C29FU, 0x6ED3D3BDU, 0xEFACAC43U, 0xA66262C4U,
    0xA8919139U, 0xA4959531U, 0x37E4E4D3U, 0x8B7979F2U,
    0x32E7E7D5U, 0x43C8C88BU, 0x5937376EU, 0xB76D6DDAU,
    0x8C8D8D01U, 0x64D5D5B1U, 0xD24E4E9CU, 0xE0A9A949U,
    0xB46C6CD8U, 0xFA5656ACU, 0x07F4F4F3U, 0x25EAEACFU,
    0xAF6565CAU, 0x8E7A7AF4U, 0xE9AEAE47U, 0x18080810U,
    0xD5BABA6FU, 0x887878F0U, 0x6F25254AU, 0x722E2E5CU,
    0x241C1C38U, 0xF1A6A657U, 0xC7B4B473U, 0x51C6C697U,
    0x23E8E8CBU, 0x7CDDDDA1U, 0x9C7474E8U, 0x211F1F3EU,
    0xDD4B4B96U, 0xDCBDBD61U, 0x868B8B0DU, 0x858A8A0FU,
    0x907070E0U, 0x423E3E7CU, 0xC4B5B571U, 0xAA6666CCU,
    0xD8484890U, 0x05030306U, 0x01F6F6F7U, 0x120E0E1CU,
    0xA36161C2U, 0x5F35356AU, 0xF95757AEU, 0xD0B9B969U,
    0x91868617U, 0x58C1C199U, 0x271D1D3AU, 0xB99E9E27U,
    0x38E1E1D9U, 0x13F8F8EBU, 0xB398982BU, 0x33111122U,
    0xBB6969D2U, 0x70D9D9A9U, 0x898E8E07U, 0xA7949433U,
    0xB69B9B2DU, 0x221E1E3CU, 0x92878715U, 0x20E9E9C9U,
    0x49CECE87U, 0xFF5555AAU, 0x78282850U, 0x7ADFDFA5U,
    0x8F8C8C03U, 0xF8A1A159U, 0x80898909U, 0x170D0D1AU,
    0xDABFBF65U, 0x31E6E6D7U, 0xC6424284U, 0xB86868D0U,
    0xC3414182U, 0xB0999929U, 0x772D2D5AU, 0x110F0F1EU,
    0xCBB0B07BU, 0xFC5454A8U, 0xD6BBBB6DU, 0x3A16162CU
};

#define BYTE(x, y)	(amd_bfe((x), (y) << 3U, 8U))

uint4 AES_Round(const __local uint *AES0, const __local uint *AES1, const __local uint *AES2, const __local uint *AES3, const uint4 X, const uint4 key)
{
    uint4 Y;
    Y.s0 = AES0[BYTE(X.s0, 0)] ^ AES1[BYTE(X.s1, 1)] ^ AES2[BYTE(X.s2, 2)] ^ AES3[BYTE(X.s3, 3)];
    Y.s1 = AES0[BYTE(X.s1, 0)] ^ AES1[BYTE(X.s2, 1)] ^ AES2[BYTE(X.s3, 2)] ^ AES3[BYTE(X.s0, 3)];
    Y.s2 = AES0[BYTE(X.s2, 0)] ^ AES1[BYTE(X.s3, 1)] ^ AES2[BYTE(X.s0, 2)] ^ AES3[BYTE(X.s1, 3)];
    Y.s3 = AES0[BYTE(X.s3, 0)] ^ AES1[BYTE(X.s0, 1)] ^ AES2[BYTE(X.s1, 2)] ^ AES3[BYTE(X.s2, 3)];
    Y ^= key;
    return(Y);
}

#endif

#ifndef WOLF_SKEIN_CL
#define WOLF_SKEIN_CL

// Vectorized Skein implementation macros and functions by Wolf

#define SKEIN_KS_PARITY	0x1BD11BDAA9FC1A22

static const __constant ulong SKEIN256_IV[8] =
{
    0xCCD044A12FDB3E13UL, 0xE83590301A79A9EBUL,
    0x55AEA0614F816E6FUL, 0x2A2767A4AE9B94DBUL,
    0xEC06025E74DD7683UL, 0xE7A436CDC4746251UL,
    0xC36FBAF9393AD185UL, 0x3EEDBA1833EDFC13UL
};

static const __constant ulong SKEIN512_256_IV[8] =
{
    0xCCD044A12FDB3E13UL, 0xE83590301A79A9EBUL,
    0x55AEA0614F816E6FUL, 0x2A2767A4AE9B94DBUL,
    0xEC06025E74DD7683UL, 0xE7A436CDC4746251UL,
    0xC36FBAF9393AD185UL, 0x3EEDBA1833EDFC13UL
};

#define SKEIN_INJECT_KEY(p, s)	do { \
    p += h; \
    p.s5 += t[s % 3]; \
    p.s6 += t[(s + 1) % 3]; \
    p.s7 += s; \
} while(0)

ulong SKEIN_ROT(const uint2 x, const uint y)
{
    if(y < 32) return(as_ulong(amd_bitalign(x, x.s10, 32 - y)));
    else return(as_ulong(amd_bitalign(x.s10, x, 32 - (y - 32))));
}

void SkeinMix8(ulong4 *pv0, ulong4 *pv1, const uint rc0, const uint rc1, const uint rc2, const uint rc3)
{
    *pv0 += *pv1;
    (*pv1).s0 = SKEIN_ROT(as_uint2((*pv1).s0), rc0);
    (*pv1).s1 = SKEIN_ROT(as_uint2((*pv1).s1), rc1);
    (*pv1).s2 = SKEIN_ROT(as_uint2((*pv1).s2), rc2);
    (*pv1).s3 = SKEIN_ROT(as_uint2((*pv1).s3), rc3);
    *pv1 ^= *pv0;
}

ulong8 SkeinEvenRound(ulong8 p, const ulong8 h, const ulong *t, const uint s)
{
    SKEIN_INJECT_KEY(p, s);
    ulong4 pv0 = p.even, pv1 = p.odd;

    SkeinMix8(&pv0, &pv1, 46, 36, 19, 37);
    pv0 = shuffle(pv0, (ulong4)(1, 2, 3, 0));
    pv1 = shuffle(pv1, (ulong4)(0, 3, 2, 1));

    SkeinMix8(&pv0, &pv1, 33, 27, 14, 42);
    pv0 = shuffle(pv0, (ulong4)(1, 2, 3, 0));
    pv1 = shuffle(pv1, (ulong4)(0, 3, 2, 1));

    SkeinMix8(&pv0, &pv1, 17, 49, 36, 39);
    pv0 = shuffle(pv0, (ulong4)(1, 2, 3, 0));
    pv1 = shuffle(pv1, (ulong4)(0, 3, 2, 1));

    SkeinMix8(&pv0, &pv1, 44, 9, 54, 56);
    return(shuffle2(pv0, pv1, (ulong8)(1, 4, 2, 7, 3, 6, 0, 5)));
}

ulong8 SkeinOddRound(ulong8 p, const ulong8 h, const ulong *t, const uint s)
{
    SKEIN_INJECT_KEY(p, s);
    ulong4 pv0 = p.even, pv1 = p.odd;

    SkeinMix8(&pv0, &pv1, 39, 30, 34, 24);
    pv0 = shuffle(pv0, (ulong4)(1, 2, 3, 0));
    pv1 = shuffle(pv1, (ulong4)(0, 3, 2, 1));

    SkeinMix8(&pv0, &pv1, 13, 50, 10, 17);
    pv0 = shuffle(pv0, (ulong4)(1, 2, 3, 0));
    pv1 = shuffle(pv1, (ulong4)(0, 3, 2, 1));

    SkeinMix8(&pv0, &pv1, 25, 29, 39, 43);
    pv0 = shuffle(pv0, (ulong4)(1, 2, 3, 0));
    pv1 = shuffle(pv1, (ulong4)(0, 3, 2, 1));

    SkeinMix8(&pv0, &pv1, 8, 35, 56, 22);
    return(shuffle2(pv0, pv1, (ulong8)(1, 4, 2, 7, 3, 6, 0, 5)));
}

ulong8 Skein512Block(ulong8 p, ulong8 h, ulong h8, const ulong *t)
{
    #pragma unroll
    for(int i = 0; i < 18; ++i)
    {
        p = SkeinEvenRound(p, h, t, i);
        ++i;
        ulong tmp = h.s0;
        h = shuffle(h, (ulong8)(1, 2, 3, 4, 5, 6, 7, 0));
        h.s7 = h8;
        h8 = tmp;
        p = SkeinOddRound(p, h, t, i);
        tmp = h.s0;
        h = shuffle(h, (ulong8)(1, 2, 3, 4, 5, 6, 7, 0));
        h.s7 = h8;
        h8 = tmp;
    }

    SKEIN_INJECT_KEY(p, 18);
    return(p);
}

#endif

/* $Id: jh.c 255 2011-06-07 19:50:20Z tp $ */
/*
 * JH implementation.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#define SPH_JH_64   1
#define SPH_LITTLE_ENDIAN 1

#define SPH_C32(x)	x
#define SPH_C64(x)	x
typedef uint sph_u32;
typedef ulong sph_u64;

/*
 * The internal bitslice representation may use either big-endian or
 * little-endian (true bitslice operations do not care about the bit
 * ordering, and the bit-swapping linear operations in JH happen to
 * be invariant through endianness-swapping). The constants must be
 * defined according to the chosen endianness; we use some
 * byte-swapping macros for that.
 */

#if SPH_LITTLE_ENDIAN

#define C32e(x)     ((SPH_C32(x) >> 24) \
          | ((SPH_C32(x) >>  8) & SPH_C32(0x0000FF00)) \
          | ((SPH_C32(x) <<  8) & SPH_C32(0x00FF0000)) \
          | ((SPH_C32(x) << 24) & SPH_C32(0xFF000000)))
#define dec32e_aligned   sph_dec32le_aligned
#define enc32e           sph_enc32le

#define C64e(x)     ((SPH_C64(x) >> 56) \
          | ((SPH_C64(x) >> 40) & SPH_C64(0x000000000000FF00)) \
          | ((SPH_C64(x) >> 24) & SPH_C64(0x0000000000FF0000)) \
          | ((SPH_C64(x) >>  8) & SPH_C64(0x00000000FF000000)) \
          | ((SPH_C64(x) <<  8) & SPH_C64(0x000000FF00000000)) \
          | ((SPH_C64(x) << 24) & SPH_C64(0x0000FF0000000000)) \
          | ((SPH_C64(x) << 40) & SPH_C64(0x00FF000000000000)) \
          | ((SPH_C64(x) << 56) & SPH_C64(0xFF00000000000000)))
#define dec64e_aligned   sph_dec64le_aligned
#define enc64e           sph_enc64le

#else

#define C32e(x)     SPH_C32(x)
#define dec32e_aligned   sph_dec32be_aligned
#define enc32e           sph_enc32be
#define C64e(x)     SPH_C64(x)
#define dec64e_aligned   sph_dec64be_aligned
#define enc64e           sph_enc64be

#endif

#define Sb(x0, x1, x2, x3, c)   do { \
    x3 = ~x3; \
    x0 ^= (c) & ~x2; \
    tmp = (c) ^ (x0 & x1); \
    x0 ^= x2 & x3; \
    x3 ^= ~x1 & x2; \
    x1 ^= x0 & x2; \
    x2 ^= x0 & ~x3; \
    x0 ^= x1 | x3; \
    x3 ^= x1 & x2; \
    x1 ^= tmp & x0; \
    x2 ^= tmp; \
  } while (0)

#define Lb(x0, x1, x2, x3, x4, x5, x6, x7)   do { \
    x4 ^= x1; \
    x5 ^= x2; \
    x6 ^= x3 ^ x0; \
    x7 ^= x0; \
    x0 ^= x5; \
    x1 ^= x6; \
    x2 ^= x7 ^ x4; \
    x3 ^= x4; \
  } while (0)

static const __constant ulong C[] =
{
    0x67F815DFA2DED572UL, 0x571523B70A15847BUL, 0xF6875A4D90D6AB81UL, 0x402BD1C3C54F9F4EUL,
    0x9CFA455CE03A98EAUL, 0x9A99B26699D2C503UL, 0x8A53BBF2B4960266UL, 0x31A2DB881A1456B5UL,
    0xDB0E199A5C5AA303UL, 0x1044C1870AB23F40UL, 0x1D959E848019051CUL, 0xDCCDE75EADEB336FUL,
    0x416BBF029213BA10UL, 0xD027BBF7156578DCUL, 0x5078AA3739812C0AUL, 0xD3910041D2BF1A3FUL,
    0x907ECCF60D5A2D42UL, 0xCE97C0929C9F62DDUL, 0xAC442BC70BA75C18UL, 0x23FCC663D665DFD1UL,
    0x1AB8E09E036C6E97UL, 0xA8EC6C447E450521UL, 0xFA618E5DBB03F1EEUL, 0x97818394B29796FDUL,
    0x2F3003DB37858E4AUL, 0x956A9FFB2D8D672AUL, 0x6C69B8F88173FE8AUL, 0x14427FC04672C78AUL,
    0xC45EC7BD8F15F4C5UL, 0x80BB118FA76F4475UL, 0xBC88E4AEB775DE52UL, 0xF4A3A6981E00B882UL,
    0x1563A3A9338FF48EUL, 0x89F9B7D524565FAAUL, 0xFDE05A7C20EDF1B6UL, 0x362C42065AE9CA36UL,
    0x3D98FE4E433529CEUL, 0xA74B9A7374F93A53UL, 0x86814E6F591FF5D0UL, 0x9F5AD8AF81AD9D0EUL,
    0x6A6234EE670605A7UL, 0x2717B96EBE280B8BUL, 0x3F1080C626077447UL, 0x7B487EC66F7EA0E0UL,
    0xC0A4F84AA50A550DUL, 0x9EF18E979FE7E391UL, 0xD48D605081727686UL, 0x62B0E5F3415A9E7EUL,
    0x7A205440EC1F9FFCUL, 0x84C9F4CE001AE4E3UL, 0xD895FA9DF594D74FUL, 0xA554C324117E2E55UL,
    0x286EFEBD2872DF5BUL, 0xB2C4A50FE27FF578UL, 0x2ED349EEEF7C8905UL, 0x7F5928EB85937E44UL,
    0x4A3124B337695F70UL, 0x65E4D61DF128865EUL, 0xE720B95104771BC7UL, 0x8A87D423E843FE74UL,
    0xF2947692A3E8297DUL, 0xC1D9309B097ACBDDUL, 0xE01BDC5BFB301B1DUL, 0xBF829CF24F4924DAUL,
    0xFFBF70B431BAE7A4UL, 0x48BCF8DE0544320DUL, 0x39D3BB5332FCAE3BUL, 0xA08B29E0C1C39F45UL,
    0x0F09AEF7FD05C9E5UL, 0x34F1904212347094UL, 0x95ED44E301B771A2UL, 0x4A982F4F368E3BE9UL,
    0x15F66CA0631D4088UL, 0xFFAF52874B44C147UL, 0x30C60AE2F14ABB7EUL, 0xE68C6ECCC5B67046UL,
    0x00CA4FBD56A4D5A4UL, 0xAE183EC84B849DDAUL, 0xADD1643045CE5773UL, 0x67255C1468CEA6E8UL,
    0x16E10ECBF28CDAA3UL, 0x9A99949A5806E933UL, 0x7B846FC220B2601FUL, 0x1885D1A07FACCED1UL,
    0xD319DD8DA15B5932UL, 0x46B4A5AAC01C9A50UL, 0xBA6B04E467633D9FUL, 0x7EEE560BAB19CAF6UL,
    0x742128A9EA79B11FUL, 0xEE51363B35F7BDE9UL, 0x76D350755AAC571DUL, 0x01707DA3FEC2463AUL,
    0x42D8A498AFC135F7UL, 0x79676B9E20ECED78UL, 0xA8DB3AEA15638341UL, 0x832C83324D3BC3FAUL,
    0xF347271C1F3B40A7UL, 0x9A762DB734F04059UL, 0xFD4F21D26C4E3EE7UL, 0xEF5957DC398DFDB8UL,
    0xDAEB492B490C9B8DUL, 0x0D70F36849D7A25BUL, 0x84558D7AD0AE3B7DUL, 0x658EF8E4F0E9A5F5UL,
    0x533B1036F4A2B8A0UL, 0x5AEC3E759E07A80CUL, 0x4F88E85692946891UL, 0x4CBCBAF8555CB05BUL,
    0x7B9487F3993BBBE3UL, 0x5D1C6B72D6F4DA75UL, 0x6DB334DC28ACAE64UL, 0x71DB28B850A5346CUL,
    0x2A518D10F2E261F8UL, 0xFC75DD593364DBE3UL, 0xA23FCE43F1BCAC1CUL, 0xB043E8023CD1BB67UL,
    0x75A12988CA5B0A33UL, 0x5C5316B44D19347FUL, 0x1E4D790EC3943B92UL, 0x3FAFEEB6D7757479UL,
    0x21391ABEF7D4A8EAUL, 0x5127234C097EF45CUL, 0xD23C32BA5324A326UL, 0xADD5A66D4A17A344UL,
    0x08C9F2AFA63E1DB5UL, 0x563C6B91983D5983UL, 0x4D608672A17CF84CUL, 0xF6C76E08CC3EE246UL,
    0x5E76BCB1B333982FUL, 0x2AE6C4EFA566D62BUL, 0x36D4C1BEE8B6F406UL, 0x6321EFBC1582EE74UL,
    0x69C953F40D4EC1FDUL, 0x26585806C45A7DA7UL, 0x16FAE0061614C17EUL, 0x3F9D63283DAF907EUL,
    0x0CD29B00E3F2C9D2UL, 0x300CD4B730CEAA5FUL, 0x9832E0F216512A74UL, 0x9AF8CEE3D830EB0DUL,
    0x9279F1B57B9EC54BUL, 0xD36886046EE651FFUL, 0x316796E6574D239BUL, 0x05750A17F3A6E6CCUL,
    0xCE6C3213D98176B1UL, 0x62A205F88452173CUL, 0x47154778B3CB2BF4UL, 0x486A9323825446FFUL,
    0x65655E4E0758DF38UL, 0x8E5086FC897CFCF2UL, 0x86CA0BD0442E7031UL, 0x4E477830A20940F0UL,
    0x8338F7D139EEA065UL, 0xBD3A2CE437E95EF7UL, 0x6FF8130126B29721UL, 0xE7DE9FEFD1ED44A3UL,
    0xD992257615DFA08BUL, 0xBE42DC12F6F7853CUL, 0x7EB027AB7CECA7D8UL, 0xDEA83EAADA7D8D53UL,
    0xD86902BD93CE25AAUL, 0xF908731AFD43F65AUL, 0xA5194A17DAEF5FC0UL, 0x6A21FD4C33664D97UL,
    0x701541DB3198B435UL, 0x9B54CDEDBB0F1EEAUL, 0x72409751A163D09AUL, 0xE26F4791BF9D75F6UL
};

#define Ceven_hi(r)   (C[((r) << 2) + 0])
#define Ceven_lo(r)   (C[((r) << 2) + 1])
#define Codd_hi(r)    (C[((r) << 2) + 2])
#define Codd_lo(r)    (C[((r) << 2) + 3])

#define S(x0, x1, x2, x3, cb, r)   do { \
    Sb(x0 ## h, x1 ## h, x2 ## h, x3 ## h, cb ## hi(r)); \
    Sb(x0 ## l, x1 ## l, x2 ## l, x3 ## l, cb ## lo(r)); \
  } while (0)

#define L(x0, x1, x2, x3, x4, x5, x6, x7)   do { \
    Lb(x0 ## h, x1 ## h, x2 ## h, x3 ## h, \
      x4 ## h, x5 ## h, x6 ## h, x7 ## h); \
    Lb(x0 ## l, x1 ## l, x2 ## l, x3 ## l, \
      x4 ## l, x5 ## l, x6 ## l, x7 ## l); \
  } while (0)

#define Wz(x, c, n)   do { \
    sph_u64 t = (x ## h & (c)) << (n); \
    x ## h = ((x ## h >> (n)) & (c)) | t; \
    t = (x ## l & (c)) << (n); \
    x ## l = ((x ## l >> (n)) & (c)) | t; \
  } while (0)

#define W0(x)   Wz(x, SPH_C64(0x5555555555555555),  1)
#define W1(x)   Wz(x, SPH_C64(0x3333333333333333),  2)
#define W2(x)   Wz(x, SPH_C64(0x0F0F0F0F0F0F0F0F),  4)
#define W3(x)   Wz(x, SPH_C64(0x00FF00FF00FF00FF),  8)
#define W4(x)   Wz(x, SPH_C64(0x0000FFFF0000FFFF), 16)
#define W5(x)   Wz(x, SPH_C64(0x00000000FFFFFFFF), 32)
#define W6(x)   do { \
    sph_u64 t = x ## h; \
    x ## h = x ## l; \
    x ## l = t; \
  } while (0)

#define SL(ro)   SLu(r + ro, ro)

#define SLu(r, ro)   do { \
    S(h0, h2, h4, h6, Ceven_, r); \
    S(h1, h3, h5, h7, Codd_, r); \
    L(h0, h2, h4, h6, h1, h3, h5, h7); \
    W ## ro(h1); \
    W ## ro(h3); \
    W ## ro(h5); \
    W ## ro(h7); \
  } while (0)

#if SPH_SMALL_FOOTPRINT_JH

/*
 * The "small footprint" 64-bit version just uses a partially unrolled
 * loop.
 */

#define E8   do { \
    unsigned r; \
    for (r = 0; r < 42; r += 7) { \
      SL(0); \
      SL(1); \
      SL(2); \
      SL(3); \
      SL(4); \
      SL(5); \
      SL(6); \
    } \
  } while (0)

#else

/*
 * On a "true 64-bit" architecture, we can unroll at will.
 */

#define E8   do { \
    SLu( 0, 0); \
    SLu( 1, 1); \
    SLu( 2, 2); \
    SLu( 3, 3); \
    SLu( 4, 4); \
    SLu( 5, 5); \
    SLu( 6, 6); \
    SLu( 7, 0); \
    SLu( 8, 1); \
    SLu( 9, 2); \
    SLu(10, 3); \
    SLu(11, 4); \
    SLu(12, 5); \
    SLu(13, 6); \
    SLu(14, 0); \
    SLu(15, 1); \
    SLu(16, 2); \
    SLu(17, 3); \
    SLu(18, 4); \
    SLu(19, 5); \
    SLu(20, 6); \
    SLu(21, 0); \
    SLu(22, 1); \
    SLu(23, 2); \
    SLu(24, 3); \
    SLu(25, 4); \
    SLu(26, 5); \
    SLu(27, 6); \
    SLu(28, 0); \
    SLu(29, 1); \
    SLu(30, 2); \
    SLu(31, 3); \
    SLu(32, 4); \
    SLu(33, 5); \
    SLu(34, 6); \
    SLu(35, 0); \
    SLu(36, 1); \
    SLu(37, 2); \
    SLu(38, 3); \
    SLu(39, 4); \
    SLu(40, 5); \
    SLu(41, 6); \
  } while (0)

#endif

/*
* blake256 kernel implementation.
*
* ==========================(LICENSE BEGIN)============================
* Copyright (c) 2014 djm34
* Copyright (c) 2014 tpruvot
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* ===========================(LICENSE END)=============================
*
* @author   djm34
*/
__constant static const int sigma[16][16] = {
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
        { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
        { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
        { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
        { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
        { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
        { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
        { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
        { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
        { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
        { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
        { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
        { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
        { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
        { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 }
};


__constant static const sph_u32  c_IV256[8] = {
    0x6A09E667, 0xBB67AE85,
    0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C,
    0x1F83D9AB, 0x5BE0CD19
};

/* Second part (64-80) msg never change, store it */
__constant static const sph_u32  c_Padding[16] = {
    0, 0, 0, 0,
    0x80000000, 0, 0, 0,
    0, 0, 0, 0,
    0, 1, 0, 640,
};
__constant static const sph_u32  c_u256[16] = {
    0x243F6A88, 0x85A308D3,
    0x13198A2E, 0x03707344,
    0xA4093822, 0x299F31D0,
    0x082EFA98, 0xEC4E6C89,
    0x452821E6, 0x38D01377,
    0xBE5466CF, 0x34E90C6C,
    0xC0AC29B7, 0xC97C50DD,
    0x3F84D5B5, 0xB5470917
};

#define GS(a,b,c,d,x) { \
    const sph_u32 idx1 = sigma[r][x]; \
    const sph_u32 idx2 = sigma[r][x+1]; \
    v[a] += (m[idx1] ^ c_u256[idx2]) + v[b]; \
    v[d] ^= v[a]; \
    v[d] = rotate(v[d], 16U); \
    v[c] += v[d]; \
    v[b] ^= v[c]; \
    v[b] = rotate(v[b], 20U); \
\
    v[a] += (m[idx2] ^ c_u256[idx1]) + v[b]; \
    v[d] ^= v[a]; \
    v[d] = rotate(v[d], 24U); \
    v[c] += v[d]; \
    v[b] ^= v[c]; \
    v[b] = rotate(v[b], 25U); \
}

/* $Id: groestl.c 260 2011-07-21 01:02:38Z tp $ */
/*
 * Groestl256
 *
 * ==========================(LICENSE BEGIN)============================
 * Copyright (c) 2014 djm34
 * Copyright (c) 2007-2010  Projet RNRT SAPHIR
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@cryptolog.com>
 */

#define SPH_C64(x)	x
#define SPH_ROTL64(x, y)	rotate((x), (ulong)(y))


#define C64e(x)     ((SPH_C64(x) >> 56) \
                    | ((SPH_C64(x) >> 40) & SPH_C64(0x000000000000FF00)) \
                    | ((SPH_C64(x) >> 24) & SPH_C64(0x0000000000FF0000)) \
                    | ((SPH_C64(x) >>  8) & SPH_C64(0x00000000FF000000)) \
                    | ((SPH_C64(x) <<  8) & SPH_C64(0x000000FF00000000)) \
                    | ((SPH_C64(x) << 24) & SPH_C64(0x0000FF0000000000)) \
                    | ((SPH_C64(x) << 40) & SPH_C64(0x00FF000000000000)) \
                    | ((SPH_C64(x) << 56) & SPH_C64(0xFF00000000000000)))

#define B64_0(x)    ((x) & 0xFF)
#define B64_1(x)    (((x) >> 8) & 0xFF)
#define B64_2(x)    (((x) >> 16) & 0xFF)
#define B64_3(x)    (((x) >> 24) & 0xFF)
#define B64_4(x)    (((x) >> 32) & 0xFF)
#define B64_5(x)    (((x) >> 40) & 0xFF)
#define B64_6(x)    (((x) >> 48) & 0xFF)
#define B64_7(x)    ((x) >> 56)
#define R64         SPH_ROTL64
#define PC64(j, r)  ((sph_u64)((j) + (r)))
#define QC64(j, r)  (((sph_u64)(r) << 56) ^ (~((sph_u64)(j) << 56)))

static const __constant ulong T0_G[] =
{
    0xc6a597f4a5f432c6UL, 0xf884eb9784976ff8UL, 0xee99c7b099b05eeeUL, 0xf68df78c8d8c7af6UL,
    0xff0de5170d17e8ffUL, 0xd6bdb7dcbddc0ad6UL, 0xdeb1a7c8b1c816deUL, 0x915439fc54fc6d91UL,
    0x6050c0f050f09060UL, 0x0203040503050702UL, 0xcea987e0a9e02eceUL, 0x567dac877d87d156UL,
    0xe719d52b192bcce7UL, 0xb56271a662a613b5UL, 0x4de69a31e6317c4dUL, 0xec9ac3b59ab559ecUL,
    0x8f4505cf45cf408fUL, 0x1f9d3ebc9dbca31fUL, 0x894009c040c04989UL, 0xfa87ef92879268faUL,
    0xef15c53f153fd0efUL, 0xb2eb7f26eb2694b2UL, 0x8ec90740c940ce8eUL, 0xfb0bed1d0b1de6fbUL,
    0x41ec822fec2f6e41UL, 0xb3677da967a91ab3UL, 0x5ffdbe1cfd1c435fUL, 0x45ea8a25ea256045UL,
    0x23bf46dabfdaf923UL, 0x53f7a602f7025153UL, 0xe496d3a196a145e4UL, 0x9b5b2ded5bed769bUL,
    0x75c2ea5dc25d2875UL, 0xe11cd9241c24c5e1UL, 0x3dae7ae9aee9d43dUL, 0x4c6a98be6abef24cUL,
    0x6c5ad8ee5aee826cUL, 0x7e41fcc341c3bd7eUL, 0xf502f1060206f3f5UL, 0x834f1dd14fd15283UL,
    0x685cd0e45ce48c68UL, 0x51f4a207f4075651UL, 0xd134b95c345c8dd1UL, 0xf908e9180818e1f9UL,
    0xe293dfae93ae4ce2UL, 0xab734d9573953eabUL, 0x6253c4f553f59762UL, 0x2a3f54413f416b2aUL,
    0x080c10140c141c08UL, 0x955231f652f66395UL, 0x46658caf65afe946UL, 0x9d5e21e25ee27f9dUL,
    0x3028607828784830UL, 0x37a16ef8a1f8cf37UL, 0x0a0f14110f111b0aUL, 0x2fb55ec4b5c4eb2fUL,
    0x0e091c1b091b150eUL, 0x2436485a365a7e24UL, 0x1b9b36b69bb6ad1bUL, 0xdf3da5473d4798dfUL,
    0xcd26816a266aa7cdUL, 0x4e699cbb69bbf54eUL, 0x7fcdfe4ccd4c337fUL, 0xea9fcfba9fba50eaUL,
    0x121b242d1b2d3f12UL, 0x1d9e3ab99eb9a41dUL, 0x5874b09c749cc458UL, 0x342e68722e724634UL,
    0x362d6c772d774136UL, 0xdcb2a3cdb2cd11dcUL, 0xb4ee7329ee299db4UL, 0x5bfbb616fb164d5bUL,
    0xa4f65301f601a5a4UL, 0x764decd74dd7a176UL, 0xb76175a361a314b7UL, 0x7dcefa49ce49347dUL,
    0x527ba48d7b8ddf52UL, 0xdd3ea1423e429fddUL, 0x5e71bc937193cd5eUL, 0x139726a297a2b113UL,
    0xa6f55704f504a2a6UL, 0xb96869b868b801b9UL, 0x0000000000000000UL, 0xc12c99742c74b5c1UL,
    0x406080a060a0e040UL, 0xe31fdd211f21c2e3UL, 0x79c8f243c8433a79UL, 0xb6ed772ced2c9ab6UL,
    0xd4beb3d9bed90dd4UL, 0x8d4601ca46ca478dUL, 0x67d9ce70d9701767UL, 0x724be4dd4bddaf72UL,
    0x94de3379de79ed94UL, 0x98d42b67d467ff98UL, 0xb0e87b23e82393b0UL, 0x854a11de4ade5b85UL,
    0xbb6b6dbd6bbd06bbUL, 0xc52a917e2a7ebbc5UL, 0x4fe59e34e5347b4fUL, 0xed16c13a163ad7edUL,
    0x86c51754c554d286UL, 0x9ad72f62d762f89aUL, 0x6655ccff55ff9966UL, 0x119422a794a7b611UL,
    0x8acf0f4acf4ac08aUL, 0xe910c9301030d9e9UL, 0x0406080a060a0e04UL, 0xfe81e798819866feUL,
    0xa0f05b0bf00baba0UL, 0x7844f0cc44ccb478UL, 0x25ba4ad5bad5f025UL, 0x4be3963ee33e754bUL,
    0xa2f35f0ef30eaca2UL, 0x5dfeba19fe19445dUL, 0x80c01b5bc05bdb80UL, 0x058a0a858a858005UL,
    0x3fad7eecadecd33fUL, 0x21bc42dfbcdffe21UL, 0x7048e0d848d8a870UL, 0xf104f90c040cfdf1UL,
    0x63dfc67adf7a1963UL, 0x77c1ee58c1582f77UL, 0xaf75459f759f30afUL, 0x426384a563a5e742UL,
    0x2030405030507020UL, 0xe51ad12e1a2ecbe5UL, 0xfd0ee1120e12effdUL, 0xbf6d65b76db708bfUL,
    0x814c19d44cd45581UL, 0x1814303c143c2418UL, 0x26354c5f355f7926UL, 0xc32f9d712f71b2c3UL,
    0xbee16738e13886beUL, 0x35a26afda2fdc835UL, 0x88cc0b4fcc4fc788UL, 0x2e395c4b394b652eUL,
    0x93573df957f96a93UL, 0x55f2aa0df20d5855UL, 0xfc82e39d829d61fcUL, 0x7a47f4c947c9b37aUL,
    0xc8ac8befacef27c8UL, 0xbae76f32e73288baUL, 0x322b647d2b7d4f32UL, 0xe695d7a495a442e6UL,
    0xc0a09bfba0fb3bc0UL, 0x199832b398b3aa19UL, 0x9ed12768d168f69eUL, 0xa37f5d817f8122a3UL,
    0x446688aa66aaee44UL, 0x547ea8827e82d654UL, 0x3bab76e6abe6dd3bUL, 0x0b83169e839e950bUL,
    0x8cca0345ca45c98cUL, 0xc729957b297bbcc7UL, 0x6bd3d66ed36e056bUL, 0x283c50443c446c28UL,
    0xa779558b798b2ca7UL, 0xbce2633de23d81bcUL, 0x161d2c271d273116UL, 0xad76419a769a37adUL,
    0xdb3bad4d3b4d96dbUL, 0x6456c8fa56fa9e64UL, 0x744ee8d24ed2a674UL, 0x141e28221e223614UL,
    0x92db3f76db76e492UL, 0x0c0a181e0a1e120cUL, 0x486c90b46cb4fc48UL, 0xb8e46b37e4378fb8UL,
    0x9f5d25e75de7789fUL, 0xbd6e61b26eb20fbdUL, 0x43ef862aef2a6943UL, 0xc4a693f1a6f135c4UL,
    0x39a872e3a8e3da39UL, 0x31a462f7a4f7c631UL, 0xd337bd5937598ad3UL, 0xf28bff868b8674f2UL,
    0xd532b156325683d5UL, 0x8b430dc543c54e8bUL, 0x6e59dceb59eb856eUL, 0xdab7afc2b7c218daUL,
    0x018c028f8c8f8e01UL, 0xb16479ac64ac1db1UL, 0x9cd2236dd26df19cUL, 0x49e0923be03b7249UL,
    0xd8b4abc7b4c71fd8UL, 0xacfa4315fa15b9acUL, 0xf307fd090709faf3UL, 0xcf25856f256fa0cfUL,
    0xcaaf8feaafea20caUL, 0xf48ef3898e897df4UL, 0x47e98e20e9206747UL, 0x1018202818283810UL,
    0x6fd5de64d5640b6fUL, 0xf088fb83888373f0UL, 0x4a6f94b16fb1fb4aUL, 0x5c72b8967296ca5cUL,
    0x3824706c246c5438UL, 0x57f1ae08f1085f57UL, 0x73c7e652c7522173UL, 0x975135f351f36497UL,
    0xcb238d652365aecbUL, 0xa17c59847c8425a1UL, 0xe89ccbbf9cbf57e8UL, 0x3e217c6321635d3eUL,
    0x96dd377cdd7cea96UL, 0x61dcc27fdc7f1e61UL, 0x0d861a9186919c0dUL, 0x0f851e9485949b0fUL,
    0xe090dbab90ab4be0UL, 0x7c42f8c642c6ba7cUL, 0x71c4e257c4572671UL, 0xccaa83e5aae529ccUL,
    0x90d83b73d873e390UL, 0x06050c0f050f0906UL, 0xf701f5030103f4f7UL, 0x1c12383612362a1cUL,
    0xc2a39ffea3fe3cc2UL, 0x6a5fd4e15fe18b6aUL, 0xaef94710f910beaeUL, 0x69d0d26bd06b0269UL,
    0x17912ea891a8bf17UL, 0x995829e858e87199UL, 0x3a2774692769533aUL, 0x27b94ed0b9d0f727UL,
    0xd938a948384891d9UL, 0xeb13cd351335deebUL, 0x2bb356ceb3cee52bUL, 0x2233445533557722UL,
    0xd2bbbfd6bbd604d2UL, 0xa9704990709039a9UL, 0x07890e8089808707UL, 0x33a766f2a7f2c133UL,
    0x2db65ac1b6c1ec2dUL, 0x3c22786622665a3cUL, 0x15922aad92adb815UL, 0xc92089602060a9c9UL,
    0x874915db49db5c87UL, 0xaaff4f1aff1ab0aaUL, 0x5078a0887888d850UL, 0xa57a518e7a8e2ba5UL,
    0x038f068a8f8a8903UL, 0x59f8b213f8134a59UL, 0x0980129b809b9209UL, 0x1a1734391739231aUL,
    0x65daca75da751065UL, 0xd731b553315384d7UL, 0x84c61351c651d584UL, 0xd0b8bbd3b8d303d0UL,
    0x82c31f5ec35edc82UL, 0x29b052cbb0cbe229UL, 0x5a77b4997799c35aUL, 0x1e113c3311332d1eUL,
    0x7bcbf646cb463d7bUL, 0xa8fc4b1ffc1fb7a8UL, 0x6dd6da61d6610c6dUL, 0x2c3a584e3a4e622cUL
};

static const __constant ulong T4_G[] =
{
    0xA5F432C6C6A597F4UL, 0x84976FF8F884EB97UL, 0x99B05EEEEE99C7B0UL, 0x8D8C7AF6F68DF78CUL,
    0x0D17E8FFFF0DE517UL, 0xBDDC0AD6D6BDB7DCUL, 0xB1C816DEDEB1A7C8UL, 0x54FC6D91915439FCUL,
    0x50F090606050C0F0UL, 0x0305070202030405UL, 0xA9E02ECECEA987E0UL, 0x7D87D156567DAC87UL,
    0x192BCCE7E719D52BUL, 0x62A613B5B56271A6UL, 0xE6317C4D4DE69A31UL, 0x9AB559ECEC9AC3B5UL,
    0x45CF408F8F4505CFUL, 0x9DBCA31F1F9D3EBCUL, 0x40C04989894009C0UL, 0x879268FAFA87EF92UL,
    0x153FD0EFEF15C53FUL, 0xEB2694B2B2EB7F26UL, 0xC940CE8E8EC90740UL, 0x0B1DE6FBFB0BED1DUL,
    0xEC2F6E4141EC822FUL, 0x67A91AB3B3677DA9UL, 0xFD1C435F5FFDBE1CUL, 0xEA25604545EA8A25UL,
    0xBFDAF92323BF46DAUL, 0xF702515353F7A602UL, 0x96A145E4E496D3A1UL, 0x5BED769B9B5B2DEDUL,
    0xC25D287575C2EA5DUL, 0x1C24C5E1E11CD924UL, 0xAEE9D43D3DAE7AE9UL, 0x6ABEF24C4C6A98BEUL,
    0x5AEE826C6C5AD8EEUL, 0x41C3BD7E7E41FCC3UL, 0x0206F3F5F502F106UL, 0x4FD15283834F1DD1UL,
    0x5CE48C68685CD0E4UL, 0xF407565151F4A207UL, 0x345C8DD1D134B95CUL, 0x0818E1F9F908E918UL,
    0x93AE4CE2E293DFAEUL, 0x73953EABAB734D95UL, 0x53F597626253C4F5UL, 0x3F416B2A2A3F5441UL,
    0x0C141C08080C1014UL, 0x52F66395955231F6UL, 0x65AFE94646658CAFUL, 0x5EE27F9D9D5E21E2UL,
    0x2878483030286078UL, 0xA1F8CF3737A16EF8UL, 0x0F111B0A0A0F1411UL, 0xB5C4EB2F2FB55EC4UL,
    0x091B150E0E091C1BUL, 0x365A7E242436485AUL, 0x9BB6AD1B1B9B36B6UL, 0x3D4798DFDF3DA547UL,
    0x266AA7CDCD26816AUL, 0x69BBF54E4E699CBBUL, 0xCD4C337F7FCDFE4CUL, 0x9FBA50EAEA9FCFBAUL,
    0x1B2D3F12121B242DUL, 0x9EB9A41D1D9E3AB9UL, 0x749CC4585874B09CUL, 0x2E724634342E6872UL,
    0x2D774136362D6C77UL, 0xB2CD11DCDCB2A3CDUL, 0xEE299DB4B4EE7329UL, 0xFB164D5B5BFBB616UL,
    0xF601A5A4A4F65301UL, 0x4DD7A176764DECD7UL, 0x61A314B7B76175A3UL, 0xCE49347D7DCEFA49UL,
    0x7B8DDF52527BA48DUL, 0x3E429FDDDD3EA142UL, 0x7193CD5E5E71BC93UL, 0x97A2B113139726A2UL,
    0xF504A2A6A6F55704UL, 0x68B801B9B96869B8UL, 0x0000000000000000UL, 0x2C74B5C1C12C9974UL,
    0x60A0E040406080A0UL, 0x1F21C2E3E31FDD21UL, 0xC8433A7979C8F243UL, 0xED2C9AB6B6ED772CUL,
    0xBED90DD4D4BEB3D9UL, 0x46CA478D8D4601CAUL, 0xD970176767D9CE70UL, 0x4BDDAF72724BE4DDUL,
    0xDE79ED9494DE3379UL, 0xD467FF9898D42B67UL, 0xE82393B0B0E87B23UL, 0x4ADE5B85854A11DEUL,
    0x6BBD06BBBB6B6DBDUL, 0x2A7EBBC5C52A917EUL, 0xE5347B4F4FE59E34UL, 0x163AD7EDED16C13AUL,
    0xC554D28686C51754UL, 0xD762F89A9AD72F62UL, 0x55FF99666655CCFFUL, 0x94A7B611119422A7UL,
    0xCF4AC08A8ACF0F4AUL, 0x1030D9E9E910C930UL, 0x060A0E040406080AUL, 0x819866FEFE81E798UL,
    0xF00BABA0A0F05B0BUL, 0x44CCB4787844F0CCUL, 0xBAD5F02525BA4AD5UL, 0xE33E754B4BE3963EUL,
    0xF30EACA2A2F35F0EUL, 0xFE19445D5DFEBA19UL, 0xC05BDB8080C01B5BUL, 0x8A858005058A0A85UL,
    0xADECD33F3FAD7EECUL, 0xBCDFFE2121BC42DFUL, 0x48D8A8707048E0D8UL, 0x040CFDF1F104F90CUL,
    0xDF7A196363DFC67AUL, 0xC1582F7777C1EE58UL, 0x759F30AFAF75459FUL, 0x63A5E742426384A5UL,
    0x3050702020304050UL, 0x1A2ECBE5E51AD12EUL, 0x0E12EFFDFD0EE112UL, 0x6DB708BFBF6D65B7UL,
    0x4CD45581814C19D4UL, 0x143C24181814303CUL, 0x355F792626354C5FUL, 0x2F71B2C3C32F9D71UL,
    0xE13886BEBEE16738UL, 0xA2FDC83535A26AFDUL, 0xCC4FC78888CC0B4FUL, 0x394B652E2E395C4BUL,
    0x57F96A9393573DF9UL, 0xF20D585555F2AA0DUL, 0x829D61FCFC82E39DUL, 0x47C9B37A7A47F4C9UL,
    0xACEF27C8C8AC8BEFUL, 0xE73288BABAE76F32UL, 0x2B7D4F32322B647DUL, 0x95A442E6E695D7A4UL,
    0xA0FB3BC0C0A09BFBUL, 0x98B3AA19199832B3UL, 0xD168F69E9ED12768UL, 0x7F8122A3A37F5D81UL,
    0x66AAEE44446688AAUL, 0x7E82D654547EA882UL, 0xABE6DD3B3BAB76E6UL, 0x839E950B0B83169EUL,
    0xCA45C98C8CCA0345UL, 0x297BBCC7C729957BUL, 0xD36E056B6BD3D66EUL, 0x3C446C28283C5044UL,
    0x798B2CA7A779558BUL, 0xE23D81BCBCE2633DUL, 0x1D273116161D2C27UL, 0x769A37ADAD76419AUL,
    0x3B4D96DBDB3BAD4DUL, 0x56FA9E646456C8FAUL, 0x4ED2A674744EE8D2UL, 0x1E223614141E2822UL,
    0xDB76E49292DB3F76UL, 0x0A1E120C0C0A181EUL, 0x6CB4FC48486C90B4UL, 0xE4378FB8B8E46B37UL,
    0x5DE7789F9F5D25E7UL, 0x6EB20FBDBD6E61B2UL, 0xEF2A694343EF862AUL, 0xA6F135C4C4A693F1UL,
    0xA8E3DA3939A872E3UL, 0xA4F7C63131A462F7UL, 0x37598AD3D337BD59UL, 0x8B8674F2F28BFF86UL,
    0x325683D5D532B156UL, 0x43C54E8B8B430DC5UL, 0x59EB856E6E59DCEBUL, 0xB7C218DADAB7AFC2UL,
    0x8C8F8E01018C028FUL, 0x64AC1DB1B16479ACUL, 0xD26DF19C9CD2236DUL, 0xE03B724949E0923BUL,
    0xB4C71FD8D8B4ABC7UL, 0xFA15B9ACACFA4315UL, 0x0709FAF3F307FD09UL, 0x256FA0CFCF25856FUL,
    0xAFEA20CACAAF8FEAUL, 0x8E897DF4F48EF389UL, 0xE920674747E98E20UL, 0x1828381010182028UL,
    0xD5640B6F6FD5DE64UL, 0x888373F0F088FB83UL, 0x6FB1FB4A4A6F94B1UL, 0x7296CA5C5C72B896UL,
    0x246C54383824706CUL, 0xF1085F5757F1AE08UL, 0xC752217373C7E652UL, 0x51F36497975135F3UL,
    0x2365AECBCB238D65UL, 0x7C8425A1A17C5984UL, 0x9CBF57E8E89CCBBFUL, 0x21635D3E3E217C63UL,
    0xDD7CEA9696DD377CUL, 0xDC7F1E6161DCC27FUL, 0x86919C0D0D861A91UL, 0x85949B0F0F851E94UL,
    0x90AB4BE0E090DBABUL, 0x42C6BA7C7C42F8C6UL, 0xC457267171C4E257UL, 0xAAE529CCCCAA83E5UL,
    0xD873E39090D83B73UL, 0x050F090606050C0FUL, 0x0103F4F7F701F503UL, 0x12362A1C1C123836UL,
    0xA3FE3CC2C2A39FFEUL, 0x5FE18B6A6A5FD4E1UL, 0xF910BEAEAEF94710UL, 0xD06B026969D0D26BUL,
    0x91A8BF1717912EA8UL, 0x58E87199995829E8UL, 0x2769533A3A277469UL, 0xB9D0F72727B94ED0UL,
    0x384891D9D938A948UL, 0x1335DEEBEB13CD35UL, 0xB3CEE52B2BB356CEUL, 0x3355772222334455UL,
    0xBBD604D2D2BBBFD6UL, 0x709039A9A9704990UL, 0x8980870707890E80UL, 0xA7F2C13333A766F2UL,
    0xB6C1EC2D2DB65AC1UL, 0x22665A3C3C227866UL, 0x92ADB81515922AADUL, 0x2060A9C9C9208960UL,
    0x49DB5C87874915DBUL, 0xFF1AB0AAAAFF4F1AUL, 0x7888D8505078A088UL, 0x7A8E2BA5A57A518EUL,
    0x8F8A8903038F068AUL, 0xF8134A5959F8B213UL, 0x809B92090980129BUL, 0x1739231A1A173439UL,
    0xDA75106565DACA75UL, 0x315384D7D731B553UL, 0xC651D58484C61351UL, 0xB8D303D0D0B8BBD3UL,
    0xC35EDC8282C31F5EUL, 0xB0CBE22929B052CBUL, 0x7799C35A5A77B499UL, 0x11332D1E1E113C33UL,
    0xCB463D7B7BCBF646UL, 0xFC1FB7A8A8FC4B1FUL, 0xD6610C6D6DD6DA61UL, 0x3A4E622C2C3A584EUL
};

#define RSTT(d, a, b0, b1, b2, b3, b4, b5, b6, b7)   do { \
        t[d] = T0_G[B64_0(a[b0])] \
            ^ R64(T0_G[B64_1(a[b1])],  8) \
            ^ R64(T0_G[B64_2(a[b2])], 16) \
            ^ R64(T0_G[B64_3(a[b3])], 24) \
            ^ T4_G[B64_4(a[b4])] \
            ^ R64(T4_G[B64_5(a[b5])],  8) \
            ^ R64(T4_G[B64_6(a[b6])], 16) \
            ^ R64(T4_G[B64_7(a[b7])], 24); \
        } while (0)

#define ROUND_SMALL_P(a, r)   do { \
        ulong t[8]; \
        a[0] ^= PC64(0x00, r); \
        a[1] ^= PC64(0x10, r); \
        a[2] ^= PC64(0x20, r); \
        a[3] ^= PC64(0x30, r); \
        a[4] ^= PC64(0x40, r); \
        a[5] ^= PC64(0x50, r); \
        a[6] ^= PC64(0x60, r); \
        a[7] ^= PC64(0x70, r); \
        RSTT(0, a, 0, 1, 2, 3, 4, 5, 6, 7); \
        RSTT(1, a, 1, 2, 3, 4, 5, 6, 7, 0); \
        RSTT(2, a, 2, 3, 4, 5, 6, 7, 0, 1); \
        RSTT(3, a, 3, 4, 5, 6, 7, 0, 1, 2); \
        RSTT(4, a, 4, 5, 6, 7, 0, 1, 2, 3); \
        RSTT(5, a, 5, 6, 7, 0, 1, 2, 3, 4); \
        RSTT(6, a, 6, 7, 0, 1, 2, 3, 4, 5); \
        RSTT(7, a, 7, 0, 1, 2, 3, 4, 5, 6); \
        a[0] = t[0]; \
        a[1] = t[1]; \
        a[2] = t[2]; \
        a[3] = t[3]; \
        a[4] = t[4]; \
        a[5] = t[5]; \
        a[6] = t[6]; \
        a[7] = t[7]; \
        } while (0)

#define ROUND_SMALL_Pf(a,r)   do { \
        a[0] ^= PC64(0x00, r); \
        a[1] ^= PC64(0x10, r); \
        a[2] ^= PC64(0x20, r); \
        a[3] ^= PC64(0x30, r); \
        a[4] ^= PC64(0x40, r); \
        a[5] ^= PC64(0x50, r); \
        a[6] ^= PC64(0x60, r); \
        a[7] ^= PC64(0x70, r); \
        RSTT(7, a, 7, 0, 1, 2, 3, 4, 5, 6); \
        a[7] = t[7]; \
            } while (0)

#define ROUND_SMALL_Q(a, r)   do { \
        ulong t[8]; \
        a[0] ^= QC64(0x00, r); \
        a[1] ^= QC64(0x10, r); \
        a[2] ^= QC64(0x20, r); \
        a[3] ^= QC64(0x30, r); \
        a[4] ^= QC64(0x40, r); \
        a[5] ^= QC64(0x50, r); \
        a[6] ^= QC64(0x60, r); \
        a[7] ^= QC64(0x70, r); \
        RSTT(0, a, 1, 3, 5, 7, 0, 2, 4, 6); \
        RSTT(1, a, 2, 4, 6, 0, 1, 3, 5, 7); \
        RSTT(2, a, 3, 5, 7, 1, 2, 4, 6, 0); \
        RSTT(3, a, 4, 6, 0, 2, 3, 5, 7, 1); \
        RSTT(4, a, 5, 7, 1, 3, 4, 6, 0, 2); \
        RSTT(5, a, 6, 0, 2, 4, 5, 7, 1, 3); \
        RSTT(6, a, 7, 1, 3, 5, 6, 0, 2, 4); \
        RSTT(7, a, 0, 2, 4, 6, 7, 1, 3, 5); \
        a[0] = t[0]; \
        a[1] = t[1]; \
        a[2] = t[2]; \
        a[3] = t[3]; \
        a[4] = t[4]; \
        a[5] = t[5]; \
        a[6] = t[6]; \
        a[7] = t[7]; \
        } while (0)

#define PERM_SMALL_P(a)   do { \
        for (int r = 0; r < 10; r ++) \
            ROUND_SMALL_P(a, r); \
        } while (0)

#define PERM_SMALL_Pf(a)   do { \
        for (int r = 0; r < 9; r ++) { \
            ROUND_SMALL_P(a, r);} \
            ROUND_SMALL_Pf(a,9); \
            } while (0)

#define PERM_SMALL_Q(a)   do { \
        for (int r = 0; r < 10; r ++) \
            ROUND_SMALL_Q(a, r); \
        } while (0)


static const __constant ulong keccakf_rndc[24] =
{
    0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
    0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
    0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
    0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
    0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
    0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
    0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
    0x8000000000008080, 0x0000000080000001, 0x8000000080008008
};

static const __constant uchar sbox[256] =
{
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};


void keccakf1600(ulong *s)
{
    for(int i = 0; i < 24; ++i)
    {
        ulong bc[5], tmp1, tmp2;
        bc[0] = s[0] ^ s[5] ^ s[10] ^ s[15] ^ s[20] ^ rotate(s[2] ^ s[7] ^ s[12] ^ s[17] ^ s[22], 1UL);
        bc[1] = s[1] ^ s[6] ^ s[11] ^ s[16] ^ s[21] ^ rotate(s[3] ^ s[8] ^ s[13] ^ s[18] ^ s[23], 1UL);
        bc[2] = s[2] ^ s[7] ^ s[12] ^ s[17] ^ s[22] ^ rotate(s[4] ^ s[9] ^ s[14] ^ s[19] ^ s[24], 1UL);
        bc[3] = s[3] ^ s[8] ^ s[13] ^ s[18] ^ s[23] ^ rotate(s[0] ^ s[5] ^ s[10] ^ s[15] ^ s[20], 1UL);
        bc[4] = s[4] ^ s[9] ^ s[14] ^ s[19] ^ s[24] ^ rotate(s[1] ^ s[6] ^ s[11] ^ s[16] ^ s[21], 1UL);

        tmp1 = s[1] ^ bc[0];

        s[0] ^= bc[4];
        s[1] = rotate(s[6] ^ bc[0], 44UL);
        s[6] = rotate(s[9] ^ bc[3], 20UL);
        s[9] = rotate(s[22] ^ bc[1], 61UL);
        s[22] = rotate(s[14] ^ bc[3], 39UL);
        s[14] = rotate(s[20] ^ bc[4], 18UL);
        s[20] = rotate(s[2] ^ bc[1], 62UL);
        s[2] = rotate(s[12] ^ bc[1], 43UL);
        s[12] = rotate(s[13] ^ bc[2], 25UL);
        s[13] = rotate(s[19] ^ bc[3], 8UL);
        s[19] = rotate(s[23] ^ bc[2], 56UL);
        s[23] = rotate(s[15] ^ bc[4], 41UL);
        s[15] = rotate(s[4] ^ bc[3], 27UL);
        s[4] = rotate(s[24] ^ bc[3], 14UL);
        s[24] = rotate(s[21] ^ bc[0], 2UL);
        s[21] = rotate(s[8] ^ bc[2], 55UL);
        s[8] = rotate(s[16] ^ bc[0], 35UL);
        s[16] = rotate(s[5] ^ bc[4], 36UL);
        s[5] = rotate(s[3] ^ bc[2], 28UL);
        s[3] = rotate(s[18] ^ bc[2], 21UL);
        s[18] = rotate(s[17] ^ bc[1], 15UL);
        s[17] = rotate(s[11] ^ bc[0], 10UL);
        s[11] = rotate(s[7] ^ bc[1], 6UL);
        s[7] = rotate(s[10] ^ bc[4], 3UL);
        s[10] = rotate(tmp1, 1UL);

        tmp1 = s[0]; tmp2 = s[1]; s[0] = bitselect(s[0] ^ s[2], s[0], s[1]); s[1] = bitselect(s[1] ^ s[3], s[1], s[2]); s[2] = bitselect(s[2] ^ s[4], s[2], s[3]); s[3] = bitselect(s[3] ^ tmp1, s[3], s[4]); s[4] = bitselect(s[4] ^ tmp2, s[4], tmp1);
        tmp1 = s[5]; tmp2 = s[6]; s[5] = bitselect(s[5] ^ s[7], s[5], s[6]); s[6] = bitselect(s[6] ^ s[8], s[6], s[7]); s[7] = bitselect(s[7] ^ s[9], s[7], s[8]); s[8] = bitselect(s[8] ^ tmp1, s[8], s[9]); s[9] = bitselect(s[9] ^ tmp2, s[9], tmp1);
        tmp1 = s[10]; tmp2 = s[11]; s[10] = bitselect(s[10] ^ s[12], s[10], s[11]); s[11] = bitselect(s[11] ^ s[13], s[11], s[12]); s[12] = bitselect(s[12] ^ s[14], s[12], s[13]); s[13] = bitselect(s[13] ^ tmp1, s[13], s[14]); s[14] = bitselect(s[14] ^ tmp2, s[14], tmp1);
        tmp1 = s[15]; tmp2 = s[16]; s[15] = bitselect(s[15] ^ s[17], s[15], s[16]); s[16] = bitselect(s[16] ^ s[18], s[16], s[17]); s[17] = bitselect(s[17] ^ s[19], s[17], s[18]); s[18] = bitselect(s[18] ^ tmp1, s[18], s[19]); s[19] = bitselect(s[19] ^ tmp2, s[19], tmp1);
        tmp1 = s[20]; tmp2 = s[21]; s[20] = bitselect(s[20] ^ s[22], s[20], s[21]); s[21] = bitselect(s[21] ^ s[23], s[21], s[22]); s[22] = bitselect(s[22] ^ s[24], s[22], s[23]); s[23] = bitselect(s[23] ^ tmp1, s[23], s[24]); s[24] = bitselect(s[24] ^ tmp2, s[24], tmp1);
        s[0] ^= keccakf_rndc[i];
    }
}

static const __constant uint keccakf_rotc[24] =
{
    1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
    27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
};

static const __constant uint keccakf_piln[24] =
{
    10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
};

void keccakf1600_1(ulong *st)
{
    int i, round;
    ulong t, bc[5];

    #pragma unroll 1
    for(round = 0; round < 24; ++round)
    {

        // Theta
        bc[0] = st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20];
        bc[1] = st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21];
        bc[2] = st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22];
        bc[3] = st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23];
        bc[4] = st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24];

        #pragma unroll 1
        for (i = 0; i < 5; ++i) {
            t = bc[(i + 4) % 5] ^ rotate(bc[(i + 1) % 5], 1UL);
            st[i     ] ^= t;
            st[i +  5] ^= t;
            st[i + 10] ^= t;
            st[i + 15] ^= t;
            st[i + 20] ^= t;
        }

        // Rho Pi
        t = st[1];
        #pragma unroll
        for (i = 0; i < 24; ++i) {
            bc[0] = st[keccakf_piln[i]];
            st[keccakf_piln[i]] = rotate(t, (ulong)keccakf_rotc[i]);
            t = bc[0];
        }

        //ulong tmp1 = st[0]; ulong tmp2 = st[1]; st[0] = bitselect(st[0] ^ st[2], st[0], st[1]); st[1] = bitselect(st[1] ^ st[3], st[1], st[2]); st[2] = bitselect(st[2] ^ st[4], st[2], st[3]); st[3] = bitselect(st[3] ^ tmp1, st[3], st[4]); st[4] = bitselect(st[4] ^ tmp2, st[4], tmp1);
        //tmp1 = st[5]; tmp2 = st[6]; st[5] = bitselect(st[5] ^ st[7], st[5], st[6]); st[6] = bitselect(st[6] ^ st[8], st[6], st[7]); st[7] = bitselect(st[7] ^ st[9], st[7], st[8]); st[8] = bitselect(st[8] ^ tmp1, st[8], st[9]); st[9] = bitselect(st[9] ^ tmp2, st[9], tmp1);
        //tmp1 = st[10]; tmp2 = st[11]; st[10] = bitselect(st[10] ^ st[12], st[10], st[11]); st[11] = bitselect(st[11] ^ st[13], st[11], st[12]); st[12] = bitselect(st[12] ^ st[14], st[12], st[13]); st[13] = bitselect(st[13] ^ tmp1, st[13], st[14]); st[14] = bitselect(st[14] ^ tmp2, st[14], tmp1);
        //tmp1 = st[15]; tmp2 = st[16]; st[15] = bitselect(st[15] ^ st[17], st[15], st[16]); st[16] = bitselect(st[16] ^ st[18], st[16], st[17]); st[17] = bitselect(st[17] ^ st[19], st[17], st[18]); st[18] = bitselect(st[18] ^ tmp1, st[18], st[19]); st[19] = bitselect(st[19] ^ tmp2, st[19], tmp1);
        //tmp1 = st[20]; tmp2 = st[21]; st[20] = bitselect(st[20] ^ st[22], st[20], st[21]); st[21] = bitselect(st[21] ^ st[23], st[21], st[22]); st[22] = bitselect(st[22] ^ st[24], st[22], st[23]); st[23] = bitselect(st[23] ^ tmp1, st[23], st[24]); st[24] = bitselect(st[24] ^ tmp2, st[24], tmp1);

        #pragma unroll 1
        for(int i = 0; i < 25; i += 5)
        {
            ulong tmp[5];

            #pragma unroll 1
            for(int x = 0; x < 5; ++x)
                tmp[x] = bitselect(st[i + x] ^ st[i + ((x + 2) % 5)], st[i + x], st[i + ((x + 1) % 5)]);

            #pragma unroll 1
            for(int x = 0; x < 5; ++x) st[i + x] = tmp[x];
        }

        //  Iota
        st[0] ^= keccakf_rndc[round];
    }
}

void keccakf1600_2(ulong *st)
{
    int i, round;
    ulong t, bc[5];

    #pragma unroll 1
    for(round = 0; round < 24; ++round)
    {

        // Theta
        //bc[0] = st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20];
        //bc[1] = st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21];
        //bc[2] = st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22];
        //bc[3] = st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23];
        //bc[4] = st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24];

        /*
        #pragma unroll
        for (i = 0; i < 5; ++i) {
            t = bc[(i + 4) % 5] ^ rotate(bc[(i + 1) % 5], 1UL);
            st[i     ] ^= t;
            st[i +  5] ^= t;
            st[i + 10] ^= t;
            st[i + 15] ^= t;
            st[i + 20] ^= t;
        }
        */

        bc[0] = st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20] ^ rotate(st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22], 1UL);
        bc[1] = st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21] ^ rotate(st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23], 1UL);
        bc[2] = st[2] ^ st[7] ^ st[12] ^ st[17] ^ st[22] ^ rotate(st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24], 1UL);
        bc[3] = st[3] ^ st[8] ^ st[13] ^ st[18] ^ st[23] ^ rotate(st[0] ^ st[5] ^ st[10] ^ st[15] ^ st[20], 1UL);
        bc[4] = st[4] ^ st[9] ^ st[14] ^ st[19] ^ st[24] ^ rotate(st[1] ^ st[6] ^ st[11] ^ st[16] ^ st[21], 1UL);

        st[0] ^= bc[4];
        st[5] ^= bc[4];
        st[10] ^= bc[4];
        st[15] ^= bc[4];
        st[20] ^= bc[4];

        st[1] ^= bc[0];
        st[6] ^= bc[0];
        st[11] ^= bc[0];
        st[16] ^= bc[0];
        st[21] ^= bc[0];

        st[2] ^= bc[1];
        st[7] ^= bc[1];
        st[12] ^= bc[1];
        st[17] ^= bc[1];
        st[22] ^= bc[1];

        st[3] ^= bc[2];
        st[8] ^= bc[2];
        st[13] ^= bc[2];
        st[18] ^= bc[2];
        st[23] ^= bc[2];

        st[4] ^= bc[3];
        st[9] ^= bc[3];
        st[14] ^= bc[3];
        st[19] ^= bc[3];
        st[24] ^= bc[3];

        // Rho Pi
        t = st[1];
        #pragma unroll
        for (i = 0; i < 24; ++i) {
            bc[0] = st[keccakf_piln[i]];
            st[keccakf_piln[i]] = rotate(t, (ulong)keccakf_rotc[i]);
            t = bc[0];
        }



        /*ulong tmp1 = st[1] ^ bc[0];

        st[0] ^= bc[4];
        st[1] = rotate(st[6] ^ bc[0], 44UL);
        st[6] = rotate(st[9] ^ bc[3], 20UL);
        st[9] = rotate(st[22] ^ bc[1], 61UL);
        st[22] = rotate(st[14] ^ bc[3], 39UL);
        st[14] = rotate(st[20] ^ bc[4], 18UL);
        st[20] = rotate(st[2] ^ bc[1], 62UL);
        st[2] = rotate(st[12] ^ bc[1], 43UL);
        st[12] = rotate(st[13] ^ bc[2], 25UL);
        st[13] = rotate(st[19] ^ bc[3], 8UL);
        st[19] = rotate(st[23] ^ bc[2], 56UL);
        st[23] = rotate(st[15] ^ bc[4], 41UL);
        st[15] = rotate(st[4] ^ bc[3], 27UL);
        st[4] = rotate(st[24] ^ bc[3], 14UL);
        st[24] = rotate(st[21] ^ bc[0], 2UL);
        st[21] = rotate(st[8] ^ bc[2], 55UL);
        st[8] = rotate(st[16] ^ bc[0], 35UL);
        st[16] = rotate(st[5] ^ bc[4], 36UL);
        st[5] = rotate(st[3] ^ bc[2], 28UL);
        st[3] = rotate(st[18] ^ bc[2], 21UL);
        st[18] = rotate(st[17] ^ bc[1], 15UL);
        st[17] = rotate(st[11] ^ bc[0], 10UL);
        st[11] = rotate(st[7] ^ bc[1], 6UL);
        st[7] = rotate(st[10] ^ bc[4], 3UL);
        st[10] = rotate(tmp1, 1UL);
        */


        //ulong tmp1 = st[0]; ulong tmp2 = st[1]; st[0] = bitselect(st[0] ^ st[2], st[0], st[1]); st[1] = bitselect(st[1] ^ st[3], st[1], st[2]); st[2] = bitselect(st[2] ^ st[4], st[2], st[3]); st[3] = bitselect(st[3] ^ tmp1, st[3], st[4]); st[4] = bitselect(st[4] ^ tmp2, st[4], tmp1);
        //tmp1 = st[5]; tmp2 = st[6]; st[5] = bitselect(st[5] ^ st[7], st[5], st[6]); st[6] = bitselect(st[6] ^ st[8], st[6], st[7]); st[7] = bitselect(st[7] ^ st[9], st[7], st[8]); st[8] = bitselect(st[8] ^ tmp1, st[8], st[9]); st[9] = bitselect(st[9] ^ tmp2, st[9], tmp1);
        //tmp1 = st[10]; tmp2 = st[11]; st[10] = bitselect(st[10] ^ st[12], st[10], st[11]); st[11] = bitselect(st[11] ^ st[13], st[11], st[12]); st[12] = bitselect(st[12] ^ st[14], st[12], st[13]); st[13] = bitselect(st[13] ^ tmp1, st[13], st[14]); st[14] = bitselect(st[14] ^ tmp2, st[14], tmp1);
        //tmp1 = st[15]; tmp2 = st[16]; st[15] = bitselect(st[15] ^ st[17], st[15], st[16]); st[16] = bitselect(st[16] ^ st[18], st[16], st[17]); st[17] = bitselect(st[17] ^ st[19], st[17], st[18]); st[18] = bitselect(st[18] ^ tmp1, st[18], st[19]); st[19] = bitselect(st[19] ^ tmp2, st[19], tmp1);
        //tmp1 = st[20]; tmp2 = st[21]; st[20] = bitselect(st[20] ^ st[22], st[20], st[21]); st[21] = bitselect(st[21] ^ st[23], st[21], st[22]); st[22] = bitselect(st[22] ^ st[24], st[22], st[23]); st[23] = bitselect(st[23] ^ tmp1, st[23], st[24]); st[24] = bitselect(st[24] ^ tmp2, st[24], tmp1);

        #pragma unroll
        for(int i = 0; i < 25; i += 5)
        {
            ulong tmp1 = st[i], tmp2 = st[i + 1];

            st[i] = bitselect(st[i] ^ st[i + 2], st[i], st[i + 1]);
            st[i + 1] = bitselect(st[i + 1] ^ st[i + 3], st[i + 1], st[i + 2]);
            st[i + 2] = bitselect(st[i + 2] ^ st[i + 4], st[i + 2], st[i + 3]);
            st[i + 3] = bitselect(st[i + 3] ^ tmp1, st[i + 3], st[i + 4]);
            st[i + 4] = bitselect(st[i + 4] ^ tmp2, st[i + 4], tmp1);
        }

        //  Iota
        st[0] ^= keccakf_rndc[round];
    }
}

void CNKeccak(ulong *output, ulong *input)
{
    ulong st[25];

    // Copy 72 bytes
    for(int i = 0; i < 9; ++i) st[i] = input[i];

    // Last four and '1' bit for padding
    //st[9] = as_ulong((uint2)(((uint *)input)[18], 0x00000001U));

    st[9] = (input[9] & 0x00000000FFFFFFFFUL) | 0x0000000100000000UL;

    for(int i = 10; i < 25; ++i) st[i] = 0x00UL;

    // Last bit of padding
    st[16] = 0x8000000000000000UL;

    keccakf1600_1(st);

    for(int i = 0; i < 25; ++i) output[i] = st[i];
}

static const __constant uchar rcon[8] = { 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40 };

#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable

#define BYTE(x, y)	(amd_bfe((x), (y) << 3U, 8U))

#define SubWord(inw)		((sbox[BYTE(inw, 3)] << 24) | (sbox[BYTE(inw, 2)] << 16) | (sbox[BYTE(inw, 1)] << 8) | sbox[BYTE(inw, 0)])

void AESExpandKey256(uint *keybuf)
{
    //#pragma unroll 4
    for(uint c = 8, i = 1; c < 60; ++c)
    {
        // For 256-bit keys, an sbox permutation is done every other 4th uint generated, AND every 8th
        uint t = ((!(c & 7)) || ((c & 7) == 4)) ? SubWord(keybuf[c - 1]) : keybuf[c - 1];

        // If the uint we're generating has an index that is a multiple of 8, rotate and XOR with the round constant,
        // then XOR this with previously generated uint. If it's 4 after a multiple of 8, only the sbox permutation
        // is done, followed by the XOR. If neither are true, only the XOR with the previously generated uint is done.
        keybuf[c] = keybuf[c - 8] ^ ((!(c & 7)) ? rotate(t, 24U) ^ as_uint((uchar4)(rcon[i++], 0U, 0U, 0U)) : t);
    }
}

#define IDX(x)	((x) * (get_global_size(0)))

__attribute__((reqd_work_group_size(WORKSIZE, 8, 1)))
__kernel void cn0(__global ulong *input, __global uint4 *Scratchpad, __global ulong *states)
{
    ulong State[25];
    uint ExpandedKey1[256];
    __local uint AES0[256], AES1[256], AES2[256], AES3[256];
    uint4 text;

    states += (25 * (get_global_id(0) - get_global_offset(0)));
    Scratchpad += ((get_global_id(0) - get_global_offset(0)));

    for(int i = get_local_id(0); i < 256; i += WORKSIZE)
    {
        const uint tmp = AES0_C[i];
        AES0[i] = tmp;
        AES1[i] = rotate(tmp, 8U);
        AES2[i] = rotate(tmp, 16U);
        AES3[i] = rotate(tmp, 24U);
    }

    ((ulong8 *)State)[0] = vload8(0, input);
    State[8] = input[8];
    State[9] = (ulong)((__global uint *)input)[18];

    ((uint *)State)[9] &= 0x00FFFFFFU;
    ((uint *)State)[9] |= ((get_global_id(0)) & 0xFF) << 24;
    ((uint *)State)[10] &= 0xFF000000U;
    ((uint *)State)[10] |= ((get_global_id(0) >> 8));
    State[9] = (input[9] & 0x00000000FFFFFFFFUL) | 0x0000000100000000UL;

    for(int i = 10; i < 25; ++i) State[i] = 0x00UL;

    // Last bit of padding
    State[16] = 0x8000000000000000UL;

    keccakf1600_2(State);

    mem_fence(CLK_GLOBAL_MEM_FENCE);

    #pragma unroll
    for(int i = 0; i < 25; ++i) states[i] = State[i];

    text = vload4(get_local_id(1) + 4, (__global uint *)(states));

    #pragma unroll
    for(int i = 0; i < 4; ++i) ((ulong *)ExpandedKey1)[i] = states[i];

    AESExpandKey256(ExpandedKey1);

    mem_fence(CLK_LOCAL_MEM_FENCE);

    #pragma unroll 2
    for(int i = 0; i < 0x4000; ++i)
    {
        #pragma unroll
        for(int j = 0; j < 10; ++j)
            text = AES_Round(AES0, AES1, AES2, AES3, text, ((uint4 *)ExpandedKey1)[j]);

        Scratchpad[IDX((i << 3) + get_local_id(1))] = text;
    }

    mem_fence(CLK_GLOBAL_MEM_FENCE);
}

__attribute__((reqd_work_group_size(WORKSIZE, 1, 1)))
__kernel void cn1(__global uint4 *Scratchpad, __global ulong *states)
{
    ulong a[2], b[2];
    __local uint AES0[256], AES1[256], AES2[256], AES3[256];

    Scratchpad += ((get_global_id(0) - get_global_offset(0)));
    states += (25 * (get_global_id(0) - get_global_offset(0)));

    for(int i = get_local_id(0); i < 256; i += WORKSIZE)
    {
        const uint tmp = AES0_C[i];
        AES0[i] = tmp;
        AES1[i] = rotate(tmp, 8U);
        AES2[i] = rotate(tmp, 16U);
        AES3[i] = rotate(tmp, 24U);
    }

    a[0] = states[0] ^ states[4];
    b[0] = states[2] ^ states[6];
    a[1] = states[1] ^ states[5];
    b[1] = states[3] ^ states[7];

    uint4 b_x = ((uint4 *)b)[0];

    mem_fence(CLK_LOCAL_MEM_FENCE);

    #pragma unroll 8
    for(int i = 0; i < 0x80000; ++i)
    {
        ulong c[2];

        ((uint4 *)c)[0] = Scratchpad[IDX((a[0] & 0x1FFFF0) >> 4)];
        ((uint4 *)c)[0] = AES_Round(AES0, AES1, AES2, AES3, ((uint4 *)c)[0], ((uint4 *)a)[0]);
        //b_x ^= ((uint4 *)c)[0];

        Scratchpad[IDX((a[0] & 0x1FFFF0) >> 4)] = b_x ^ ((uint4 *)c)[0];

        uint4 tmp;
        tmp = Scratchpad[IDX((c[0] & 0x1FFFF0) >> 4)];

        a[1] += c[0] * as_ulong2(tmp).s0;
        a[0] += mul_hi(c[0], as_ulong2(tmp).s0);

        Scratchpad[IDX((c[0] & 0x1FFFF0) >> 4)] = ((uint4 *)a)[0];

        ((uint4 *)a)[0] ^= tmp;

        b_x = ((uint4 *)c)[0];
    }

    mem_fence(CLK_GLOBAL_MEM_FENCE);
}

__attribute__((reqd_work_group_size(WORKSIZE, 8, 1)))
__kernel void cn2(__global uint4 *Scratchpad, __global ulong *states, __global uint *Branch0, __global uint *Branch1, __global uint *Branch2, __global uint *Branch3)
{
    __local uint AES0[256], AES1[256], AES2[256], AES3[256];
    uint ExpandedKey2[256];
    ulong State[25];
    uint4 text;

    Scratchpad += ((get_global_id(0) - get_global_offset(0)));
    states += (25 * (get_global_id(0) - get_global_offset(0)));

    for(int i = get_local_id(0); i < 256; i += WORKSIZE)
    {
        const uint tmp = AES0_C[i];
        AES0[i] = tmp;
        AES1[i] = rotate(tmp, 8U);
        AES2[i] = rotate(tmp, 16U);
        AES3[i] = rotate(tmp, 24U);
    }

    #if defined(__Tahiti__) || defined(__Pitcairn__)

    for(int i = 0; i < 4; ++i) ((ulong *)ExpandedKey2)[i] = states[i + 4];
    text = vload4(get_local_id(1) + 4, (__global uint *)states);

    #else

    text = vload4(get_local_id(1) + 4, (__global uint *)states);
    ((uint8 *)ExpandedKey2)[0] = vload8(1, (__global uint *)states);

    #endif

    AESExpandKey256(ExpandedKey2);

    barrier(CLK_LOCAL_MEM_FENCE);

    #pragma unroll 2
    for(int i = 0; i < 0x4000; ++i)
    {
        text ^= Scratchpad[IDX((i << 3) + get_local_id(1))];

        #pragma unroll
        for(int j = 0; j < 10; ++j)
            text = AES_Round(AES0, AES1, AES2, AES3, text, ((uint4 *)ExpandedKey2)[j]);
    }

    vstore2(as_ulong2(text), get_local_id(1) + 4, states);

    barrier(CLK_GLOBAL_MEM_FENCE);

    if(!get_local_id(1))
    {
        for(int i = 0; i < 25; ++i) State[i] = states[i];

        keccakf1600_2(State);

        for(int i = 0; i < 25; ++i) states[i] = State[i];

        switch(State[0] & 3)
        {
            case 0:
                Branch0[atomic_inc(Branch0 + get_global_size(0))] = get_global_id(0) - get_global_offset(0);
                break;
            case 1:
                Branch1[atomic_inc(Branch1 + get_global_size(0))] = get_global_id(0) - get_global_offset(0);
                break;
            case 2:
                Branch2[atomic_inc(Branch2 + get_global_size(0))] = get_global_id(0) - get_global_offset(0);
                break;
            case 3:
                Branch3[atomic_inc(Branch3 + get_global_size(0))] = get_global_id(0) - get_global_offset(0);
                break;
        }
    }

    mem_fence(CLK_GLOBAL_MEM_FENCE);
}

/*
__kernel void cryptonight(__global ulong *input, __global uint4 *Scratchpad, __global ulong *states, __global uint *Branch0, __global uint *Branch1, __global uint *Branch2, __global uint *Branch3, ulong ThreadCount)
{
    uchar State[200];
    __local uint AES0[256], AES1[256], AES2[256], AES3[256];
    uchar ExpandedKey1[256], ExpandedKey2[256];
    ulong inbuf[10], a[2], b[2];
    uint4 text[8];

    for(int i = 0; i < 256; ++i)
    {
        const uint tmp = AES0_C[i];
        AES0[i] = tmp;
        AES1[i] = rotate(tmp, 8U);
        AES2[i] = rotate(tmp, 16U);
        AES3[i] = rotate(tmp, 24U);
    }

    ((ulong8 *)inbuf)[0] = vload8(0, input);
    inbuf[8] = input[8];
    inbuf[9] = (ulong)((__global uint *)input)[18];

    ((uint *)(((uchar *)inbuf) + 39))[0] = get_global_id(0);
    CNKeccak((ulong *)State, inbuf);

    a[0] = ((ulong *)State)[0] ^ ((ulong *)State)[4];
    b[0] = ((ulong *)State)[2] ^ ((ulong *)State)[6];
    a[1] = ((ulong *)State)[1] ^ ((ulong *)State)[5];
    b[1] = ((ulong *)State)[3] ^ ((ulong *)State)[7];

    for(uint i = 0; i < 8; ++i) text[i] = vload4(i + 4, (uint *)(State));

    for(int i = 0; i < 4; ++i) ((ulong *)ExpandedKey1)[i] = ((ulong *)State)[i];
    for(int i = 0; i < 4; ++i) ((ulong *)ExpandedKey2)[i] = ((ulong *)State)[i + 4];

    AESExpandKey256(ExpandedKey1);
    AESExpandKey256(ExpandedKey2);

    mem_fence(CLK_LOCAL_MEM_FENCE);

    Scratchpad += ((1 << 17) * (get_global_id(0) - get_global_offset(0)));

    //#pragma unroll 1
    for(int i = 0; i < (1 << 17); i += 8)
    {
        #pragma unroll
        for(int j = 0; j < 10; ++j)
        {
            #pragma unroll
            for(int x = 0; x < 8; ++x)
                text[x] = AES_Round(AES0, AES1, AES2, AES3, text[x], ((uint4 *)ExpandedKey1)[j]);
        }

        for(int j = 0; j < 8; ++j) *(Scratchpad + i + j) = text[j];
    }


    uint4 b_x = ((uint4 *)b)[0];

    //#pragma unroll 1
    for(int i = 0; i < 0x80000; ++i)
    {
        ulong c[2];

        ((uint4 *)c)[0] = Scratchpad[(a[0] & 0x1FFFF0) >> 4];
        ((uint4 *)c)[0] = AES_Round(AES0, AES1, AES2, AES3, ((uint4 *)c)[0], ((uint4 *)a)[0]);
        b_x ^= ((uint4 *)c)[0];

        Scratchpad[(a[0] & 0x1FFFF0) >> 4] = b_x;

        uint4 tmp;
        tmp = Scratchpad[(c[0] & 0x1FFFF0) >> 4];

        a[1] += c[0] * as_ulong2(tmp).s0;
        a[0] += mul_hi(c[0], as_ulong2(tmp).s0);

        Scratchpad[(c[0] & 0x1FFFF0) >> 4] = ((uint4 *)a)[0];

        ((uint4 *)a)[0] ^= tmp;

        b_x = ((uint4 *)c)[0];
    }

    for(uint i = 0; i < 8; ++i) text[i] = vload4(i + 4, (uint *)(State));

    for(int i = 0; i < (1 << 17); i += 8)
    {
        #pragma unroll
        for(int j = 0; j < 8; ++j) text[j] ^= Scratchpad[i + j];

        #pragma unroll 1
        for(int j = 0; j < 10; ++j)
        {
            #pragma unroll
            for(int x = 0; x < 8; ++x)
                text[x] = AES_Round(AES0, AES1, AES2, AES3, text[x], ((uint4 *)ExpandedKey2)[j]);
        }
    }

    for(uint i = 0; i < 8; ++i) vstore4(text[i], i + 4, (uint *)(State));

    keccakf1600((ulong *)State);

    states += (25 * (get_global_id(0) - get_global_offset(0)));

    for(int i = 0; i < 25; ++i) states[i] = ((ulong *)State)[i];

    switch(State[0] & 3)
    {
        case 0:
            Branch0[atomic_inc(Branch0 + ThreadCount)] = get_global_id(0) - get_global_offset(0);
            break;
        case 1:
            Branch1[atomic_inc(Branch1 + ThreadCount)] = get_global_id(0) - get_global_offset(0);
            break;
        case 2:
            Branch2[atomic_inc(Branch2 + ThreadCount)] = get_global_id(0) - get_global_offset(0);
            break;
        case 3:
            Branch3[atomic_inc(Branch3 + ThreadCount)] = get_global_id(0) - get_global_offset(0);
            break;
    }
}
*/

#define VSWAP8(x)	(((x) >> 56) | (((x) >> 40) & 0x000000000000FF00UL) | (((x) >> 24) & 0x0000000000FF0000UL) \
          | (((x) >>  8) & 0x00000000FF000000UL) | (((x) <<  8) & 0x000000FF00000000UL) \
          | (((x) << 24) & 0x0000FF0000000000UL) | (((x) << 40) & 0x00FF000000000000UL) | (((x) << 56) & 0xFF00000000000000UL))

#define VSWAP4(x)	((((x) >> 24) & 0xFFU) | (((x) >> 8) & 0xFF00U) | (((x) << 8) & 0xFF0000U) | (((x) << 24) & 0xFF000000U))

__kernel void Skein(__global ulong *states, __global uint *BranchBuf, __global uint *output, uint Target, ulong Threads)
{
    const ulong idx = get_global_id(0) - get_global_offset(0);

    if(idx >= Threads) return;

    states += 25 * BranchBuf[idx];

    // skein
    ulong8 h = vload8(0, SKEIN512_256_IV);

    // Type field begins with final bit, first bit, then six bits of type; the last 96
    // bits are input processed (including in the block to be processed with that tweak)
    // The output transform is only one run of UBI, since we need only 256 bits of output
    // The tweak for the output transform is Type = Output with the Final bit set
    // T[0] for the output is 8, and I don't know why - should be message size...
    ulong t[3] = { 0x00UL, 0x7000000000000000UL, 0x00UL };
    ulong8 p, m;

    for(uint i = 0; i < 4; ++i)
    {
        if(i < 3) t[0] += 0x40UL;
        else t[0] += 0x08UL;

        t[2] = t[0] ^ t[1];

        m = (i < 3) ? vload8(i, states) : (ulong8)(states[24], 0UL, 0UL, 0UL, 0UL, 0UL, 0UL, 0UL);
        const ulong h8 = h.s0 ^ h.s1 ^ h.s2 ^ h.s3 ^ h.s4 ^ h.s5 ^ h.s6 ^ h.s7 ^ SKEIN_KS_PARITY;
        p = Skein512Block(m, h, h8, t);

        h = m ^ p;

        if(i < 2) t[1] = 0x3000000000000000UL;
        else t[1] = 0xB000000000000000UL;
    }

    t[0] = 0x08UL;
    t[1] = 0xFF00000000000000UL;
    t[2] = t[0] ^ t[1];

    p = (ulong8)(0);
    const ulong h8 = h.s0 ^ h.s1 ^ h.s2 ^ h.s3 ^ h.s4 ^ h.s5 ^ h.s6 ^ h.s7 ^ SKEIN_KS_PARITY;

    p = Skein512Block(p, h, h8, t);

    //vstore8(p, 0, output);

    if(as_uint16(p).s7 <= Target) output[atomic_inc(output + 0xFF)] = BranchBuf[idx] + get_global_offset(0);

    mem_fence(CLK_GLOBAL_MEM_FENCE);
}

#define SWAP8(x)	as_ulong(as_uchar8(x).s76543210)

__kernel void JH(__global ulong *states, __global uint *BranchBuf, __global uint *output, uint Target, ulong Threads)
{
    const uint idx = get_global_id(0) - get_global_offset(0);

    if(idx >= Threads) return;

    states += 25 * BranchBuf[idx];

    sph_u64 h0h = 0xEBD3202C41A398EBUL, h0l = 0xC145B29C7BBECD92UL, h1h = 0xFAC7D4609151931CUL, h1l = 0x038A507ED6820026UL, h2h = 0x45B92677269E23A4UL, h2l = 0x77941AD4481AFBE0UL, h3h = 0x7A176B0226ABB5CDUL, h3l = 0xA82FFF0F4224F056UL;
    sph_u64 h4h = 0x754D2E7F8996A371UL, h4l = 0x62E27DF70849141DUL, h5h = 0x948F2476F7957627UL, h5l = 0x6C29804757B6D587UL, h6h = 0x6C0D8EAC2D275E5CUL, h6l = 0x0F7A0557C6508451UL, h7h = 0xEA12247067D3E47BUL, h7l = 0x69D71CD313ABE389UL;
    sph_u64 tmp;

    for(int i = 0; i < 5; ++i)
    {
        ulong input[8];

        if(i < 3)
        {
            for(int x = 0; x < 8; ++x) input[x] = (states[(i << 3) + x]);
        }
        else if(i == 3)
        {
            input[0] = (states[24]);
            input[1] = 0x80UL;
            for(int x = 2; x < 8; ++x) input[x] = 0x00UL;
        }
        else
        {
            input[7] = 0x4006000000000000UL;

            for(int x = 0; x < 7; ++x) input[x] = 0x00UL;
        }

        h0h ^= input[0];
        h0l ^= input[1];
        h1h ^= input[2];
        h1l ^= input[3];
        h2h ^= input[4];
        h2l ^= input[5];
        h3h ^= input[6];
        h3l ^= input[7];

        E8;

        h4h ^= input[0];
        h4l ^= input[1];
        h5h ^= input[2];
        h5l ^= input[3];
        h6h ^= input[4];
        h6l ^= input[5];
        h7h ^= input[6];
        h7l ^= input[7];
    }

    //output[0] = h6h;
    //output[1] = h6l;
    //output[2] = h7h;
    //output[3] = h7l;

    if(as_uint2(h7l).s1 <= Target) output[atomic_inc(output + 0xFF)] = BranchBuf[idx] + get_global_offset(0);
}

#define SWAP4(x)	as_uint(as_uchar4(x).s3210)

__kernel void Blake(__global ulong *states, __global uint *BranchBuf, __global uint *output, uint Target, ulong Threads)
{
    const uint idx = get_global_id(0) - get_global_offset(0);

    if(idx >= Threads) return;

    states += 25 * BranchBuf[idx];

    unsigned int m[16];
    unsigned int v[16];
    uint h[8];

    ((uint8 *)h)[0] = vload8(0U, c_IV256);

    for(uint i = 0, bitlen = 0; i < 4; ++i)
    {
        if(i < 3)
        {
            ((uint16 *)m)[0] = vload16(i, (__global uint *)states);
            for(int i = 0; i < 16; ++i) m[i] = SWAP4(m[i]);
            bitlen += 512;
        }
        else
        {
            m[0] = SWAP4(((__global uint *)states)[48]);
            m[1] = SWAP4(((__global uint *)states)[49]);
            m[2] = 0x80000000U;

            for(int i = 3; i < 13; ++i) m[i] = 0x00U;

            m[13] = 1U;
            m[14] = 0U;
            m[15] = 0x640;
            bitlen += 64;
        }

        ((uint16 *)v)[0].lo = ((uint8 *)h)[0];
        ((uint16 *)v)[0].hi = vload8(0U, c_u256);

        //v[12] ^= (i < 3) ? (i + 1) << 9 : 1600U;
        //v[13] ^= (i < 3) ? (i + 1) << 9 : 1600U;

        v[12] ^= bitlen;
        v[13] ^= bitlen;

        for(int r = 0; r < 14; r++)
        {
            GS(0, 4, 0x8, 0xC, 0x0);
            GS(1, 5, 0x9, 0xD, 0x2);
            GS(2, 6, 0xA, 0xE, 0x4);
            GS(3, 7, 0xB, 0xF, 0x6);
            GS(0, 5, 0xA, 0xF, 0x8);
            GS(1, 6, 0xB, 0xC, 0xA);
            GS(2, 7, 0x8, 0xD, 0xC);
            GS(3, 4, 0x9, 0xE, 0xE);
        }

        ((uint8 *)h)[0] ^= ((uint8 *)v)[0] ^ ((uint8 *)v)[1];
    }

    for(int i = 0; i < 8; ++i) h[i] = SWAP4(h[i]);

    //for(int i = 0; i < 4; ++i) output[i] = ((ulong *)h)[i];
    if(h[7] <= Target) output[atomic_inc(output + 0xFF)] = BranchBuf[idx] + get_global_offset(0);
}

__kernel void Groestl(__global ulong *states, __global uint *BranchBuf, __global uint *output, uint Target, ulong Threads)
{
    const uint idx = get_global_id(0) - get_global_offset(0);

    if(idx >= Threads) return;

    states += 25 * BranchBuf[idx];

    ulong State[8];

    for(int i = 0; i < 7; ++i) State[i] = 0UL;

    State[7] = 0x0001000000000000UL;

    for(uint i = 0; i < 4; ++i)
    {
        ulong H[8], M[8];

        if(i < 3)
        {
            ((ulong8 *)M)[0] = vload8(i, states);
        }
        else
        {
            M[0] = states[24];
            M[1] = 0x80UL;

            for(int x = 2; x < 7; ++x) M[x] = 0UL;

            M[7] = 0x0400000000000000UL;
        }

        for(int x = 0; x < 8; ++x) H[x] = M[x] ^ State[x];

        PERM_SMALL_P(H);
        PERM_SMALL_Q(M);

        for(int x = 0; x < 8; ++x) State[x] ^= H[x] ^ M[x];
    }

    ulong tmp[8];

    for(int i = 0; i < 8; ++i) tmp[i] = State[i];

    PERM_SMALL_P(State);

    for(int i = 0; i < 8; ++i) State[i] ^= tmp[i];

    //for(int i = 0; i < 4; ++i) output[i] = State[i + 4];
    if(as_uint2(State[7]).s1 <= Target) output[atomic_inc(output + 0xFF)] = BranchBuf[idx] + get_global_offset(0);
}
