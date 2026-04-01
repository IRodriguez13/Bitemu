/**
 * BE2 - Sega Genesis: VDP (Video Display Processor)
 *
 * VRAM, CRAM, VSRAM, registros. Render de tiles.
 * Sin magic numbers: genesis_constants.h
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_VDP_H
#define BITEMU_GENESIS_VDP_H

#include "../genesis_constants.h"
#include <stdint.h>

typedef struct gen_vdp gen_vdp_t;

/* Callback para DMA 68k→VDP: lee word desde addr 68k. NULL = no DMA 68k. */
typedef uint16_t (*gen_vdp_dma_read_fn)(void *ctx, uint32_t addr);

struct gen_vdp {
    uint8_t framebuffer[GEN_FB_SIZE];
    uint8_t regs[GEN_VDP_REG_COUNT];
    uint16_t vram[GEN_VDP_VRAM_SIZE / 2];
    uint16_t cram[GEN_VDP_CRAM_SIZE];
    uint16_t vsram[GEN_VDP_VSRAM_SIZE];
    uint16_t addr_reg;       /* address latch (14 bits) */
    uint8_t code_reg;       /* CD0-CD3: access mode */
    uint8_t _pad_cmd;        /* alinear pending_hi (16-bit palabra alta del comando) */
    uint16_t pending_hi;     /* primera palabra del comando VDP 32-bit */
    int addr_inc;           /* auto-increment (reg 0x0F) */

    /* Timing: ciclos acumulados en frame actual */
    int cycle_counter;
    int line_counter;       /* scanline actual (0-261) */
    uint8_t status_reg;     /* bits VBlank, HBlank, etc. */
    uint16_t status_cache;   /* cache para read byte-a-byte (evita doble clear) */

    /* DMA: fill pendiente hasta próximo write a data port (GEN1 savestate lo persiste). */
    uint8_t dma_fill_pending;

    /* DMA 68k: ciclos 68k a “consumir” sin ejecutar opcodes (aprox.; ver gen_vdp_take_dma_stall). */
    uint32_t dma_stall_68k;

    /* Video: NTSC vs PAL (líneas totales / V-int línea visible). Sincronizar con impl->is_pal. */
    uint8_t is_pal;

    /* Reg 10: H-int cada N líneas; -1 = recargar en próximo inicio de frame */
    int hint_counter;

    /* DMA 68k: callback para leer memoria. ctx = genesis_mem_t*. No serializable. */
    gen_vdp_dma_read_fn dma_read_16;
    void *dma_read_ctx;

    /* IRQ al 68k: pendientes internos (el bit F del status refleja “algo pendiente”). */
    uint8_t irq_vint_pending;
    uint8_t irq_hint_pending;

    /* Lectura puerto datos: palabra VRAM + reparto byte alto/bajo (68k big-endian). */
    uint16_t data_read_latch;
    uint8_t data_read_latch_valid;
    /* FIFO puerto datos (MVP): palabras escritas acumuladas hasta drenarse en gen_vdp_step. */
    uint8_t fifo_word_backlog;
    uint32_t fifo_drain_acc;
};

void gen_vdp_init(gen_vdp_t *vdp);
void gen_vdp_reset(gen_vdp_t *vdp);

/* NTSC (0) vs PAL (1): afecta líneas totales y posición de V-blank. Llamar tras detectar región cartucho. */
void gen_vdp_set_pal(gen_vdp_t *vdp, int is_pal);

/* Reinicia contador H-int desde reg 10 (p. ej. tras load parcial sin GEN1). */
void gen_vdp_reload_hint_counter(gen_vdp_t *vdp);

/* Callback para DMA 68k→VDP. Llamar antes de run. */
void gen_vdp_set_dma_read(gen_vdp_t *vdp, gen_vdp_dma_read_fn fn, void *ctx);

/* Puerto control: escribe word (parte de comando 32-bit o registro) */
void gen_vdp_write_ctrl(gen_vdp_t *vdp, uint16_t val);

/* Puerto datos: escribe según modo actual (VRAM/CRAM/VSRAM) */
void gen_vdp_write_data(gen_vdp_t *vdp, uint16_t val);

/* Patrón de barras cuando el display está apagado o para diagnóstico. */
void gen_vdp_render_test_pattern(gen_vdp_t *vdp);

/* Render desde VRAM/CRAM (tiles plano A) */
void gen_vdp_render(gen_vdp_t *vdp);

void gen_vdp_step(gen_vdp_t *vdp, int cycles);

/* Lectura: status (desde control port) y HV counter */
uint16_t gen_vdp_read_status(gen_vdp_t *vdp);
uint8_t gen_vdp_read_status_byte(gen_vdp_t *vdp, int fetch);
uint16_t gen_vdp_read_hv(gen_vdp_t *vdp);

int gen_vdp_is_vram_read_mode(const gen_vdp_t *vdp);
uint16_t gen_vdp_read_data_word(gen_vdp_t *vdp);
uint8_t gen_vdp_read_data_byte(gen_vdp_t *vdp, unsigned addr_odd);

/* Interrupciones: nivel pendiente (4=HBlank, 6=VBlank) o 0 si ninguna */
int gen_vdp_pending_irq_level(gen_vdp_t *vdp);

/* Devuelve y pone a 0 ciclos 68k pendientes por último DMA VDP (para stall en el core). */
uint32_t gen_vdp_take_dma_stall(gen_vdp_t *vdp);

#endif /* BITEMU_GENESIS_VDP_H */
