/**
 * Bitemu - SIMD: detección x86/ARM y saturación con intrínsecos + fallback
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "simd.h"
#include "../utils/log.h"
#include <stdio.h>
#include <string.h>

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
# if defined(__GNUC__) || defined(__clang__)
#  include <cpuid.h>
# elif defined(_MSC_VER)
#  include <intrin.h>
# endif
#endif

#if defined(__linux__) && defined(__arm__) && !defined(__aarch64__)
# include <sys/auxv.h>
# include <asm/hwcap.h>
#endif

#if defined(__SSE2__)
# include <emmintrin.h>
#endif

#if defined(__ARM_NEON)
# include <arm_neon.h>
#endif

static uint32_t g_caps;
static int g_inited;
static const char *g_clip_i32_name = "generic";

static void clip_i32_to_i16_generic(const int32_t *src, int16_t *dst, size_t n)
{
    for (size_t i = 0; i < n; i++)
        dst[i] = bitemu_sat_i16_i32(src[i]);
}

static void clip_i16_generic(int16_t *buf, size_t n)
{
    for (size_t i = 0; i < n; i++)
        buf[i] = bitemu_sat_i16_i32((int32_t)buf[i]);
}

#if defined(__SSE2__)
static void clip_i32_to_i16_sse2(const int32_t *src, int16_t *dst, size_t n)
{
    size_t i = 0;
    for (; i + 8 <= n; i += 8)
    {
        __m128i lo = _mm_loadu_si128((const __m128i *)(const void *)(src + i));
        __m128i hi = _mm_loadu_si128((const __m128i *)(const void *)(src + i + 4));
        __m128i out = _mm_packs_epi32(lo, hi);
        _mm_storeu_si128((__m128i *)(void *)(dst + i), out);
    }
    for (; i < n; i++)
        dst[i] = bitemu_sat_i16_i32(src[i]);
}

static void clip_i16_sse2(int16_t *buf, size_t n)
{
    size_t i = 0;
    for (; i + 8 <= n; i += 8)
    {
        __m128i v = _mm_loadu_si128((const __m128i *)(const void *)(buf + i));
        __m128i sign = _mm_srai_epi16(v, 15);
        __m128i lo = _mm_unpacklo_epi16(v, sign);
        __m128i hi = _mm_unpackhi_epi16(v, sign);
        v = _mm_packs_epi32(lo, hi);
        _mm_storeu_si128((__m128i *)(void *)(buf + i), v);
    }
    for (; i < n; i++)
        buf[i] = bitemu_sat_i16_i32((int32_t)buf[i]);
}

# if defined(__GNUC__) && defined(__x86_64__)
#  include <immintrin.h>

__attribute__((target("avx2")))
static void clip_i32_to_i16_avx2(const int32_t *src, int16_t *dst, size_t n)
{
    size_t i = 0;
    for (; i + 16 <= n; i += 16)
    {
        __m256i q0 = _mm256_loadu_si256((const __m256i *)(const void *)(src + i));
        __m256i q1 = _mm256_loadu_si256((const __m256i *)(const void *)(src + i + 8));
        __m128i lo0 = _mm256_castsi256_si128(q0);
        __m128i hi0 = _mm256_extracti128_si256(q0, 1);
        __m128i lo1 = _mm256_castsi256_si128(q1);
        __m128i hi1 = _mm256_extracti128_si256(q1, 1);
        __m128i p0 = _mm_packs_epi32(lo0, hi0);
        __m128i p1 = _mm_packs_epi32(lo1, hi1);
        _mm_storeu_si128((__m128i *)(void *)(dst + i), p0);
        _mm_storeu_si128((__m128i *)(void *)(dst + i + 8), p1);
    }
    for (; i < n; i++)
        dst[i] = bitemu_sat_i16_i32(src[i]);
}
# endif /* GCC x86_64 AVX2 */
#endif /* __SSE2__ */

#if defined(__ARM_NEON)
static void clip_i32_to_i16_neon(const int32_t *src, int16_t *dst, size_t n)
{
    size_t i = 0;
    for (; i + 8 <= n; i += 8)
    {
        int32x4_t a = vld1q_s32(src + i);
        int32x4_t b = vld1q_s32(src + i + 4);
        int16x8_t o = vcombine_s16(vqmovn_s32(a), vqmovn_s32(b));
        vst1q_s16(dst + i, o);
    }
    for (; i < n; i++)
        dst[i] = bitemu_sat_i16_i32(src[i]);
}

static void clip_i16_neon(int16_t *buf, size_t n)
{
    size_t i = 0;
    for (; i + 8 <= n; i += 8)
    {
        int16x8_t v = vld1q_s16(buf + i);
        int32x4_t lo = vmovl_s16(vget_low_s16(v));
        int32x4_t hi = vmovl_s16(vget_high_s16(v));
        v = vcombine_s16(vqmovn_s32(lo), vqmovn_s32(hi));
        vst1q_s16(buf + i, v);
    }
    for (; i < n; i++)
        buf[i] = bitemu_sat_i16_i32((int32_t)buf[i]);
}
#endif

static void fill_rgb888_generic(uint8_t *dst, size_t nbytes, uint8_t r, uint8_t g, uint8_t b)
{
    if (r == g && g == b)
    {
        memset(dst, r, nbytes);
        return;
    }
    uint64_t pack2 = (uint64_t)r | ((uint64_t)g << 8) | ((uint64_t)b << 16)
                   | ((uint64_t)r << 24) | ((uint64_t)g << 32) | ((uint64_t)b << 40);
    size_t i = 0;
    for (; i + 6 <= nbytes; i += 6)
        memcpy(dst + i, &pack2, 6);
    for (; i + 3 <= nbytes; i += 3)
    {
        dst[i] = r;
        dst[i + 1] = g;
        dst[i + 2] = b;
    }
}

#if defined(__SSE2__)
static void fill_rgb888_sse2(uint8_t *dst, size_t nbytes, uint8_t r, uint8_t g, uint8_t b)
{
    if (r == g && g == b)
    {
        memset(dst, r, nbytes);
        return;
    }
    size_t i = 0;
    for (; i + 48 <= nbytes; i += 48)
    {
        __m128i p0 = _mm_setr_epi8((char)r, (char)g, (char)b, (char)r, (char)g, (char)b,
            (char)r, (char)g, (char)b, (char)r, (char)g, (char)b, (char)r, (char)g, (char)b, (char)r);
        __m128i p1 = _mm_setr_epi8((char)g, (char)b, (char)r, (char)g, (char)b, (char)r,
            (char)g, (char)b, (char)r, (char)g, (char)b, (char)r, (char)g, (char)b, (char)r, (char)g);
        __m128i p2 = _mm_setr_epi8((char)b, (char)r, (char)g, (char)b, (char)r, (char)g,
            (char)b, (char)r, (char)g, (char)b, (char)r, (char)g, (char)b, (char)r, (char)g, (char)b);
        _mm_storeu_si128((__m128i *)(void *)(dst + i), p0);
        _mm_storeu_si128((__m128i *)(void *)(dst + i + 16), p1);
        _mm_storeu_si128((__m128i *)(void *)(dst + i + 32), p2);
    }
    fill_rgb888_generic(dst + i, nbytes - i, r, g, b);
}
#endif

#if defined(__ARM_NEON)
static void fill_rgb888_neon(uint8_t *dst, size_t nbytes, uint8_t r, uint8_t g, uint8_t b)
{
    if (r == g && g == b)
    {
        memset(dst, r, nbytes);
        return;
    }
    uint8x8_t vr = vdup_n_u8(r);
    uint8x8_t vg = vdup_n_u8(g);
    uint8x8_t vb = vdup_n_u8(b);
    size_t i = 0;
    for (; i + 24 <= nbytes; i += 24)
    {
        uint8x8x3_t t;
        t.val[0] = vr;
        t.val[1] = vg;
        t.val[2] = vb;
        vst3_u8(dst + i, t);
    }
    fill_rgb888_generic(dst + i, nbytes - i, r, g, b);
}
#endif

static void (*clip_i32_to_i16_ptr)(const int32_t *, int16_t *, size_t) = clip_i32_to_i16_generic;
static void (*clip_i16_ptr)(int16_t *, size_t) = clip_i16_generic;

static void detect_caps(uint32_t *caps)
{
    *caps = BITEMU_SIMD_NONE;

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
# if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER)
    unsigned eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx))
    {
        if (edx & (1u << 26))
            *caps |= BITEMU_SIMD_SSE2;
        if (ecx & (1u << 9))
            *caps |= BITEMU_SIMD_SSSE3;
    }
# if defined(__x86_64__)
    {
        unsigned max_leaf = __get_cpuid_max(0, NULL);
        if (max_leaf >= 7)
        {
            unsigned eax7, ebx7, ecx7, edx7;
            if (__get_cpuid_count(7, 0, &eax7, &ebx7, &ecx7, &edx7)
                && (ebx7 & (1u << 5)))
                *caps |= BITEMU_SIMD_AVX2;
        }
    }
# endif
# elif defined(_MSC_VER)
    int regs[4];
    __cpuid(regs, 1);
    if ((unsigned)regs[3] & (1u << 26))
        *caps |= BITEMU_SIMD_SSE2;
    if ((unsigned)regs[2] & (1u << 9))
        *caps |= BITEMU_SIMD_SSSE3;
# ifdef _M_X64
    __cpuid(regs, 0);
    if ((unsigned)regs[0] >= 7)
    {
        __cpuidex(regs, 7, 0);
        if ((unsigned)regs[1] & (1u << 5))
            *caps |= BITEMU_SIMD_AVX2;
    }
# endif
# endif
#elif defined(__aarch64__)
    *caps |= BITEMU_SIMD_NEON;
#elif defined(__ARM_NEON)
# if defined(__linux__) && !defined(__aarch64__)
    {
        unsigned long hw = getauxval(AT_HWCAP);
        if (hw & HWCAP_NEON)
            *caps |= BITEMU_SIMD_NEON;
    }
# else
    *caps |= BITEMU_SIMD_NEON;
# endif
#endif
}

static void pick_clip_impl(void)
{
    clip_i32_to_i16_ptr = clip_i32_to_i16_generic;
    clip_i16_ptr = clip_i16_generic;
    g_clip_i32_name = "generic";

#if defined(__GNUC__) && defined(__x86_64__) && defined(__SSE2__)
    if (g_caps & BITEMU_SIMD_AVX2)
    {
        clip_i32_to_i16_ptr = clip_i32_to_i16_avx2;
        clip_i16_ptr = clip_i16_sse2;
        g_clip_i32_name = "avx2";
    }
    else if (g_caps & BITEMU_SIMD_SSE2)
    {
        clip_i32_to_i16_ptr = clip_i32_to_i16_sse2;
        clip_i16_ptr = clip_i16_sse2;
        g_clip_i32_name = "sse2";
    }
#elif defined(__SSE2__)
    if (g_caps & BITEMU_SIMD_SSE2)
    {
        clip_i32_to_i16_ptr = clip_i32_to_i16_sse2;
        clip_i16_ptr = clip_i16_sse2;
        g_clip_i32_name = "sse2";
    }
#elif defined(__ARM_NEON)
    if (g_caps & BITEMU_SIMD_NEON)
    {
        clip_i32_to_i16_ptr = clip_i32_to_i16_neon;
        clip_i16_ptr = clip_i16_neon;
        g_clip_i32_name = "neon";
    }
#endif
}

void bitemu_simd_init(void)
{
    if (g_inited)
        return;
    g_inited = 1;
    detect_caps(&g_caps);
    pick_clip_impl();

    /* Un único log en caliente para diagnóstico (no spam por frame). */
    log_info("SIMD: %s | i32→i16=%s", bitemu_simd_desc(), g_clip_i32_name);
}

uint32_t bitemu_simd_caps(void)
{
    if (!g_inited)
        bitemu_simd_init();
    return g_caps;
}

const char *bitemu_simd_clip_i32_to_i16_backend(void)
{
    if (!g_inited)
        bitemu_simd_init();
    return g_clip_i32_name;
}

const char *bitemu_simd_desc(void)
{
    static char buf[128];
    if (!g_inited)
        bitemu_simd_init();

    char *p = buf;
    size_t room = sizeof buf;
    int first = 1;

#define APPEND(fmt, ...)                                                                     \
    do                                                                                       \
    {                                                                                        \
        int n = snprintf(p, room, (first ? fmt : " " fmt), __VA_ARGS__);                     \
        if (n < 0 || (size_t)n >= room)                                                     \
        {                                                                                    \
            buf[sizeof buf - 1] = '\0';                                                      \
            return buf;                                                                      \
        }                                                                                    \
        p += (size_t)n;                                                                      \
        room -= (size_t)n;                                                                   \
        first = 0;                                                                           \
    } while (0)

    if (g_caps == BITEMU_SIMD_NONE)
    {
        APPEND("%s", "scalar-only (sin extensiones SIMD detectadas)");
        return buf;
    }
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
    APPEND("%s", "x86");
#elif defined(__aarch64__)
    APPEND("%s", "AArch64");
#elif defined(__arm__)
    APPEND("%s", "ARM");
#else
    APPEND("%s", "host");
#endif
    if (g_caps & BITEMU_SIMD_SSE2)
        APPEND("%s", "SSE2");
    if (g_caps & BITEMU_SIMD_SSSE3)
        APPEND("%s", "SSSE3");
    if (g_caps & BITEMU_SIMD_AVX2)
        APPEND("%s", "AVX2");
    if (g_caps & BITEMU_SIMD_NEON)
        APPEND("%s", "NEON");

#undef APPEND
    return buf;
}

void bitemu_clip_i32_to_i16(const int32_t *src, int16_t *dst, size_t n)
{
    if (!g_inited)
        bitemu_simd_init();
    if (!n)
        return;
    clip_i32_to_i16_ptr(src, dst, n);
}

void bitemu_clip_i16(int16_t *buf, size_t n)
{
    if (!g_inited)
        bitemu_simd_init();
    if (!n)
        return;
    clip_i16_ptr(buf, n);
}

size_t bitemu_audio_ring_pull_scaled_clip_i16(const int16_t *ring, size_t ring_len,
    size_t rd, float volume, size_t sample_count, int16_t *dst)
{
    if (!ring || !dst || ring_len == 0 || sample_count == 0)
        return rd;
    if (!g_inited)
        bitemu_simd_init();

    if (volume == 1.0f)
    {
        for (size_t i = 0; i < sample_count; i++)
        {
            dst[i] = ring[rd];
            rd = (rd + 1) % ring_len;
        }
        return rd;
    }

    size_t i = 0;
    while (i < sample_count)
    {
        int32_t scratch[BITEMU_SIMD_AUDIO_PULL_CHUNK];
        size_t chunk = sample_count - i;
        if (chunk > BITEMU_SIMD_AUDIO_PULL_CHUNK)
            chunk = BITEMU_SIMD_AUDIO_PULL_CHUNK;
        for (size_t k = 0; k < chunk; k++)
        {
            scratch[k] = (int32_t)(ring[rd] * volume);
            rd = (rd + 1) % ring_len;
        }
        clip_i32_to_i16_ptr(scratch, dst + i, chunk);
        i += chunk;
    }
    return rd;
}

void bitemu_fill_rgb888(uint8_t *dst, size_t nbytes, uint8_t r, uint8_t g, uint8_t b)
{
    if (!dst || nbytes == 0)
        return;
    if (!g_inited)
        bitemu_simd_init();
#if defined(__ARM_NEON)
    if (g_caps & BITEMU_SIMD_NEON)
    {
        fill_rgb888_neon(dst, nbytes, r, g, b);
        return;
    }
#endif
#if defined(__SSE2__)
    if (g_caps & BITEMU_SIMD_SSE2)
    {
        fill_rgb888_sse2(dst, nbytes, r, g, b);
        return;
    }
#endif
    fill_rgb888_generic(dst, nbytes, r, g, b);
}
