/**
 * BE2 - Sega Genesis / Mega Drive: constantes
 *
 * Agrupadas por subsistema. Sin magic numbers.
 * Referencias: Genesis Programming (Wikibooks), Segaretro.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#ifndef BITEMU_GENESIS_CONSTANTS_H
#define BITEMU_GENESIS_CONSTANTS_H

/* ===== Timing (NTSC) ===== */

enum {
    GEN_68000_HZ         = 7670448,
    GEN_FPS              = 60,
    GEN_SCANLINES_TOTAL  = 262,
    GEN_SCANLINES_VISIBLE = 224,
    GEN_CYCLES_PER_LINE  = 488,
    GEN_CYCLES_PER_FRAME = GEN_SCANLINES_TOTAL * GEN_CYCLES_PER_LINE,  /* 127856 */
};

/* Simplificado para stub: menos ciclos por frame para tests rápidos */
enum {
    GEN_CYCLES_PER_FRAME_STUB = 48868,
};

/* VDP status register bits (read from 0xC00004) */
enum {
    GEN_VDP_STATUS_PAL   = 0x01,
    GEN_VDP_STATUS_DMA  = 0x02,
    GEN_VDP_STATUS_HB   = 0x04,
    GEN_VDP_STATUS_VB   = 0x08,
    GEN_VDP_STATUS_ODD  = 0x10,
    GEN_VDP_STATUS_COL  = 0x20,
    GEN_VDP_STATUS_SOVR = 0x40,
    GEN_VDP_STATUS_F    = 0x80,   /* VINT asserted; cleared on status read */
};

/* VDP reg 0: IE1=HBlank IRQ enable (bit 5) */
/* VDP reg 1: IE0=VBlank IRQ enable (bit 5), display enable (bit 6) */
enum {
    GEN_VDP_REG0_IE1 = 0x20,
    GEN_VDP_REG1_IE0 = 0x20,
};

/* Interrupt levels and vectors (68000) */
enum {
    GEN_IRQ_LEVEL_HBLANK = 4,
    GEN_IRQ_LEVEL_VBLANK = 6,
    GEN_VECTOR_HBLANK    = 0x70,
    GEN_VECTOR_VBLANK    = 0x78,
};

/* ===== Display (VDP Mode 5: 320×224) ===== */

enum {
    GEN_DISPLAY_WIDTH    = 320,
    GEN_DISPLAY_HEIGHT   = 224,
    GEN_FB_SIZE          = GEN_DISPLAY_WIDTH * GEN_DISPLAY_HEIGHT,
    GEN_PIXELS_PER_LINE  = GEN_DISPLAY_WIDTH,
};

/* ===== Memory: tamaños ===== */

enum {
    GEN_RAM_SIZE         = 0x10000,   /* 64 KB work RAM */
    GEN_ROM_MAX_SIZE     = 4 * 1024 * 1024,
    GEN_ROM_MAX          = GEN_ROM_MAX_SIZE,  /* alias para bitemu_load_rom */
    GEN_ROM_PATH_MAX     = 1024,
};

/* ===== Memory map: direcciones 68K (24-bit) ===== */

enum {
    /* Cartridge */
    GEN_ADDR_ROM_START   = 0x000000,
    GEN_ADDR_ROM_END     = 0x3FFFFF,
    GEN_ROM_MASK         = 0x3FFFFF,

    /* Z80 space: 68k accede cuando tiene el bus */
    GEN_ADDR_Z80_START   = 0xA00000,
    GEN_ADDR_Z80_END     = 0xA0FFFF,
    GEN_Z80_RAM_SIZE     = 8192,   /* 8KB RAM del Z80 */

    /* I/O: version, joypad, expansion */
    GEN_ADDR_IO_START    = 0xA10000,
    GEN_ADDR_IO_END      = 0xA1001F,
    GEN_IO_VERSION       = 0xA10000,
    GEN_IO_JOYPAD1       = 0xA10002,
    GEN_IO_JOYPAD2       = 0xA10004,
    GEN_IO_EXPANSION     = 0xA10006,

    /* Z80 bus request / reset */
    GEN_ADDR_Z80_BUSREQ  = 0xA11100,
    GEN_ADDR_Z80_RESET   = 0xA11200,

    /* TMSS (licencia SEGA) */
    GEN_ADDR_TMSS        = 0xA14000,

    /* YM2612 (en espacio Z80: 0xA04000-0xA04003) */
    GEN_ADDR_YM_START    = 0xA04000,
    GEN_ADDR_YM_END      = 0xA04003,

    /* VDP */
    GEN_ADDR_VDP_DATA    = 0xC00000,
    GEN_ADDR_VDP_CTRL    = 0xC00004,
    GEN_ADDR_VDP_HV      = 0xC00008,
    GEN_ADDR_PSG         = 0xC00011,

    /* Work RAM */
    GEN_ADDR_RAM_START   = 0xFF0000,
    GEN_ADDR_RAM_END     = 0xFFFFFF,
    GEN_RAM_MASK         = 0xFFFF,
};

/* ===== ROM header ===== */

enum {
    GEN_HEADER_OFFSET    = 0x100,
    GEN_HEADER_MAGIC_LEN = 4,
};
#define GEN_HEADER_MAGIC "SEGA"

/* SMD format: 512-byte header, 16KB blocks (8KB even + 8KB odd) */
enum {
    GEN_SMD_HEADER_SIZE = 512,
    GEN_SMD_BLOCK_SIZE  = 16384,
    GEN_SMD_BLOCK_MID   = 8192,
};

/* TMSS: write "SEGA" to unlock VDP (Model 1) */
enum {
    GEN_ADDR_TMSS_START = 0xA14000,
    GEN_ADDR_TMSS_END   = 0xA14003,
};

/* ===== CPU 68000: estado inicial y vectores ===== */

/* SR bits 12-10 = interrupt mask (0-7), 7=disabled */
#define GEN_SR_IMASK_SHIFT 8
#define GEN_SR_IMASK_MASK  0x0700

enum {
    GEN_CPU_SR_INIT      = 0x2700,   /* Supervisor, interrupt mask 7 */
    GEN_CPU_SP_INIT      = 0x00FF0000,
    GEN_CPU_PC_RESET     = 0x000000,
    GEN_VECTOR_SSP       = 0x000000, /* Reset: initial SSP (32-bit) */
    GEN_VECTOR_PC        = 0x000004, /* Reset: initial PC (32-bit) */
    GEN_VECTOR_ILLEGAL   = 4,        /* ILLEGAL opcode */
    GEN_VECTOR_CHK       = 6,        /* CHK out of bounds */
};

/* ===== CPU 68000: opcodes ===== */

enum {
    GEN_OP_NOP           = 0x4E71,
    GEN_OP_RTS           = 0x4E75,
    GEN_OP_RTE           = 0x4E73,
    GEN_OP_RTR           = 0x4E77,
    GEN_OP_RESET         = 0x4E70,
    GEN_OP_STOP          = 0x4E72,
    GEN_OP_TRAP          = 0x4E40,   /* 4E4x: TRAP #n */
    GEN_OP_TRAP_MASK     = 0xFFF0,
    GEN_OP_MOVEQ         = 0x7000,   /* 0x7xxx: MOVEQ #imm,Dn */
    GEN_OP_MOVEQ_MASK    = 0xF100,
    GEN_OP_BRA           = 0x6000,   /* 0x60xx: BRA */
    GEN_OP_BRA_MASK      = 0xFF00,
    GEN_OP_TST           = 0x4A00,   /* 4Axx: TST <ea> */
    GEN_OP_TST_MASK      = 0xFF00,
    GEN_OP_MOVEA_W       = 0x3040,   /* MOVEA.W <ea>,An */
    GEN_OP_MOVEA_L       = 0x2040,   /* MOVEA.L <ea>,An */
    GEN_OP_MOVEA_MASK    = 0xFFC0,
    GEN_OP_LEA           = 0x41C0,   /* LEA <ea>,An */
    GEN_OP_JMP           = 0x4EC0,   /* JMP <ea> */
    GEN_OP_JSR           = 0x4E80,   /* JSR <ea> */
};

/* TST: bits 7-6 = size (00=B, 01=W, 10=L) */
enum {
    GEN_TST_SIZE_B = 0,
    GEN_TST_SIZE_W = 1,
    GEN_TST_SIZE_L = 2,
};

/* EA: bits 5-0 del opcode. mode = (ea>>3)&7, reg = ea&7 */
enum {
    GEN_EA_MASK         = 0x3F,
    GEN_EA_MODE_SHIFT   = 3,
    GEN_EA_REG_MASK     = 7,
    GEN_EA_MODE_POSTINC = 3,   /* (An)+ */
    GEN_EA_MODE_PREDEC  = 4,   /* -(An) */
};

/* Shift/rotate: bits 11-9=count (0=8, 1-7=1-7), 8=ir, 7-6=size, 5-3=reg, 2-0=op */
enum {
    GEN_OP_ASR           = 0xE000,   /* ASR #n,Dn o ASR Dm,Dn */
    GEN_OP_LSR           = 0xE008,   /* LSR #n,Dn o LSR Dm,Dn */
    GEN_OP_ROR           = 0xE018,   /* ROR */
    GEN_OP_ROXR          = 0xE010,   /* ROXR (rotate with X) */
    GEN_OP_ASL           = 0xE100,   /* ASL (= LSL para flags) */
    GEN_OP_LSL           = 0xE108,   /* LSL #n,Dn o LSL Dm,Dn */
    GEN_OP_ROXL          = 0xE110,   /* ROXL (rotate with X) */
    GEN_OP_ROL           = 0xE118,   /* ROL */
    GEN_OP_SHIFT_MASK    = 0xFFF8,   /* bits 2-0 = op subtype */
    GEN_OP_MOVEM_STORE   = 0x4800,   /* MOVEM regs,mem; 0100 10xx xxxx xxxx */
    GEN_OP_MOVEM_LOAD    = 0x4C00,   /* MOVEM mem,regs; 0100 11xx xxxx xxxx */
    GEN_OP_MOVEM_MASK    = 0xFE00,   /* ignora bit 10 (size) */
    GEN_OP_ADDQ          = 0x5000,   /* ADDQ #imm,ea; bit 8=0 */
    GEN_OP_SUBQ          = 0x5100,   /* SUBQ #imm,ea; bit 8=1 */
    GEN_OP_ADDQ_SUBQ_MASK = 0xF100,
    GEN_OP_EXT_W         = 0x4880,   /* EXT.W Dn; bits 7-6=10 */
    GEN_OP_EXT_L         = 0x48C0,   /* EXT.L Dn; bits 7-6=11 */
    GEN_OP_EXT_MASK      = 0xFFC0,
    GEN_OP_SWAP          = 0x4840,   /* SWAP Dn; 0x4840|n */
    GEN_OP_SWAP_MASK     = 0xFFF8,
    GEN_OP_NEG           = 0x4400,   /* NEG <ea>; 0x44xx */
    GEN_OP_NEG_MASK      = 0xFF00,
    GEN_OP_NOT           = 0x4600,   /* NOT <ea>; 0x46xx */
    GEN_OP_NOT_MASK      = 0xFF00,
    GEN_OP_EXG           = 0xC100,   /* EXG Rx,Ry; 0xC1xx, opmode 0x08/0x48/0x88 */
    GEN_OP_EXG_MASK      = 0xF100,
    GEN_OP_EXG_OPS       = 0x00F8,   /* 0x08=D,D 0x48=A,A 0x88=D,A */
    GEN_OP_PEA           = 0x4840,   /* PEA <ea>; 0x4840 + EA */
    GEN_OP_PEA_MASK      = 0xFFC0,
    GEN_OP_LINK          = 0x4E50,   /* LINK An,#imm; 0x4E50|n */
    GEN_OP_LINK_MASK     = 0xFFF8,
    GEN_OP_UNLK          = 0x4E58,   /* UNLK An; 0x4E58|n */
    GEN_OP_UNLK_MASK     = 0xFFF8,
    GEN_OP_SCC           = 0x50C0,   /* Scc <ea>; 0x50xx, bits 7-6 != 11 (DBcc) */
    GEN_OP_SCC_MASK      = 0xF0C0,
    GEN_OP_DBCC          = 0x50C0,   /* DBcc Dn,label; bits 7-6 = 11 */
    /* ADDA: opmode 011=word, 111=long. SUB/CMP excluyen estos. */
    GEN_OP_ADDA_W        = 0xD0C0,   /* ADDA.W <ea>,An */
    GEN_OP_ADDA_L        = 0xD1C0,   /* ADDA.L <ea>,An */
    GEN_OP_SUBA_W        = 0x90C0,   /* SUBA.W <ea>,An */
    GEN_OP_SUBA_L        = 0x91C0,   /* SUBA.L <ea>,An */
    GEN_OP_CMPA_W        = 0xB0C0,   /* CMPA.W <ea>,An */
    GEN_OP_CMPA_L        = 0xB1C0,   /* CMPA.L <ea>,An */
    GEN_OP_ADDA_SUBA_CMPA_MASK = 0xF0C0,
    /* Immediate: 0x0200 ORI, 0x0400 SUBI, 0x0600 ADDI, 0x0A00 EORI, 0x0C00 CMPI, 0x0000 ANDI */
    GEN_OP_ORI           = 0x0000,   /* ORI #imm,<ea>; 0x00xx */
    GEN_OP_ANDI          = 0x0200,   /* ANDI #imm,<ea>; 0x02xx */
    GEN_OP_SUBI          = 0x0400,   /* SUBI #imm,<ea>; 0x04xx */
    GEN_OP_ADDI          = 0x0600,   /* ADDI #imm,<ea>; 0x06xx */
    GEN_OP_EORI          = 0x0A00,   /* EORI #imm,<ea>; 0x0Axx */
    GEN_OP_CMPI          = 0x0C00,   /* CMPI #imm,<ea>; 0x0Cxx */
    GEN_OP_IMM_MASK      = 0xFE00,
    /* Bit ops: 0x0800 BCHG, 0x0840 BCLR, 0x0880 BSET, 0x0800 BTST (reg) */
    GEN_OP_BTST          = 0x0800,   /* BTST; bits 7-6: 00=Dn, 01=#imm */
    GEN_OP_BCHG          = 0x0840,   /* bits 8-6: 001 */
    GEN_OP_BCLR          = 0x0880,   /* bits 8-6: 010 */
    GEN_OP_BSET          = 0x08C0,   /* bits 8-6: 011 */
    GEN_OP_BIT_MASK      = 0xF0C0,   /* BTST 0x0800/0x0900, BCHG 0x0840, BCLR 0x0880, BSET 0x08C0 */
    /* MULU 0xC0xx, MULS 0xC1xx; DIVU 0x80xx, DIVS 0x81xx */
    GEN_OP_MULU          = 0xC0C0,
    GEN_OP_MULS          = 0xC1C0,
    GEN_OP_DIVU          = 0x80C0,
    GEN_OP_DIVS          = 0x81C0,
    GEN_OP_MUL_DIV_MASK  = 0xF0C0,
    GEN_OP_MUL_MASK      = 0xFFC0,
    GEN_OP_DIV_MASK      = 0xFFC0,
    /* ADDX, SUBX: 0xD0xx (Dy,Dx) or -(Ay),-(Ax) */
    GEN_OP_ADDX          = 0xD100,   /* ADDX Dy,Dx or -(Ay),-(Ax) */
    GEN_OP_SUBX          = 0x9100,   /* SUBX */
    GEN_OP_NEGX          = 0x4000,   /* NEGX 0x40xx */
    GEN_OP_NEGX_MASK     = 0xFF00,
    /* TAS 0x4AC0, MOVE SR 0x40C0, MOVE CCR 0x42C0, MOVE to SR 0x46C0, CHK 0x4180 */
    GEN_OP_TAS           = 0x4AC0,
    GEN_OP_TAS_MASK      = 0xFFC0,
    GEN_OP_MOVE_FROM_SR  = 0x40C0,
    GEN_OP_MOVE_TO_CCR   = 0x44C0,
    GEN_OP_MOVE_TO_SR    = 0x46C0,
    GEN_OP_CHK           = 0x4180,
    GEN_OP_CHK_MASK      = 0xF1C0,
    GEN_OP_ILLEGAL       = 0x4AFC,
};

/* MOVEM: bits del opcode y tamaños en bytes */
enum {
    GEN_MOVEM_SIZE_BIT   = 10,   /* 0=word, 1=long */
    GEN_MOVEM_REGS       = 16,   /* D0-D7, A0-A7 */
    GEN_MOVEM_STEP_WORD  = 2,
    GEN_MOVEM_STEP_LONG  = 4,
};

/* Ciclos por instrucción (68000) */
enum {
    GEN_CYCLES_NOP       = 4,
    GEN_CYCLES_RTS       = 16,
    GEN_CYCLES_RTE       = 20,
    GEN_CYCLES_RTR       = 20,
    GEN_CYCLES_TRAP      = 34,
    GEN_CYCLES_MOVEQ     = 4,
    GEN_CYCLES_BRA       = 10,
    GEN_CYCLES_TST       = 4,
    GEN_CYCLES_SHIFT     = 6,
    GEN_CYCLES_MOVEM_PER_REG = 8,
    GEN_CYCLES_ADDQ_SUBQ    = 4,
    GEN_CYCLES_EXT          = 4,
    GEN_CYCLES_SWAP         = 4,
    GEN_CYCLES_NEG_NOT      = 4,
    GEN_CYCLES_EXG          = 6,
    GEN_CYCLES_PEA          = 12,
    GEN_CYCLES_LINK         = 16,
    GEN_CYCLES_UNLK         = 12,
    GEN_CYCLES_SCC          = 4,
    GEN_CYCLES_ADDA_SUBA_CMPA = 8,
    GEN_CYCLES_IMM          = 8,
    GEN_CYCLES_BIT          = 4,
    GEN_CYCLES_MULU         = 70,
    GEN_CYCLES_MULS         = 70,
    GEN_CYCLES_DIVU         = 140,
    GEN_CYCLES_DIVS         = 158,
    GEN_CYCLES_ADDX_SUBX    = 4,
    GEN_CYCLES_NEGX         = 4,
    GEN_CYCLES_TAS          = 4,
    GEN_CYCLES_CHK          = 10,
    GEN_CYCLES_ILLEGAL      = 34,
    GEN_CYCLES_RESET        = 132,
    GEN_CYCLES_STOP         = 4,
};

/* ADDQ/SUBQ: imm en bits 11-9; 0=8, 1-7=1-7 */
enum {
    GEN_ADDQ_IMM_SHIFT   = 9,
    GEN_ADDQ_IMM_MASK    = 7,
};

/* ===== VDP: registros y memoria ===== */

enum {
    GEN_VDP_REG_COUNT    = 24,
    GEN_VDP_DATA_PORT    = 0,
    GEN_VDP_CTRL_PORT    = 1,
    GEN_VDP_VRAM_SIZE    = 0x10000,   /* 64 KB */
    GEN_VDP_CRAM_SIZE    = 64,        /* 64 colores (words) */
    GEN_VDP_VSRAM_SIZE   = 40,        /* 40 words */
    GEN_VDP_TILE_SIZE    = 8,
    GEN_VDP_TILES_W      = 40,   /* 320/8 */
    GEN_VDP_TILES_H      = 28,   /* 224/8 */
};

/* VDP command word: bits 31-30 = code, 23-16|14-3|1-0 = address */
enum {
    GEN_VDP_CMD_VRAM_READ   = 0x000000,
    GEN_VDP_CMD_VRAM_WRITE  = 0x400000,
    GEN_VDP_CMD_CRAM_WRITE  = 0xC00000,
    GEN_VDP_CMD_VSRAM_WRITE = 0x400000,  /* 0x10 en bits 5-4 */
};

/* ===== Joypad ===== */

enum {
    GEN_JOYPAD_BITS      = 12,        /* 6 botones × 2 puertos */
    GEN_JOYPAD_UP        = 0,
    GEN_JOYPAD_DOWN      = 1,
    GEN_JOYPAD_LEFT      = 2,
    GEN_JOYPAD_RIGHT     = 3,
    GEN_JOYPAD_B         = 4,
    GEN_JOYPAD_C         = 5,
    GEN_JOYPAD_A         = 6,
    GEN_JOYPAD_START     = 7,
    GEN_JOYPAD_Z         = 8,
    GEN_JOYPAD_Y         = 9,
    GEN_JOYPAD_X         = 10,
    GEN_JOYPAD_MODE      = 11,
};

/* ===== Test pattern (stub) ===== */

enum {
    GEN_TEST_BAR_WIDTH   = 32,
    GEN_TEST_PALETTE_MAX = 16,
};

#endif /* BITEMU_GENESIS_CONSTANTS_H */
