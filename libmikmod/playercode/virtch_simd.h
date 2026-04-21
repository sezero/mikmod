/*	MikMod sound library
    (c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
    complete list.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Library General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
    02111-1307, USA.
*/

/*==============================================================================

   Shared SIMD helpers for MikMod virtch.c and virtch2.c.
  
   Usage from virtch.c (standard mixer):
  
     #ifdef NATIVE_64BIT_INT
     typedef SLONGLONG VIRTCH_SIMD_MIXIDX;
     #else
     typedef SLONG VIRTCH_SIMD_MIXIDX;
     #endif
  
     #define VIRTCH_SIMD_FETCH_SAMPLE(srce, idx) ((SWORD)(srce[(idx) >> FRACBITS]))
     #define VIRTCH_SIMD_UPDATE_TAIL(sample) do { (void)(sample); } while (0)
     #include "virtch_simd.h"
     #include "virtch_simd.c"
     #undef VIRTCH_SIMD_FETCH_SAMPLE
     #undef VIRTCH_SIMD_UPDATE_TAIL
  
   Usage from virtch2.c (HQ mixer):
  
     #ifdef NATIVE_64BIT_INT
     typedef SLONGLONG VIRTCH_SIMD_MIXIDX;
     #else
     typedef SLONG VIRTCH_SIMD_MIXIDX;
     #endif
  
     static __inline SWORD GetSample(const SWORD* const srce, SLONGLONG idx) { ... }
     #define VIRTCH_SIMD_FETCH_SAMPLE(srce, idx) GetSample((srce), (idx))
     #define VIRTCH_SIMD_UPDATE_TAIL(sample) \
         do { vnf->lastvalL = vnf->lvolsel * (sample); vnf->lastvalR = vnf->rvolsel * (sample); } while (0)
     #include "virtch_simd.h"
     #include "virtch_simd.c"
     #undef VIRTCH_SIMD_FETCH_SAMPLE
     #undef VIRTCH_SIMD_UPDATE_TAIL
  
   Notes:
     - vnf is expected to be visible in the including translation unit.
     - This file intentionally centralizes only the "plain stereo/mono fast path".
       Ramp/click handling stays in the owning mixer.
*/

#ifndef LIBMIKMOD_VIRTCH_SIMD_H
#define LIBMIKMOD_VIRTCH_SIMD_H

#include <stddef.h>
#include <stdint.h>

#ifndef IS_ALIGNED_16
#define IS_ALIGNED_16(ptr) ((((uintptr_t)(const void *)(ptr)) & 15u) == 0)
#endif

#ifndef IS_ALIGNED_32
#define IS_ALIGNED_32(ptr) ((((uintptr_t)(const void *)(ptr)) & 31u) == 0)
#endif

#ifndef VIRTCH_SIMD_MIXIDX
#error "VIRTCH_SIMD_MIXIDX must be defined before including virtch_simd.h"
#endif

#ifndef VIRTCH_SIMD_FETCH_SAMPLE
#error "VIRTCH_SIMD_FETCH_SAMPLE(srce, idx) must be defined before including virtch_simd.h"
#endif

#ifndef VIRTCH_SIMD_UPDATE_TAIL
#error "VIRTCH_SIMD_UPDATE_TAIL(sample) must be defined before including virtch_simd.h"
#endif

#if defined(VIRTCH_HAVE_SSE2)
# include <emmintrin.h>
#endif

#if defined(VIRTCH_HAVE_AVX2)
# include <immintrin.h>
#endif

#ifdef NATIVE_64BIT_INT
#define NATIVE SLONGLONG
#else
#define NATIVE SLONG
#endif
typedef SSIZE_T MIXIDX;


static VIRTCH_SIMD_MIXIDX VirtchSIMD_MixMonoNormal_C(
    const SWORD* srce,
    SLONG* dest,
    VIRTCH_SIMD_MIXIDX idx,
    VIRTCH_SIMD_MIXIDX increment,
    SLONG numSamples)
{
    SWORD sample = 0;
    const SLONG lvolsel = vnf->lvolsel;

    while (numSamples--) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += lvolsel * sample;
    }

    VIRTCH_SIMD_UPDATE_TAIL(sample);
    return idx;
}

static VIRTCH_SIMD_MIXIDX VirtchSIMD_MixStereoNormal_C(
    const SWORD* srce,
    SLONG* dest,
    VIRTCH_SIMD_MIXIDX idx,
    VIRTCH_SIMD_MIXIDX increment,
    SLONG numSamples)
{
    SWORD sample = 0;
    const SLONG lvolsel = vnf->lvolsel;
    const SLONG rvolsel = vnf->rvolsel;

    while (numSamples--) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += lvolsel * sample;
        *dest++ += rvolsel * sample;
    }

    VIRTCH_SIMD_UPDATE_TAIL(sample);
    return idx;
}

#if defined(VIRTCH_HAVE_SSE2)
static VIRTCH_SIMD_MIXIDX VirtchSIMD_MixStereoNormal_SSE2(
    const SWORD* srce,
    SLONG* dest,
    VIRTCH_SIMD_MIXIDX idx,
    VIRTCH_SIMD_MIXIDX increment,
    SLONG numSamples)
{
    SWORD sample = 0;
    const SWORD vol[8] = { (SWORD)vnf->lvolsel, (SWORD)vnf->rvolsel };
    SLONG remain = numSamples;

    while (!IS_ALIGNED_16(dest)) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += vol[0] * sample;
        *dest++ += vol[1] * sample;
        if (!--numSamples) {
            VIRTCH_SIMD_UPDATE_TAIL(sample);
            return idx;
        }
    }

    remain = numSamples & 3;
    {
        const __m128i v0 = _mm_set_epi16(0, vol[1], 0, vol[0], 0, vol[1], 0, vol[0]);

        for (numSamples >>= 2; numSamples; --numSamples) {
            const SWORD s0 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
            const SWORD s1 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s2 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s3 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const __m128i v1 = _mm_set_epi16(0, s1, 0, s1, 0, s0, 0, s0);
            const __m128i v2 = _mm_set_epi16(0, s3, 0, s3, 0, s2, 0, s2);
            const __m128i d0 = _mm_load_si128((const __m128i*)(dest + 0));
            const __m128i d1 = _mm_load_si128((const __m128i*)(dest + 4));
            _mm_store_si128((__m128i*)(dest + 0), _mm_add_epi32(d0, _mm_madd_epi16(v0, v1)));
            _mm_store_si128((__m128i*)(dest + 4), _mm_add_epi32(d1, _mm_madd_epi16(v0, v2)));
            dest += 8;
            idx += increment;
            sample = s3;
        }
    }

    while (remain--) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += vol[0] * sample;
        *dest++ += vol[1] * sample;
    }

    VIRTCH_SIMD_UPDATE_TAIL(sample);
    return idx;
}
#endif

#if defined(VIRTCH_HAVE_AVX2)
static VIRTCH_SIMD_MIXIDX VirtchSIMD_MixStereoNormal_AVX2(
    const SWORD* srce,
    SLONG* dest,
    VIRTCH_SIMD_MIXIDX idx,
    VIRTCH_SIMD_MIXIDX increment,
    SLONG numSamples)
{
    SWORD sample = 0;
    SLONG remain = numSamples;

    while (!IS_ALIGNED_32(dest)) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += vnf->lvolsel * sample;
        *dest++ += vnf->rvolsel * sample;
        if (!--numSamples) {
            VIRTCH_SIMD_UPDATE_TAIL(sample);
            return idx;
        }
    }

    remain = numSamples & 7;
    {
        const __m256i vol = _mm256_set_epi16(
            0, (short)vnf->rvolsel, 0, (short)vnf->lvolsel,
            0, (short)vnf->rvolsel, 0, (short)vnf->lvolsel,
            0, (short)vnf->rvolsel, 0, (short)vnf->lvolsel,
            0, (short)vnf->rvolsel, 0, (short)vnf->lvolsel);

        for (numSamples >>= 3; numSamples; --numSamples) {
            const SWORD s0 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
            const SWORD s1 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s2 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s3 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s4 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s5 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s6 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            const SWORD s7 = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);

            const __m256i samp01 = _mm256_set_epi16(
                0, s3, 0, s3, 0, s2, 0, s2,
                0, s1, 0, s1, 0, s0, 0, s0);
            const __m256i samp23 = _mm256_set_epi16(
                0, s7, 0, s7, 0, s6, 0, s6,
                0, s5, 0, s5, 0, s4, 0, s4);

            const __m256i d0 = _mm256_load_si256((const __m256i*)(dest + 0));
            const __m256i d1 = _mm256_load_si256((const __m256i*)(dest + 8));

            _mm256_store_si256((__m256i*)(dest + 0), _mm256_add_epi32(d0, _mm256_madd_epi16(vol, samp01)));
            _mm256_store_si256((__m256i*)(dest + 8), _mm256_add_epi32(d1, _mm256_madd_epi16(vol, samp23)));

            dest += 16;
            idx += increment;
            sample = s7;
        }
    }

    while (remain--) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += vnf->lvolsel * sample;
        *dest++ += vnf->rvolsel * sample;
    }

    VIRTCH_SIMD_UPDATE_TAIL(sample);
    return idx;
}
#endif

#if defined(VIRTCH_HAVE_ALTIVEC)
static VIRTCH_SIMD_MIXIDX VirtchSIMD_MixStereoNormal_ALTIVEC(
    const SWORD* srce,
    SLONG* dest,
    VIRTCH_SIMD_MIXIDX idx,
    VIRTCH_SIMD_MIXIDX increment,
    SLONG numSamples)
{
    SWORD sample = 0;
    const SWORD vol[8] = { (SWORD)vnf->lvolsel, (SWORD)vnf->rvolsel };
    SLONG remain = numSamples;

    while (!IS_ALIGNED_16(dest)) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += vol[0] * sample;
        *dest++ += vol[1] * sample;
        if (!--numSamples) {
            VIRTCH_SIMD_UPDATE_TAIL(sample);
            return idx;
        }
    }

    remain = numSamples & 3;
    {
        SWORD s[8];
        const vector signed short r0 = vec_ld(0, vol);
        const vector signed short v0 = vec_perm(r0, r0, (vector unsigned char)(
            0, 1, 0, 1, 2, 3, 2, 1,
            0, 1, 0, 1, 2, 3, 2, 3));

        for (numSamples >>= 2; numSamples; --numSamples) {
            vector short int r1;
            vector signed short v1, v2;
            vector signed int d0, d1, a0, a1;

            s[0] = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
            s[1] = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            s[2] = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            s[3] = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx += increment);
            s[4] = 0;

            r1 = vec_ld(0, s);
            v1 = vec_perm(r1, r1, (vector unsigned char)(
                0 * 2, 0 * 2 + 1, 4 * 2, 4 * 2 + 1,
                0 * 2, 0 * 2 + 1, 4 * 2, 4 * 2 + 1,
                1 * 2, 1 * 2 + 1, 4 * 2, 4 * 2 + 1,
                1 * 2, 1 * 2 + 1, 4 * 2, 4 * 2 + 1));
            v2 = vec_perm(r1, r1, (vector unsigned char)(
                2 * 2, 2 * 2 + 1, 4 * 2, 4 * 2 + 1,
                2 * 2, 2 * 2 + 1, 4 * 2, 4 * 2 + 1,
                3 * 2, 3 * 2 + 1, 4 * 2, 4 * 2 + 1,
                3 * 2, 3 * 2 + 1, 4 * 2, 4 * 2 + 1));

            d0 = vec_ld(0, dest);
            d1 = vec_ld(0, dest + 4);
            a0 = vec_mule(v0, v1);
            a1 = vec_mule(v0, v2);

            vec_st(vec_add(d0, a0), 0, dest);
            vec_st(vec_add(d1, a1), 0x10, dest);

            dest += 8;
            idx += increment;
            sample = s[3];
        }
    }

    while (remain--) {
        sample = VIRTCH_SIMD_FETCH_SAMPLE(srce, idx);
        idx += increment;
        *dest++ += vol[0] * sample;
        *dest++ += vol[1] * sample;
    }

    VIRTCH_SIMD_UPDATE_TAIL(sample);
    return idx;
}
#endif

static VIRTCH_SIMD_MIXIDX MixSIMDStereoNormal(
    const SWORD* srce,
    SLONG* dest,
    VIRTCH_SIMD_MIXIDX idx,
    VIRTCH_SIMD_MIXIDX increment,
    SLONG numSamples)
{
#if defined(VIRTCH_HAVE_AVX2)
    return VirtchSIMD_MixStereoNormal_AVX2(srce, dest, idx, increment, numSamples);
#elif defined(VIRTCH_HAVE_SSE2)
    return VirtchSIMD_MixStereoNormal_SSE2(srce, dest, idx, increment, numSamples);
#elif defined(VIRTCH_HAVE_ALTIVEC)
    return VirtchSIMD_MixStereoNormal_ALTIVEC(srce, dest, idx, increment, numSamples);
#else
    return VirtchSIMD_MixStereoNormal_C(srce, dest, idx, increment, numSamples);
#endif
}


#endif

