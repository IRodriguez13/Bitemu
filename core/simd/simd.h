/**
 * Bitemu - Subsistema SIMD (detección y rutinas aceleradas con fallback)
 *
 * Capas usadas por BE1/BE2 (y futuros backends): detección x86/ARM, logging
 * centralizado al inicializar, y helpers de saturación reutilizables.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_CORE_SIMD_H
#define BITEMU_CORE_SIMD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BITEMU_SIMD_AUDIO_PULL_CHUNK
#define BITEMU_SIMD_AUDIO_PULL_CHUNK 256
#endif

/* Bits de capacidad (extensible para SSSE3/AVX2 cuando haga falta). */
enum {
    BITEMU_SIMD_NONE   = 0,
    BITEMU_SIMD_SSE2   = 1u << 0,
    BITEMU_SIMD_SSSE3  = 1u << 1,
    BITEMU_SIMD_AVX2   = 1u << 2,
    BITEMU_SIMD_NEON   = 1u << 8
};

/**
 * Idempotente. Detecta CPU, fija rutas aceleradas disponibles y emite un único
 * log_info con el resumen (backend de audio / PPU pueden llamar antes; el
 * primer bitemu_create también lo invoca).
 */
void bitemu_simd_init(void);

uint32_t bitemu_simd_caps(void);

/** Cadena estática descriptiva (no liberar). Válida tras init. */
const char *bitemu_simd_desc(void);

/** Nombre corto de la ruta i32→i16: "sse2", "neon", "generic". */
const char *bitemu_simd_clip_i32_to_i16_backend(void);

/**
 * Saturación int32 → int16. Inline para mezclas puntuales (YM, consola, APU).
 */
static inline int16_t bitemu_sat_i16_i32(int32_t x)
{
    if (x > 32767)
        return 32767;
    if (x < -32768)
        return (int16_t)(int32_t)-32768;
    return (int16_t)x;
}

/**
 * Saturación por bloques: dst[i] = sat(src[i]). n puede ser 0 (no-op).
 * Usa SSE2, NEON o bucle C según init.
 */
void bitemu_clip_i32_to_i16(const int32_t *src, int16_t *dst, size_t n);

/**
 * Saturación in-place de int16 (rangos fuera de int16 por bugs de mezcla).
 * Rutas SIMD donde aplica; siempre seguro.
 */
void bitemu_clip_i16(int16_t *buf, size_t n);

/**
 * Audio: lee sample_count muestras desde buffer circular int16, aplica volume
 * y escribe en dst con saturación int16 (SIMD en la fase de clip por bloques).
 * Retorna el nuevo índice de lectura (rd).
 */
size_t bitemu_audio_ring_pull_scaled_clip_i16(const int16_t *ring, size_t ring_len,
    size_t rd, float volume, size_t sample_count, int16_t *dst);

/**
 * Rellena nbytes de píxeles RGB888 (nbytes múltiplo de 3) con (r,g,b).
 * Ruta NEON/SSE2 o memset si r==g==b; idempotente con bitemu_simd_init.
 */
void bitemu_fill_rgb888(uint8_t *dst, size_t nbytes, uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif /* BITEMU_CORE_SIMD_H */
