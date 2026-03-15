/**
 * BE2 - Sega Genesis: input (CLI teclado)
 * Mismo mapeo que GB: WASD, J=A, K=B, I=Start
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

    uint8_t state = 0xFF;  /* 0 = pressed */
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1)
    {
        switch (c)
        {
        case 'w': case 'W': state &= ~(1 << 2); break;
        case 's': case 'S': state &= ~(1 << 3); break;
        case 'a': case 'A': state &= ~(1 << 1); break;
        case 'd': case 'D': state &= ~(1 << 0); break;
        case 'j': case 'J': state &= ~(1 << 4); break;
        case 'k': case 'K': state &= ~(1 << 5); break;
        case 'i': case 'I': case '\r': case '\n': state &= ~(1 << 7); break;
        default: break;
        }
    }
    impl->joypad_state = (impl->joypad_state & 0xFF00) | (uint8_t)(~state & 0xFF);
}

#else
void gen_input_poll(genesis_impl_t *impl)
{
    (void)impl;
}
#endif
