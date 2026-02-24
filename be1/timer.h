/**
 * Bitemu - Game Boy Timer
 * DIV, TIMA, TAC, TMA
 */

#ifndef BITEMU_GB_TIMER_H
#define BITEMU_GB_TIMER_H

struct gb_mem;

void gb_timer_step(struct gb_mem *mem, int cycles);

#endif /* BITEMU_GB_TIMER_H */
