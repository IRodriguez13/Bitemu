/**
 * Simple Genesis Test ROM
 * Tests basic VDP, YM2612, and memory functionality
 */

#include <stdint.h>

/* Genesis memory map */
#define GEN_VDP_DATA      0xC00000
#define GEN_VDP_CTRL      0xC00004
#define GEN_VDP_HV        0xC00008
#define GEN_YM2612_A0     0xA04000
#define GEN_YM2612_A1     0xA04002
#define GEN_RAM_START      0xFF0000

/* VDP Registers */
#define VDP_REG_MODE1    0x81
#define VDP_REG_MODE2    0x81
#define VDP_REG_MODE4    0x81

/* Test patterns */
volatile uint32_t *test_output = (volatile uint32_t*)0xFF0000;

static inline void write_vdp_data(uint16_t data) {
    *((volatile uint16_t*)GEN_VDP_DATA) = data;
}

static inline void write_vdp_ctrl(uint16_t data) {
    *((volatile uint16_t*)GEN_VDP_CTRL) = data;
}

static inline uint16_t read_vdp_status(void) {
    return *((volatile uint16_t*)GEN_VDP_CTRL);
}

static inline uint16_t read_vdp_hv(void) {
    return *((volatile uint16_t*)GEN_VDP_HV);
}

static inline void write_ym2612(uint16_t addr, uint8_t data) {
    *((volatile uint8_t*)addr) = data;
}

static inline uint8_t read_ym2612_status(void) {
    return *((volatile uint8_t*)GEN_YM2612_A0);
}

void test_vdp_registers(void) {
    test_output[0] = 0xDEADBEEF; /* Test start marker */
    
    /* Test VDP register access */
    write_vdp_ctrl(0x8000 | VDP_REG_MODE1);
    write_vdp_data(0x00); /* Disable display */
    
    test_output[1] = 0x12345678; /* VDP test complete */
}

void test_ym2612_busy(void) {
    /* Test YM2612 busy flag */
    write_ym2612(GEN_YM2612_A0, 0x2A); /* Address */
    write_ym2612(GEN_YM2612_A1, 0x00); /* Data */
    
    uint8_t status = read_ym2612_status();
    test_output[2] = status; /* Should have busy flag set */
}

void test_hv_counters(void) {
    /* Read HV counters multiple times to verify they change */
    uint16_t hv1 = read_vdp_hv();
    for (volatile int i = 0; i < 1000; i++); /* Small delay */
    uint16_t hv2 = read_vdp_hv();
    
    test_output[3] = hv1;
    test_output[4] = hv2;
    test_output[5] = (hv1 != hv2) ? 0xAAAAAAAA : 0x55555555; /* Should be different */
}

void test_memory_access(void) {
    /* Test basic memory access */
    volatile uint32_t *ram = (volatile uint32_t*)GEN_RAM_START;
    
    ram[0] = 0xCAFEBABE;
    ram[1] = 0xDEADBEEF;
    
    test_output[6] = ram[0];
    test_output[7] = ram[1];
    test_output[8] = (ram[0] == 0xCAFEBABE) ? 0x11111111 : 0x22222222;
    test_output[9] = (ram[1] == 0xDEADBEEF) ? 0x33333333 : 0x44444444;
}

void main(void) {
    /* Initialize test output area */
    for (int i = 0; i < 16; i++) {
        test_output[i] = 0x00000000;
    }
    
    /* Run tests */
    test_vdp_registers();
    test_ym2612_busy();
    test_hv_counters();
    test_memory_access();
    
    /* Final marker */
    test_output[15] = 0xCAFEBABE;
    
    /* Infinite loop */
    while (1) {
        /* Spin */
    }
}
