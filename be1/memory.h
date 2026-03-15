/**
 * Bitemu - Game Boy Memory
 * Mapa de memoria y bancos
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * 0x0000-0x3FFF  ROM Bank 0 (16KB, fija)
 * 0x4000-0x7FFF  ROM Bank 1-N (16KB, switchable)
 * 0x8000-0x9FFF  VRAM (8KB)
 * 0xA000-0xBFFF  External RAM (8KB, switchable)
 * 0xC000-0xCFFF  WRAM Bank 0 (4KB)
 * 0xD000-0xDFFF  WRAM Bank 1 (4KB, DMG: mismo que bank 0)
 * 0xE000-0xFDFF  Echo RAM (mirror 0xC000-0xDDFF)
 * 0xFE00-0xFE9F  OAM (160 bytes)
 * 0xFEA0-0xFEFF  Prohibido
 * 0xFF00-0xFF7F  I/O Registers
 * 0xFF80-0xFFFE  HRAM (127 bytes)
 * 0xFFFF         IE (Interrupt Enable)
 */

#ifndef BITEMU_GB_MEMORY_H
#define BITEMU_GB_MEMORY_H

#include <stddef.h>
#include <stdint.h>

/* Tamaños */
#define GB_ROM_BANK_SIZE  0x4000  /* 16KB */
#define GB_EXT_RAM_BANK   0x2000  /* 8KB por bank */
#define GB_EXT_RAM_BANKS  4
#define GB_EXT_RAM_SIZE   (GB_EXT_RAM_BANK * GB_EXT_RAM_BANKS)
#define GB_VRAM_SIZE      0x2000  /* 8KB */
#define GB_WRAM_SIZE      0x2000  /* 8KB (2 banks x 4KB) */
#define GB_OAM_SIZE       0xA0    /* 160 bytes */
#define GB_HRAM_SIZE      0x80    /* 128 bytes */
#define GB_IO_SIZE        0x80    /* 0xFF00-0xFF7F */

/* Límites de dirección */
#define GB_ROM0_END       0x3FFF
#define GB_ROM1_START     0x4000
#define GB_ROM1_END       0x7FFF
#define GB_VRAM_START     0x8000
#define GB_VRAM_END       0x9FFF
#define GB_EXT_RAM_START  0xA000
#define GB_EXT_RAM_END    0xBFFF
#define GB_WRAM_START     0xC000
#define GB_WRAM_END       0xDFFF
#define GB_ECHO_START     0xE000
#define GB_ECHO_END       0xFDFF
#define GB_OAM_START      0xFE00
#define GB_OAM_END        0xFE9F
#define GB_PROHIBITED_END 0xFEFF
#define GB_IO_START       0xFF00
#define GB_IO_END         0xFF7F
#define GB_HRAM_START     0xFF80
#define GB_HRAM_END       0xFFFE
#define GB_IE_ADDR        0xFFFF

/* MBC1 */
#define GB_MBC_RAM_ENABLE_END  0x1FFF
#define GB_MBC_ROM_BANK_END    0x3FFF
#define GB_MBC_RAM_BANK_END    0x5FFF
#define GB_MBC_RAM_ENABLE_MASK 0x0F
#define GB_MBC_RAM_ENABLE_VAL  0x0A
#define GB_MBC_ROM_BANK_MASK   0x1F
#define GB_MBC_ROM_BANK_HI     0x60
#define GB_MBC_RAM_BANK_MASK   0x03

/* Cartridge type (ROM header 0x147): 0=ROM only, 1-3=MBC1, 5-6=MBC2, 0x0F-0x13=MBC3, 0x19-0x1E=MBC5 */
#define GB_CART_ROM_ONLY  0x00
#define GB_CART_MBC1      0x01
#define GB_CART_MBC2      0x05
#define GB_CART_MBC2_BAT  0x06
#define GB_CART_MBC3      0x13
#define GB_CART_MBC5      0x19
#define GB_CART_MBC5_MAX  0x1E

/* MBC2: 512 x 4-bit internal RAM at 0xA000-0xA1FF */
#define GB_MBC2_RAM_SIZE  512

/* MBC3 RTC: registros 4-0xC (4=seconds, 5=min, 6=hours, 7=days low, 8=days high; 9-0xC no usados) */
#define GB_MBC3_RTC_S   4
#define GB_MBC3_RTC_M   5
#define GB_MBC3_RTC_H   6
#define GB_MBC3_RTC_DL  7
#define GB_MBC3_RTC_DH  8
#define GB_MBC3_RTC_HALT 0x40   /* DH bit 6: RTC halted */

typedef struct gb_mem {
    /* ROM: apuntado desde fuera (cargado por load_rom) */
    uint8_t *rom;
    size_t rom_size;
    uint8_t rom_bank;
    /* Fast path: punteros a bancos actuales (actualizados en reset y bank switch) */
    const uint8_t *rom0_ptr;
    const uint8_t *rom1_ptr;
    uint8_t cart_type;
    uint8_t mbc5_rom_bank_high;  /* MBC5: bit 9 del banco ROM (0x3000-0x3FFF) */

    /* External RAM (cartucho) */
    uint8_t ext_ram[GB_EXT_RAM_SIZE];
    uint8_t ext_ram_bank;
    uint8_t ext_ram_enabled;

    /* Internal */
    uint8_t wram[GB_WRAM_SIZE];
    uint8_t vram[GB_VRAM_SIZE];
    uint8_t oam[GB_OAM_SIZE];
    uint8_t hram[GB_HRAM_SIZE];
    uint8_t io[GB_IO_SIZE];
    uint8_t ie;

    /* Timer: contador interno 16-bit para DIV (DIV = bits 15-8) */
    uint16_t timer_div;

    /* MBC3 RTC: valores actuales y copia al hacer latch (0x01 then 0x00 en 0x6000-0x7FFF) */
    uint8_t rtc_s, rtc_m, rtc_h, rtc_dl, rtc_dh;
    uint8_t rtc_latched[5];
    uint8_t rtc_latch_prev;  /* 1 = se escribió 0x01, siguiente 0x00 hace latch */
    uint64_t rtc_last_sync;  /* timestamp (segundos) última sincronización con reloj real */

    /* Joypad: bits 0-3 = D-pad (R,L,U,D), 4-7 = buttons (A,B,Select,Start); 1 = pressed */
    uint8_t joypad_state;

    /* APU: flags de trigger pendientes (bits 0-3 = ch1-ch4); memory los activa, APU los consume */
    uint8_t apu_trigger_flags;

    /* MBC1: banking mode (0 = ROM mode, 1 = RAM mode) — set via 0x6000-0x7FFF */
    uint8_t mbc1_mode;
} gb_mem_t;

void gb_mem_init(gb_mem_t *mem);
void gb_mem_reset(gb_mem_t *mem);
void gb_mem_update_rom_ptrs(gb_mem_t *mem);  /* tras load_state o cambio de banco */

/* Battery save: path = ROM path; .sav derived by replacing extension. No-op if no battery cart. */
void gb_mem_load_sav(gb_mem_t *mem, const char *rom_path);
void gb_mem_save_sav(const gb_mem_t *mem, const char *rom_path);

uint8_t gb_mem_read(gb_mem_t *mem, uint16_t addr);
void gb_mem_write(gb_mem_t *mem, uint16_t addr, uint8_t val);

/* Lectura de 16 bits (little-endian) */
uint16_t gb_mem_read16(gb_mem_t *mem, uint16_t addr);
void gb_mem_write16(gb_mem_t *mem, uint16_t addr, uint16_t val);

#endif /* BITEMU_GB_MEMORY_H */
