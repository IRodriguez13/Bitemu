/**
 * Bitemu BE1 - Game Boy constantes
 *
 * Centraliza todos los valores literales de la implementación para evitar
 * "magic numbers" y documentar el significado de cada uno.
 */

#ifndef BITEMU_GB_CONSTANTS_H
#define BITEMU_GB_CONSTANTS_H

/* ----- Timing (dots = T-cycles @ 4.194304 MHz) ----- */
#define GB_CPU_HZ            4194304
#define GB_DOTS_PER_SCANLINE 456
#define GB_SCANLINES_VISIBLE 144
#define GB_SCANLINES_VBLANK  10
#define GB_SCANLINES_TOTAL   154
#define GB_DOTS_PER_FRAME   (GB_DOTS_PER_SCANLINE * GB_SCANLINES_TOTAL)  /* 70224 */
#define GB_FPS               59.7275
/* Audio: 44.1 kHz → una muestra cada ~95 ciclos de CPU */
#define GB_AUDIO_SAMPLE_RATE 44100
#define GB_CYCLES_PER_SAMPLE (GB_CPU_HZ / GB_AUDIO_SAMPLE_RATE)  /* 95 */

/* ----- PPU: modos y duraciones ----- */
#define GB_PPU_MODE_HBLANK  0
#define GB_PPU_MODE_VBLANK  1
#define GB_PPU_MODE_OAM     2
#define GB_PPU_MODE_TRANSFER 3

#define GB_PPU_MODE2_DOTS   80
#define GB_PPU_MODE3_MIN    172
#define GB_PPU_MODE3_MAX    289
#define GB_PPU_VBLANK_DOTS  (GB_DOTS_PER_SCANLINE * GB_SCANLINES_VBLANK)

/* LCDC (0xFF40): bits de control del LCD */
#define GB_LCDC_ON           0x80
#define GB_LCDC_WIN_TILEMAP  0x40
#define GB_LCDC_WIN_ON       0x20
#define GB_LCDC_BG_TILES     0x10   /* 0 = signed 0x9000, 1 = unsigned 0x8000 */
#define GB_LCDC_BG_TILEMAP   0x08   /* 0 = 0x9800, 1 = 0x9C00 */
#define GB_LCDC_OBJ_H        0x04   /* 0 = 8px, 1 = 16px */
#define GB_LCDC_OBJ_ON       0x02
#define GB_LCDC_BG_ON        0x01

/* STAT (0xFF41): bits de estado e interrupciones */
#define GB_STAT_LYC_INT  0x40
#define GB_STAT_OAM_INT  0x20
#define GB_STAT_VBL_INT  0x10
#define GB_STAT_HBL_INT  0x08
#define GB_STAT_LYC_EQ   0x04   /* LYC=LY comparison */
#define GB_STAT_MODE_MASK 0x03

/* ----- Display ----- */
#define GB_LCD_WIDTH        160
#define GB_LCD_HEIGHT       144
#define GB_TILE_SIZE        8
#define GB_TILES_PER_ROW    20
#define GB_TILES_PER_COL    18

/* VRAM: direcciones base de tiles */
#define GB_TILE_DATA_UNSIGNED 0x8000
#define GB_TILE_DATA_SIGNED   0x9000
#define GB_BG_MAP_LO         0x9800
#define GB_BG_MAP_HI         0x9C00

/** Máximo de sprites por scanline (hardware solo muestra los primeros 10 que coinciden). */
#define GB_OAM_SPRITES_PER_LINE 10

/* ----- Timer: TAC (0xFF07) ----- */
#define GB_TAC_ENABLE  0x04
#define GB_TAC_RATE    0x03

/* ----- IO offsets (desplazamiento desde 0xFF00) ----- */
#define GB_IO_JOYP          0x00
#define GB_IO_SB            0x01
#define GB_IO_SC            0x02
#define GB_IO_DIV           0x04
#define GB_IO_TIMA          0x05
#define GB_IO_TMA           0x06
#define GB_IO_TAC           0x07
#define GB_IO_NR10          0x10
#define GB_IO_NR11          0x11
#define GB_IO_NR12          0x12
#define GB_IO_NR13          0x13
#define GB_IO_NR14          0x14
#define GB_IO_NR21          0x16
#define GB_IO_NR22          0x17
#define GB_IO_NR23          0x18
#define GB_IO_NR24          0x19
#define GB_IO_NR30          0x1A
#define GB_IO_NR31          0x1B
#define GB_IO_NR32          0x1C
#define GB_IO_NR33          0x1D
#define GB_IO_NR34          0x1E
#define GB_IO_NR41          0x20
#define GB_IO_NR42          0x21
#define GB_IO_NR43          0x22
#define GB_IO_NR44          0x23
#define GB_IO_NR50          0x24
#define GB_IO_NR51          0x25
#define GB_IO_NR52          0x26
#define GB_IO_LCDC          0x40
#define GB_IO_STAT          0x41
#define GB_IO_SCY           0x42
#define GB_IO_SCX           0x43
#define GB_IO_LY            0x44
#define GB_IO_LYC           0x45
#define GB_IO_DMA           0x46
#define GB_IO_BGP           0x47
#define GB_IO_OBP0         0x48
#define GB_IO_OBP1         0x49
#define GB_IO_WY            0x4A
#define GB_IO_WX            0x4B
#define GB_IO_IF            0x0F

/* JOYP (0xFF00): bits de selección de grupo de teclas */
#define GB_JOYP_SEL_DIR  0x10
#define GB_JOYP_SEL_BTN  0x20
#define GB_JOYP_READ_MASK 0x0F

/* ----- Interrupt bits (IF/IE) ----- */
#define GB_IF_VBLANK  0x01
#define GB_IF_LCDC    0x02
#define GB_IF_TIMER   0x04
#define GB_IF_SERIAL  0x08
#define GB_IF_JOYPAD  0x10
/** Máscara de los 5 bits de interrupción en IF/IE (no usar bits reservados). */
#define GB_IF_IE_MASK  0x1F
/** Bits reservados de IF: leen 1 y no se modifican con escritura. */
#define GB_IF_RESERVED 0xE0

/* Interrupt vectors */
#define GB_IV_VBLANK  0x40
#define GB_IV_LCDC    0x48
#define GB_IV_TIMER   0x50
#define GB_IV_SERIAL  0x58
#define GB_IV_JOYPAD  0x60

/* ----- CPU / ALU ----- */
/** Punto de entrada tras reset (sin boot ROM). */
#define GB_CPU_ENTRY   0x0100
/** Estado típico del registro F tras boot (Z=1, N=1, H=1, C=0). */
#define GB_CPU_F_POSTBOOT 0xB0
/** Máscara del byte bajo (8 bits). */
#define GB_BYTE_LO      0xFF
/** Máscara de la palabra baja (16 bits). */
#define GB_WORD_LO      0xFFFF
/** Máscara de 12 bits para medio acarreo en ADD HL,rr (nibble alto de cada byte). */
#define GB_HALFCARRY_12 0xFFF
/** Base de la zona I/O (0xFF00); LDH n,A usa 0xFF00|n. */
#define GB_IO_BASE      0xFF00
/** Registro F: solo se usan los 4 bits altos (Z,N,H,C). */
#define GB_F_UPPER      0xF0
/** DAA: umbral BCD para ajuste (decimal 99). */
#define GB_BCD_HI       0x99
/** DAA: corrección para carry/medio acarreo (restar 0x60 / sumar 6). */
#define GB_DAA_CARRY    0x60
#define GB_DAA_NIBBLE   6
/** SP tras reset (apunta al tope de la pila, debajo de la zona I/O). */
#define GB_SP_INIT      0xFFFE

/* Joypad */
/** Estado "ninguna tecla": todas las líneas en 1 (released). */
#define GB_JOYP_RELEASED 0xFF
#define GB_JOYP_DIR_BIT     4
#define GB_JOYP_BTN_BIT     5
#define GB_JOYP_BIT_RIGHT   0
#define GB_JOYP_BIT_LEFT    1
#define GB_JOYP_BIT_UP      2
#define GB_JOYP_BIT_DOWN    3
#define GB_JOYP_BIT_A       0
#define GB_JOYP_BIT_B       1
#define GB_JOYP_BIT_SELECT  2
#define GB_JOYP_BIT_START   3

#endif /* BITEMU_GB_CONSTANTS_H */
