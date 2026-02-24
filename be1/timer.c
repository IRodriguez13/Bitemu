/**
 * Bitemu - Game Boy Timer
 * DIV: 16-bit internal, upper byte at 0xFF04, 16384 Hz
 * TIMA: incrementa cuando TAC habilitado, overflow -> IF bit 2, reload TMA
 * Escribir en DIV resetea el contador interno
 */

#include "timer.h"
#include "memory.h"
#include "gb_constants.h"

/* Divisores TAC para TIMA (ciclos entre incrementos) */
static const int gb_tima_div[] = {1024, 16, 64, 256};

void gb_timer_step(struct gb_mem *mem, int cycles)
{
    uint16_t div_old = mem->timer_div;
    mem->timer_div += cycles;

    /* DIV = bits 15-8 del contador interno */
    mem->io[GB_IO_DIV] = (uint8_t)(mem->timer_div >> 8);

    /* TIMA: solo si TAC bit 2 está activo */
    if (!(mem->io[GB_IO_TAC] & 0x04))
        return;

    int div = gb_tima_div[mem->io[GB_IO_TAC] & 3];
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
