/**
 * Bitemu - Game Boy Timer
 *
 * DIV: contador interno de 16 bits; el byte alto se expone en 0xFF04 (16384 Hz).
 * TIMA: se incrementa cuando TAC bit 2 está activo; overflow -> IF bit 2 y recarga desde TMA.
 * Escribir en DIV resetea el contador interno (y por tanto DIV visible).
 */

#include "timer.h"
#include "memory.h"
#include "gb_constants.h"

/* Divisores para TIMA según TAC bits 0-1: 1024, 16, 64, 256 ciclos entre pasos */
static const int gb_tima_div[] = {1024, 16, 64, 256};

void gb_timer_step(struct gb_mem *mem, int cycles)
{
    uint16_t div_old = mem->timer_div;
    mem->timer_div += cycles;

    mem->io[GB_IO_DIV] = (uint8_t)(mem->timer_div >> 8);

    if (!(mem->io[GB_IO_TAC] & GB_TAC_ENABLE))
        return;

    int div = gb_tima_div[mem->io[GB_IO_TAC] & GB_TAC_RATE];
    int old_phase = div_old / div;
    int new_phase = mem->timer_div / div;

    while (old_phase < new_phase)
    {
        uint8_t tima = mem->io[GB_IO_TIMA] + 1;
        if (tima == 0)
        {
            mem->io[GB_IO_TIMA] = mem->io[GB_IO_TMA];
            mem->io[GB_IO_IF] |= GB_IF_TIMER;
        }
        else
        {
            mem->io[GB_IO_TIMA] = tima;
        }
        old_phase++;
    }
}
