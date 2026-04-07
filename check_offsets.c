/**
 * Verificador de offsets de estructuras
 */

#include <stdio.h>
#include <stddef.h>
#include "be2/vdp/vdp.h"
#include "be2/ym2612/ym2612.h"

int main(void) {
    printf("=== VDP Structure Offsets ===\n");
    printf("irq_vint_pending: %zu (expected: 280864)\n", offsetof(gen_vdp_t, irq_vint_pending));
    printf("irq_hint_pending: %zu (expected: 280865)\n", offsetof(gen_vdp_t, irq_hint_pending));
    printf("data_read_latch: %zu\n", offsetof(gen_vdp_t, data_read_latch));
    printf("data_read_latch_valid: %zu\n", offsetof(gen_vdp_t, data_read_latch_valid));
    printf("fifo_word_backlog: %zu\n", offsetof(gen_vdp_t, fifo_word_backlog));
    printf("fifo_drain_acc: %zu\n", offsetof(gen_vdp_t, fifo_drain_acc));
    
    printf("\n=== YM2612 Structure Offsets ===\n");
    printf("timer_a_counter: %zu (expected: 736)\n", offsetof(gen_ym2612_t, timer_a_counter));
    printf("timer_b_counter: %zu\n", offsetof(gen_ym2612_t, timer_b_counter));
    printf("timer_tick_acc: %zu\n", offsetof(gen_ym2612_t, timer_tick_acc));
    printf("timer_overflow: %zu\n", offsetof(gen_ym2612_t, timer_overflow));
    printf("busy_cycles: %zu\n", offsetof(gen_ym2612_t, busy_cycles));
    printf("last_write_port: %zu\n", offsetof(gen_ym2612_t, last_write_port));
    printf("write_timestamp: %zu\n", offsetof(gen_ym2612_t, write_timestamp));
    
    return 0;
}
