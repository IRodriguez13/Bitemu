/**
 * Bitemu - Timing
 * Control de sincronización y ciclos
 */

#ifndef BITEMU_TIMING_H
#define BITEMU_TIMING_H

#include <stdint.h>

typedef struct timing timing_t;

void timing_init(timing_t *timing);
uint64_t timing_get_cycles(timing_t *timing);

#endif /* BITEMU_TIMING_H */
