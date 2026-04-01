/**
 * BE2 - Sega Genesis: input (CLI teclado)
 * P1: WASD, J=A, K=B, Coma=C, I/Enter=Start (bits joypad_raw como memory.c).
 * P2: TFGH, O=A, Y=B, U=C, P=Start.
 *
 * Copyright (c) 2026 Iván Ezequiel Rodriguez
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "input.h"
#include <stddef.h>

#if defined(__linux__) || defined(__unix__)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static struct termios g_old_term;
static int g_raw_mode = 0;

static void set_raw_mode(int on)
{
    if (on && !g_raw_mode)
    {
        tcgetattr(STDIN_FILENO, &g_old_term);
        struct termios raw = g_old_term;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        g_raw_mode = 1;
    }
    else if (!on && g_raw_mode)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_term);
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
        g_raw_mode = 0;
    }
}

void gen_input_poll(genesis_impl_t *impl)
{
    if (!impl)
        return;
    set_raw_mode(1);

    /* joypad_raw: bit=1 presionado; orden R,L,D,U luego A,B,C,Start… (ver memory.c). */
    uint16_t p1 = 0, p2 = 0;
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1)
    {
        switch (c)
        {
        case 'w': case 'W': p1 |= 1u << 3; break; /* U */
        case 's': case 'S': p1 |= 1u << 1; break; /* D */
        case 'a': case 'A': p1 |= 1u << 2; break; /* L */
        case 'd': case 'D': p1 |= 1u << 0; break; /* R */
        case 'j': case 'J': p1 |= 1u << 4; break; /* A */
        case 'k': case 'K': p1 |= 1u << 5; break; /* B */
        case ',': p1 |= 1u << 6; break; /* C */
        case 'i': case 'I': case '\r': case '\n': p1 |= 1u << 7; break; /* Start */

        case 't': case 'T': p2 |= 1u << 3; break;
        case 'g': case 'G': p2 |= 1u << 1; break;
        case 'f': case 'F': p2 |= 1u << 2; break;
        case 'h': case 'H': p2 |= 1u << 0; break;
        case 'y': case 'Y': p2 |= 1u << 5; break;
        case 'u': case 'U': p2 |= 1u << 6; break;
        case 'o': case 'O': p2 |= 1u << 4; break;
        case 'p': case 'P': p2 |= 1u << 7; break;
        default: break;
        }
    }
    impl->joypad_state = p1 & 0x0FFF;
    impl->joypad2_state = p2 & 0x0FFF;
}

#else
void gen_input_poll(genesis_impl_t *impl)
{
    (void)impl;
}
#endif
