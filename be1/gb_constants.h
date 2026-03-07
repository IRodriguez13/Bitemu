/**
 * Bitemu BE1 - Game Boy constantes
 *
 * Agrupadas por subsistema en enums para type-safety y legibilidad.
 * Solo quedan como #define los valores float o usados en macros del preprocesador.
 */

#ifndef BITEMU_GB_CONSTANTS_H
#define BITEMU_GB_CONSTANTS_H

/* Float: no puede ser enum */
#define GB_FPS  59.7275

/* ===== Timing (T-cycles @ 4.194304 MHz) ===== */

enum {
    GB_CPU_HZ            = 4194304,
    GB_DOTS_PER_SCANLINE = 456,
    GB_SCANLINES_VISIBLE = 144,
    GB_SCANLINES_VBLANK  = 10,
    GB_SCANLINES_TOTAL   = 154,
    GB_DOTS_PER_FRAME    = GB_DOTS_PER_SCANLINE * GB_SCANLINES_TOTAL, /* 70224 */
    GB_AUDIO_SAMPLE_RATE = 44100,
    GB_CYCLES_PER_SAMPLE = GB_CPU_HZ / GB_AUDIO_SAMPLE_RATE,         /* ~95 */
};

/* ===== APU: Audio Processing Unit ===== */

enum {
    /* NR52: master control */
    APU_NR52_POWER       = 0x80,
    APU_NR52_CH1         = 0x01,
    APU_NR52_CH2         = 0x02,
    APU_NR52_CH3         = 0x04,
    APU_NR52_CH4         = 0x08,
    APU_NR52_STATUS_MASK = 0x0F,

    /* NRx4: trigger / length enable */
    APU_TRIGGER_BIT      = 0x80,
    APU_LENGTH_ENABLE    = 0x40,

    /* NRx2: envelope (NR12, NR22, NR42) */
    APU_ENV_VOL_POS      = 4,       /* initial volume in bits 7-4 */
    APU_ENV_DIR_BIT      = 0x08,    /* bit 3: 1=increase, 0=decrease */
    APU_ENV_PERIOD_MASK  = 0x07,    /* bits 2-0: period (0=disabled) */

    /* NRx1: duty cycle (NR11, NR21) */
    APU_DUTY_POS         = 6,
    APU_DUTY_MASK        = 0x03,
    APU_DUTY_PATTERNS    = 4,
    APU_DUTY_STEPS       = 8,
    APU_DUTY_STEP_MASK   = APU_DUTY_STEPS - 1,

    /* 4-bit nibble operations */
    APU_NIBBLE_MASK      = 0x0F,
    APU_NIBBLE_SHIFT     = 4,

    /* NR30: wave DAC enable */
    APU_WAVE_DAC_ON      = 0x80,

    /* NR32: wave output level — bits 6-5 */
    APU_WAVE_VOL_POS     = 5,
    APU_WAVE_VOL_MASK    = 0x03,

    /* Wave RAM (0xFF30-0xFF3F): 16 bytes = 32 4-bit samples */
    APU_WAVE_RAM_OFFSET  = 0x30,
    APU_WAVE_SAMPLES     = 32,
    APU_WAVE_STEP_MASK   = APU_WAVE_SAMPLES - 1,

    /* Frequency registers (NRx3 low, NRx4 bits 2-0 high) */
    APU_FREQ_HI_MASK     = 0x07,
    APU_FREQ_BASE        = 2048,

    /* T-cycles per frequency timer tick */
    APU_SQUARE_PERIOD_MUL = 4,
    APU_WAVE_PERIOD_MUL   = 2,

    /* Noise (NR43) */
    APU_NOISE_DIV_MASK   = 0x07,
    APU_NOISE_DIV_MUL    = 16,
    APU_NOISE_DIV_ZERO   = 8,
    APU_NOISE_SHIFT_POS  = 4,
    APU_NOISE_WIDTH_BIT  = 0x08,

    /* LFSR */
    APU_LFSR_INIT        = 0x7FFF,
    APU_LFSR_TAP_BIT     = 14,
    APU_LFSR_SHORT_CLR   = 0x7FBF,
    APU_LFSR_SHORT_TAP   = 6,

    /* DAC / mixing */
    APU_DAC_MAX          = 15,
    APU_MASTER_VOL_MASK  = 0x07,
    APU_MASTER_VOL_R_POS = 4,
    APU_PAN_RIGHT_SHIFT  = 4,
    APU_MIX_SCALE        = 68,       /* 32767 / (60 * 8) ≈ 68 */
    APU_SAMPLE_MAX       = 32767,
    APU_SAMPLE_MIN       = (-32768),

    /* Frame sequencer (512 Hz) */
    APU_FRAME_SEQ_PERIOD = 8192,
    APU_FRAME_SEQ_STEPS  = 8,

    /* Length counter limits */
    APU_SQ_LENGTH_MAX    = 64,
    APU_WAVE_LENGTH_MAX  = 256,
    APU_LENGTH_LOAD_MASK = 0x3F,
};

/* ===== PPU ===== */

enum gb_ppu_mode {
    GB_PPU_MODE_HBLANK   = 0,
    GB_PPU_MODE_VBLANK   = 1,
    GB_PPU_MODE_OAM      = 2,
    GB_PPU_MODE_TRANSFER = 3,
};

enum {
    GB_PPU_MODE2_DOTS  = 80,
    GB_PPU_MODE3_MIN   = 172,
    GB_PPU_MODE3_MAX   = 289,
    GB_PPU_VBLANK_DOTS = GB_DOTS_PER_SCANLINE * GB_SCANLINES_VBLANK,
};

/* LCDC (0xFF40) */
enum {
    GB_LCDC_ON          = 0x80,
    GB_LCDC_WIN_TILEMAP = 0x40,
    GB_LCDC_WIN_ON      = 0x20,
    GB_LCDC_BG_TILES    = 0x10,  /* 0 = signed 0x9000, 1 = unsigned 0x8000 */
    GB_LCDC_BG_TILEMAP  = 0x08,  /* 0 = 0x9800, 1 = 0x9C00 */
    GB_LCDC_OBJ_H       = 0x04,  /* 0 = 8px, 1 = 16px */
    GB_LCDC_OBJ_ON      = 0x02,
    GB_LCDC_BG_ON       = 0x01,
};

/* STAT (0xFF41) */
enum {
    GB_STAT_LYC_INT   = 0x40,
    GB_STAT_OAM_INT   = 0x20,
    GB_STAT_VBL_INT   = 0x10,
    GB_STAT_HBL_INT   = 0x08,
    GB_STAT_LYC_EQ    = 0x04,
    GB_STAT_MODE_MASK = 0x03,
};

/* ===== Display ===== */

enum {
    GB_LCD_WIDTH           = 160,
    GB_LCD_HEIGHT          = 144,
    GB_TILE_SIZE           = 8,
    GB_TILES_PER_ROW       = 20,
    GB_TILES_PER_COL       = 18,
    GB_OAM_SPRITES_PER_LINE = 10,
};

enum {
    GB_TILE_DATA_UNSIGNED = 0x8000,
    GB_TILE_DATA_SIGNED   = 0x9000,
    GB_BG_MAP_LO          = 0x9800,
    GB_BG_MAP_HI          = 0x9C00,
};

/* ===== Timer: TAC (0xFF07) ===== */

enum {
    GB_TAC_ENABLE = 0x04,
    GB_TAC_RATE   = 0x03,
};

/* ===== I/O register offsets (from 0xFF00) ===== */

enum {
    GB_IO_JOYP = 0x00,
    GB_IO_SB   = 0x01,
    GB_IO_SC   = 0x02,
    GB_IO_DIV  = 0x04,
    GB_IO_TIMA = 0x05,
    GB_IO_TMA  = 0x06,
    GB_IO_TAC  = 0x07,
    GB_IO_IF   = 0x0F,

    /* APU registers */
    GB_IO_NR10 = 0x10,
    GB_IO_NR11 = 0x11,
    GB_IO_NR12 = 0x12,
    GB_IO_NR13 = 0x13,
    GB_IO_NR14 = 0x14,
    GB_IO_NR21 = 0x16,
    GB_IO_NR22 = 0x17,
    GB_IO_NR23 = 0x18,
    GB_IO_NR24 = 0x19,
    GB_IO_NR30 = 0x1A,
    GB_IO_NR31 = 0x1B,
    GB_IO_NR32 = 0x1C,
    GB_IO_NR33 = 0x1D,
    GB_IO_NR34 = 0x1E,
    GB_IO_NR41 = 0x20,
    GB_IO_NR42 = 0x21,
    GB_IO_NR43 = 0x22,
    GB_IO_NR44 = 0x23,
    GB_IO_NR50 = 0x24,
    GB_IO_NR51 = 0x25,
    GB_IO_NR52 = 0x26,

    /* PPU registers */
    GB_IO_LCDC = 0x40,
    GB_IO_STAT = 0x41,
    GB_IO_SCY  = 0x42,
    GB_IO_SCX  = 0x43,
    GB_IO_LY   = 0x44,
    GB_IO_LYC  = 0x45,
    GB_IO_DMA  = 0x46,
    GB_IO_BGP  = 0x47,
    GB_IO_OBP0 = 0x48,
    GB_IO_OBP1 = 0x49,
    GB_IO_WY   = 0x4A,
    GB_IO_WX   = 0x4B,
};

/* ===== Joypad (0xFF00) ===== */

enum {
    GB_JOYP_SEL_DIR   = 0x10,
    GB_JOYP_SEL_BTN   = 0x20,
    GB_JOYP_READ_MASK = 0x0F,
    GB_JOYP_RELEASED  = 0xFF,
    GB_JOYP_DIR_BIT   = 4,
    GB_JOYP_BTN_BIT   = 5,
    GB_JOYP_BIT_RIGHT  = 0,
    GB_JOYP_BIT_LEFT   = 1,
    GB_JOYP_BIT_UP     = 2,
    GB_JOYP_BIT_DOWN   = 3,
    GB_JOYP_BIT_A      = 0,
    GB_JOYP_BIT_B      = 1,
    GB_JOYP_BIT_SELECT = 2,
    GB_JOYP_BIT_START  = 3,
};

/* ===== Interrupts (IF/IE) ===== */

enum {
    GB_IF_VBLANK   = 0x01,
    GB_IF_LCDC     = 0x02,
    GB_IF_TIMER    = 0x04,
    GB_IF_SERIAL   = 0x08,
    GB_IF_JOYPAD   = 0x10,
    GB_IF_IE_MASK  = 0x1F,
    GB_IF_RESERVED = 0xE0,
};

enum {
    GB_IV_VBLANK = 0x40,
    GB_IV_LCDC   = 0x48,
    GB_IV_TIMER  = 0x50,
    GB_IV_SERIAL = 0x58,
    GB_IV_JOYPAD = 0x60,
};

/* ===== CPU / ALU ===== */

enum {
    GB_CPU_ENTRY      = 0x0100,
    GB_CPU_F_POSTBOOT = 0xB0,
    GB_BYTE_LO        = 0xFF,
    GB_WORD_LO        = 0xFFFF,
    GB_HALFCARRY_12   = 0xFFF,
    GB_IO_BASE        = 0xFF00,
    GB_F_UPPER        = 0xF0,
    GB_BCD_HI         = 0x99,
    GB_DAA_CARRY      = 0x60,
    GB_DAA_NIBBLE      = 6,
    GB_SP_INIT         = 0xFFFE,
};

#endif /* BITEMU_GB_CONSTANTS_H */
