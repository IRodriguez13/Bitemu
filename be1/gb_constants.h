/**
 * Bitemu BE1 - Game Boy constantes
 * Sin magic numbers.
 */

#ifndef BITEMU_GB_CONSTANTS_H
#define BITEMU_GB_CONSTANTS_H

/* Timing (dots = T-cycles @ 4.194304 MHz) */
#define GB_CPU_HZ            4194304
#define GB_DOTS_PER_SCANLINE 456
#define GB_SCANLINES_VISIBLE 144
#define GB_SCANLINES_VBLANK  10
#define GB_SCANLINES_TOTAL   154
#define GB_DOTS_PER_FRAME   (GB_DOTS_PER_SCANLINE * GB_SCANLINES_TOTAL)  /* 70224 */
#define GB_FPS               59.7275

/* PPU modes */
#define GB_PPU_MODE_HBLANK  0
#define GB_PPU_MODE_VBLANK  1
#define GB_PPU_MODE_OAM     2
#define GB_PPU_MODE_TRANSFER 3

/* PPU mode durations (dots) */
#define GB_PPU_MODE2_DOTS   80
#define GB_PPU_MODE3_MIN    172
#define GB_PPU_MODE3_MAX    289
#define GB_PPU_VBLANK_DOTS  (GB_DOTS_PER_SCANLINE * GB_SCANLINES_VBLANK)  /* 4560 */

/* Display */
#define GB_LCD_WIDTH        160
#define GB_LCD_HEIGHT       144
#define GB_TILE_SIZE        8
#define GB_TILES_PER_ROW    20
#define GB_TILES_PER_COL    18

/* IO offsets (from 0xFF00) */
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

/* Interrupt bits (IF/IE) */
#define GB_IF_VBLANK  0x01
#define GB_IF_LCDC    0x02
#define GB_IF_TIMER   0x04
#define GB_IF_SERIAL  0x08
#define GB_IF_JOYPAD  0x10

/* Interrupt vectors */
#define GB_IV_VBLANK  0x40
#define GB_IV_LCDC    0x48
#define GB_IV_TIMER   0x50
#define GB_IV_SERIAL  0x58
#define GB_IV_JOYPAD  0x60

/* Joypad */
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
