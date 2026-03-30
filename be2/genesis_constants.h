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

/* PAL (Mega Drive EU): ~7.61 MHz 68000, 313 líneas activas de scan, 50 Hz */
enum {
    GEN_68000_HZ_PAL         = 7609488,
    GEN_SCANLINES_TOTAL_PAL  = 313,
    GEN_SCANLINES_VISIBLE_PAL = 224, /* activas en modo 240/224; V-blank empieza tras la última línea visible */
    GEN_FPS_PAL              = 50,
    GEN_CYCLES_PER_LINE_PAL  = 488,
    GEN_CYCLES_PER_FRAME_PAL = GEN_SCANLINES_TOTAL_PAL * GEN_CYCLES_PER_LINE_PAL,
};

/*
 * Stall 68k por palabra DMA: distinto en zona activa vs vblank (hueco CPU mayor con rayo off).
 * Valores aproximados respecto a tablas tipo "DMA speed test" / GPGX; no son slot-exact.
 */
enum {
    GEN_VDP_DMA_STALL_FILL_PER_WORD_ACTIVE   = 6,
    GEN_VDP_DMA_STALL_FILL_PER_WORD_VBLANK   = 4,
    GEN_VDP_DMA_STALL_COPY_PER_WORD_ACTIVE  = 10,
    GEN_VDP_DMA_STALL_COPY_PER_WORD_VBLANK  = 6,
    GEN_VDP_DMA_STALL_68K_PER_WORD_ACTIVE   = 12,
    GEN_VDP_DMA_STALL_68K_PER_WORD_VBLANK   = 5,
    /* Alias histórico = activo (DMA típico en pantalla) */
    GEN_VDP_DMA_STALL_FILL_PER_WORD = GEN_VDP_DMA_STALL_FILL_PER_WORD_ACTIVE,
    GEN_VDP_DMA_STALL_COPY_PER_WORD = GEN_VDP_DMA_STALL_COPY_PER_WORD_ACTIVE,
    GEN_VDP_DMA_STALL_68K_PER_WORD  = GEN_VDP_DMA_STALL_68K_PER_WORD_ACTIVE,
    GEN_VDP_DMA_STALL_CYCLES_PER_WORD = GEN_VDP_DMA_STALL_68K_PER_WORD_ACTIVE,
};

/* Retraso mínimo 68k tras escribir BUSREQ (A11100 bit0=1) antes de acceso estable a RAM Z80. */
enum {
    GEN_Z80_BUSACK_CYCLES_68K = 48,
};

/* Opcional: menos ciclos por frame solo para pruebas locales rápidas (no usado en `cycles_per_frame`). */
enum {
    GEN_CYCLES_PER_FRAME_STUB = 48868,
};

/* ===== Audio ===== */
/*
 * YM2612 busy (~11.4 µs tras escribir puerto datos a escala bus 68k NTSC ≈ 86–92 ciclos;
 * afinado hacia mediciones / test ROMs FM; ver Documentación genesis-test-roms).
 */
enum {
    GEN_YM2612_WRITE_BUSY_CYCLES_68K = 88,
};
enum {
    GEN_SAMPLE_RATE          = 44100,
    GEN_CYCLES_PER_SAMPLE    = GEN_68000_HZ / GEN_SAMPLE_RATE,  /* ~174 */
    GEN_CYCLES_PER_SAMPLE_PAL = GEN_68000_HZ_PAL / GEN_SAMPLE_RATE,
    GEN_PSG_HZ               = 3579545,   /* NTSC: master/15 */
    GEN_PSG_HZ_PAL           = 3546894,   /* PAL subcarrier-derived clock */
    GEN_PSG_CYCLES_PER_SAMPLE = GEN_PSG_HZ / GEN_SAMPLE_RATE,   /* ~81 */
    GEN_PSG_CYCLES_PER_SAMPLE_PAL = GEN_PSG_HZ_PAL / GEN_SAMPLE_RATE,
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
/* VDP reg 10 (0x0A): contador de líneas entre H-interrupts (0 = 256 líneas) */
enum {
    GEN_VDP_REG0_IE1 = 0x20,
    GEN_VDP_REG1_IE0 = 0x20,
    GEN_VDP_REG_HINT = 10,
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
    GEN_DISPLAY_WIDTH       = 320,
    GEN_DISPLAY_HEIGHT      = 224,
    GEN_DISPLAY_PIXELS      = GEN_DISPLAY_WIDTH * GEN_DISPLAY_HEIGHT,
    /* Framebuffer Host RGB888 (3 bytes/píxel); la API `bitemu_get_framebuffer` devuelve este layout en Genesis. */
    GEN_FB_BYTES_PER_PIXEL  = 3,
    GEN_FB_STRIDE           = GEN_DISPLAY_WIDTH * GEN_FB_BYTES_PER_PIXEL,
    GEN_FB_SIZE             = GEN_DISPLAY_PIXELS * GEN_FB_BYTES_PER_PIXEL,
    GEN_PIXELS_PER_LINE     = GEN_DISPLAY_WIDTH,
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

    /* SRAM (backup RAM, 0x200000-0x20FFFF). Enable: write 1 to 0xA130F1 */
    GEN_ADDR_SRAM_START  = 0x200000,
    GEN_ADDR_SRAM_END    = 0x20FFFF,
    GEN_SRAM_SIZE        = 0x10000,   /* 64 KB */
    GEN_SRAM_MASK        = 0xFFFF,
    GEN_ADDR_SRAM_ENABLE = 0xA130F1,
    /* Mapper Super Street Fighter II (y similares): bancos 512KB en 0x000000-0x3FFFFF.
     * Escrituras en direcciones impares 0xA130F3, 0xA130F5, … seleccionan página ROM por slot. */
    GEN_ADDR_SSF2_SLOT0  = 0xA130F3,
    GEN_ADDR_SSF2_SLOT7  = 0xA130FF,
    GEN_SSF2_SLOT_SHIFT  = 19,       /* 512 KiB por slot */
    GEN_SSF2_SLOT_SIZE   = 0x80000u,
    GEN_SSF2_SLOT_COUNT  = 8,

    /* Lock-on: S&K 2MB + locked 2MB. Patch (Sonic 2 & K) = 256KB extra. */
    GEN_LOCKON_SIZE      = 4 * 1024 * 1024,
    GEN_LOCKON_PATCH     = 256 * 1024,

    /* Z80 space: 68k accede cuando tiene el bus */
    GEN_ADDR_Z80_START   = 0xA00000,
    GEN_ADDR_Z80_END     = 0xA0FFFF,
    GEN_Z80_RAM_SIZE     = 8192,   /* 8KB RAM del Z80 */

    /* I/O: version, joypad, expansion */
    GEN_ADDR_IO_START    = 0xA10000,
    GEN_ADDR_IO_END      = 0xA1001F,
    GEN_IO_VERSION       = 0xA10000,
    GEN_IO_VERSION_VAL   = 0x00,      /* Model 1: 0x00 = NTSC JP */
    GEN_IO_JOYPAD1_DATA  = 0xA10002,
    GEN_IO_JOYPAD1_CTRL  = 0xA10003,
    GEN_IO_JOYPAD2_DATA  = 0xA10004,
    GEN_IO_JOYPAD2_CTRL  = 0xA10005,
    GEN_IO_JOYPAD1       = 0xA10002,  /* alias */
    GEN_IO_JOYPAD2       = 0xA10004,
    GEN_IO_EXPANSION     = 0xA10006,
    GEN_JOYPAD_TH        = 0x40,
    GEN_JOYPAD_TR        = 0x80,

    /* Z80 bus request / reset (offset en 0xA1xxxx) */
    GEN_ADDR_Z80_BUSREQ  = 0xA11100,
    GEN_ADDR_Z80_RESET   = 0xA11200,
    GEN_Z80_BUSREQ_OFF   = 0x1100,    /* (addr & GEN_ADDR_OFFSET_MASK) */
    GEN_Z80_RESET_OFF    = 0x1200,
    GEN_ADDR_OFFSET_MASK = 0xFFFF,    /* low 16 bits para Z80 bus decode */
    GEN_IO_UNMAPPED_READ = 0xFF,      /* open bus MVP: byte leído sin dispositivo (no modelo prefetch 68k) */

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

/* ROM header: 0x1B0-0x1B1 "RA" = save RAM presente; 0x1B4-0x1BB rango SRAM (BE32) */
enum {
    GEN_HEADER_SRAM_OFF   = 0x1B0,
    GEN_HEADER_SRAM_MAGIC = 0x5241,   /* "RA" big-endian */
    GEN_HEADER_SRAM_START_OFF = 0x1B4, /* inicio backup RAM en espacio cartucho */
    GEN_HEADER_SRAM_END_OFF   = 0x1B8, /* fin (inclusive) */
    GEN_HEADER_REGION_OFF = 0x1F0,      /* 4 chars región cartucho; PAL si solo 'E' (sin J/U) */
};

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
    GEN_VECTOR_ADDRESS_ERROR = 3,    /* Word/long en dirección impar */
    GEN_VECTOR_ILLEGAL   = 4,        /* ILLEGAL opcode */
    GEN_VECTOR_ZERO_DIV  = 5,        /* DIVU/DIVS por cero */
    GEN_VECTOR_CHK       = 6,        /* CHK out of bounds */
    GEN_VECTOR_PRIVILEGE_VIOLATION = 8,
};

/* ===== CPU 68000: opcodes ===== */

enum {
    GEN_OP_NOP           = 0x4E71,
    GEN_OP_RTS           = 0x4E75,
    GEN_OP_RTE           = 0x4E73,
    GEN_OP_RTD           = 0x4E74,
    GEN_OP_TRAPV         = 0x4E76,
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

/* Shift/rotate reg: línea 0xE, bits 11-9 count/Dm, 8=ir, 7-6=size, 5-3=Dn; 2-0 tipo (68000) */
enum {
    GEN_OP_ASL           = 0xE000,   /* …000 */
    GEN_OP_LSL           = 0xE001,   /* …001 */
    GEN_OP_LSR           = 0xE002,   /* …010 */
    GEN_OP_ASR           = 0xE003,   /* …011 */
    GEN_OP_ROL           = 0xE004,   /* …100 */
    GEN_OP_ROR           = 0xE005,   /* …101 */
    GEN_OP_ROXL          = 0xE006,   /* …110 */
    GEN_OP_ROXR          = 0xE007,   /* …111 */
    GEN_OP_SHIFT_MASK    = 0xF007,   /* nibble alto + bits 2-0 */
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
    GEN_CYCLES_ABCD_REG     = 6,
    GEN_CYCLES_ABCD_MEM     = 18,
    GEN_CYCLES_NEGX         = 4,
    GEN_CYCLES_TAS          = 4,
    GEN_CYCLES_CHK          = 10,
    GEN_CYCLES_ILLEGAL      = 34,
    GEN_CYCLES_RESET        = 132,
    /* STOP: fetch + inmediato 16 (68000 UM ~8 ciclos bus interno). */
    GEN_CYCLES_STOP         = 8,
    /* Address Error: agrupación exception (~50 ciclos; marco largo real no modelado). */
    GEN_CYCLES_ADDRESS_ERROR = 50,
};

/* ADDQ/SUBQ: imm en bits 11-9; 0=8, 1-7=1-7 */
enum {
    GEN_ADDQ_IMM_SHIFT   = 9,
    GEN_ADDQ_IMM_MASK    = 7,
};

/* ===== VDP: registros y memoria ===== */

enum {
    GEN_VDP_REG_COUNT    = 24,
    GEN_VDP_REG_ADDR_INC = 15,        /* reg 0x0F: auto-increment */
    GEN_VDP_DATA_PORT    = 0,
    GEN_VDP_CTRL_PORT    = 1,
    GEN_VDP_VRAM_SIZE    = 0x10000,   /* 64 KB */
    GEN_VDP_VRAM_WORDS   = GEN_VDP_VRAM_SIZE / 2,
    GEN_VDP_ADDR_MASK    = 0x3FFF,    /* 14-bit address */
    GEN_VDP_CRAM_SIZE    = 64,        /* 64 colores (words) */
    GEN_VDP_CRAM_MASK    = 0x3F,      /* index 0-63 */
    GEN_VDP_CRAM_COLOR   = 0x0EEE,    /* 9-bit: BBBGGGRRR, low bit always 0 */
    GEN_VDP_VSRAM_SIZE   = 40,        /* 40 words */
    GEN_VDP_VSRAM_MASK   = 0x3FF,     /* 10-bit scroll value */
    GEN_VDP_TILE_SIZE    = 8,
    GEN_VDP_TILE_BYTES   = 32,        /* 8×8×4 bpp */
    GEN_VDP_TILES_W      = 40,        /* 320/8 */
    GEN_VDP_TILES_H      = 28,        /* 224/8 */
    GEN_VDP_NAMETABLE_W  = 64,        /* words per map row */
    GEN_VDP_PLANE_UNIT   = 0x2000,    /* reg 2: base × this */
    GEN_VDP_PLANE_A_SHIFT = 3,        /* reg 2 bits 4-2 → shift 3 */
    GEN_VDP_PLANE_B_SHIFT = 0,        /* reg 4 bits 2-0 = SB15-13 */
    GEN_VDP_TILE_ROW_BYTES = 4,       /* 8 px × 4 bpp = 4 bytes/row */
    GEN_VDP_TILE_HI_BYTE   = 8,       /* bit offset: high byte en word */
};

/* VDP command: bits 31-30 = type, 0x80 = register write */
enum {
    GEN_VDP_CMD_TYPE_MASK  = 0xC0000000,
    GEN_VDP_CMD_REG_WRITE  = 0x80000000,
    GEN_VDP_CMD_REG_SHIFT  = 24,
    GEN_VDP_CMD_REG_NUM    = 0x1F,
    GEN_VDP_CMD_DATA_MASK  = 0xFF,
    GEN_VDP_CMD_ADDR_HI    = 0x3FFF,  /* bits 29-16 */
    GEN_VDP_CMD_ADDR_LO    = 3,       /* bits 1-0 → addr 15-14 */
    GEN_VDP_CMD_CODE_SHIFT = 28,
    GEN_VDP_CMD_CODE_MASK  = 0x0F,
};

/* VDP access codes (code_reg) */
enum {
    GEN_VDP_CODE_VRAM_READ  = 0,
    GEN_VDP_CODE_VRAM_WRITE = 1,
    GEN_VDP_CODE_CRAM_WRITE = 3,
    GEN_VDP_CODE_VSRAM_MASK = 0x0E,
    GEN_VDP_CODE_VSRAM_BIT  = 4,
    GEN_VDP_CODE_VSRAM_WRITE = 5,
};

/* DMA: CD5 = bit 5 del code (comando 32-bit, code = (cmd>>30)&3 | (cmd>>2)&0x3C) */
enum {
    GEN_VDP_CMD_CD5_BIT     = 5,
    GEN_VDP_DMA_ENABLE_BIT  = 4,   /* reg 1 bit 4: DMA enable */
};

/* DMA regs: 19-20 length (bytes), 21-22 source low, 23 source high + type */
enum {
    GEN_VDP_REG_DMA_LEN_LO  = 19,
    GEN_VDP_REG_DMA_LEN_HI  = 20,
    GEN_VDP_REG_DMA_SRC_LO  = 21,
    GEN_VDP_REG_DMA_SRC_HI  = 22,
    GEN_VDP_REG_DMA_SRC_EXT = 23,
};

/* reg[23] >> 4: 0x8-0xB = Fill, 0xC-0xF = Copy, 0x0-0x7 = 68k bus */
enum {
    GEN_VDP_DMA_TYPE_68K    = 0,
    GEN_VDP_DMA_TYPE_FILL   = 2,
    GEN_VDP_DMA_TYPE_COPY   = 3,
};
/* En DMA 68k, (reg[23]>>4) impar: avance dirección fuente +1 (MOVE8); par: +2 (MOVE16). */
enum {
    GEN_VDP_DMA68K_MOVE8_NIBBLE_BIT = 1,
};

/* VDP 16-bit register write: bits 15-14 = 10 */
enum {
    GEN_VDP_REG16_MASK  = 0xC000,
    GEN_VDP_REG16_VAL   = 0x8000,
    GEN_VDP_REG16_REG   = 8,
    GEN_VDP_ADDR_INC_DEF = 2,
    GEN_VDP_ADDR_INC_MASK = 0xFF,     /* reg 0x0F: 8-bit increment */
};

/* CRAM: Genesis color 0x0BBBGGGRRR → RGB 3 bits each */
enum {
    GEN_VDP_CRAM_R_BITS = 3,
    GEN_VDP_CRAM_R_MAX  = 7,
    GEN_VDP_CRAM_GRAY_MAX = 21,       /* 7+7+7 */
};

/* Tile map word: bits 10-0 = index, 11-12 = flip, 13-14 = palette, 15 = priority */
enum {
    GEN_VDP_TILE_IDX_MASK   = 0x7FF,
    GEN_VDP_TILE_FLIP_X    = 11,
    GEN_VDP_TILE_FLIP_Y    = 12,
    GEN_VDP_TILE_PAL_SHIFT = 13,
    GEN_VDP_TILE_PAL_MASK  = 7,
    GEN_VDP_TILE_PRIO_BIT  = 15,
    GEN_VDP_PALETTE_SIZE   = 16,
};

/* Sprite Attribute Table: reg 5 bits 6-1 = base / 0x200 */
enum {
    GEN_VDP_SAT_BASE_SHIFT = 9,       /* addr = (reg5 & 0x7E) << 9 */
    GEN_VDP_SAT_BASE_MASK  = 0x7F,   /* bits 6-0 = AT15-9 */
    GEN_VDP_SAT_ENTRY_BYTES = 8,
    GEN_VDP_SAT_ENTRY_WORDS = 4,
    GEN_VDP_SPRITE_MAX      = 80,
    GEN_VDP_MAX_SPRITES_PER_LINE = 20,   /* límite efectivo NTSC modo 5 (SAT) */
};

/* Sprite pattern base: reg 6 bits 5-0 = base bits 13-8 */
enum {
    GEN_VDP_SPR_PAT_BASE_SHIFT = 8,   /* (reg6 & 0x3F) << 8 */
    GEN_VDP_SPR_PAT_BASE_MASK  = 0x3F,
};

/* Sprite size (word 1 bits 5-4): 00=8x8, 01=8x16, 10=16x16, 11=8x32 */
enum {
    GEN_VDP_SPR_SIZE_SHIFT = 4,
    GEN_VDP_SPR_SIZE_MASK  = 3,
    GEN_VDP_SPR_SIZE_8x8   = 0,
    GEN_VDP_SPR_SIZE_8x16  = 1,
    GEN_VDP_SPR_SIZE_16x16 = 2,
    GEN_VDP_SPR_SIZE_8x32  = 3,
};

/* Sprite word 2: priority, palette, flip, tile index */
enum {
    GEN_VDP_SPR_PRIO_BIT  = 15,
    GEN_VDP_SPR_PAL_SHIFT = 13,
    GEN_VDP_SPR_PAL_MASK  = 3,
    GEN_VDP_SPR_FLIP_V    = 12,
    GEN_VDP_SPR_FLIP_H    = 11,
    GEN_VDP_SPR_TILE_MASK = 0x7FF,
};

/* Reg 7: background color - bits 5-4 = palette, 3-0 = index */
enum {
    GEN_VDP_BG_PAL_SHIFT = 4,
    GEN_VDP_BG_PAL_MASK  = 3,
    GEN_VDP_BG_IDX_MASK  = 0x0F,
};

/* Reg 12 (0x8C): mode 4 - bits 1-0 = H32 (00) vs H40 (11) */
enum {
    GEN_VDP_REG_MODE4     = 12,
    GEN_VDP_H40_MASK      = 3,    /* bits 1-0: 11=40 tiles, 00=32 tiles */
    GEN_VDP_H40_VAL       = 3,    /* 40 tiles = 320 px */
    GEN_DISPLAY_WIDTH_H32 = 256,
};

/* Reg 3: Window plane base - bits 6-4 = WA15-13, addr = ((reg>>4)&7) * 0x2000 */
enum {
    GEN_VDP_REG_WINDOW   = 3,
    GEN_VDP_WINDOW_SHIFT = 4,
    GEN_VDP_WINDOW_MASK  = 0x70,   /* bits 6-4 */
};

/* Reg 13 (0x8D): H scroll table base - (reg & 0x3F) * 0x400 bytes */
enum {
    GEN_VDP_REG_HSCROLL   = 13,
    GEN_VDP_HSCROLL_BASE_MASK  = 0x3F,
    GEN_VDP_HSCROLL_BASE_WORDS = 0x200,  /* 0x400 bytes / 2 */
};

/* Reg 11 (0x8B): bits 1-0 modo scroll horizontal; bits 3-2 modo scroll vertical.
 * 00=pantalla completa; 10=celda 8 líneas; 11=por línea; 01=reservado (tratamos como 00). */
enum {
    GEN_VDP_REG_MODE3         = 11,
    GEN_VDP_MODE3_HSCROLL_MASK = 3,
    GEN_VDP_MODE3_VSCROLL_SHIFT = 2,
    GEN_VDP_MODE3_VSCROLL_MASK = 3,
};

/* Reg 17 (0x11): bit 7 = ventana alineada a la derecha (resto reservado en hardware) */
/* Reg 18: Window H pos (WX) - bits 6-0, window from x=0 to (WX+1)*8. WX+1>=32/40 = hidden */
/* Reg 19: Window V pos (WY) - bits 7-0, window from y=0 to (WY+1). WY+1>=224 = hidden */
enum {
    GEN_VDP_REG_WH = 17,
    GEN_VDP_WH_RIGHT_MASK = 0x80,
    GEN_VDP_REG_WX = 18,
    GEN_VDP_REG_WY = 19,
    GEN_VDP_WX_MASK = 0x7F,
    GEN_VDP_WY_MASK = 0xFF,
};

/* HBlank: ciclos desde fin de visible hasta fin de línea */
enum {
    GEN_VDP_HBLANK_CYCLES = 80,
};

/* HV counter: H visible 0-0x93, blank 0xE9-0xFF */
enum {
    GEN_VDP_H_VISIBLE_END = 0x93,
    GEN_VDP_H_VISIBLE_N   = 0x94,
    GEN_VDP_H_BLANK_START = 0xE9,
    GEN_VDP_HV_V_MASK     = 0xFF,     /* V en byte bajo del HV word */
};

/* ===== Joypad ===== */

enum {
    GEN_JOYPAD_IDLE      = 0x3F,      /* sin botones: TH high */
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
