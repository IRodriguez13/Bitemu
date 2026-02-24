/**
 * Bitemu BE1 - Game Boy Input
 * Mapeo: D-pad=WASD, A=J, B=K, Select=U, Start=I (o Enter)
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

void gb_input_poll(gb_mem_t *mem)
{
    if (!mem)
        return;
    set_raw_mode(1);

    uint8_t state = 0xFF;
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1)
    {
        switch (c)
        {
        case 'w':
        case 'W':
            state &= ~(1 << GB_JOYP_BIT_UP);
            break;
        case 's':
        case 'S':
            state &= ~(1 << GB_JOYP_BIT_DOWN);
            break;
        case 'a':
        case 'A':
            state &= ~(1 << GB_JOYP_BIT_LEFT);
            break;
        case 'd':
        case 'D':
            state &= ~(1 << GB_JOYP_BIT_RIGHT);
            break;
        case 'j':
        case 'J':
            state &= ~(1 << (4 + GB_JOYP_BIT_A));
            break;
        case 'k':
        case 'K':
            state &= ~(1 << (4 + GB_JOYP_BIT_B));
            break;
        case 'u':
        case 'U':
            state &= ~(1 << (4 + GB_JOYP_BIT_SELECT));
            break;
        case 'i':
        case 'I':
        case '\r':
        case '\n':
            state &= ~(1 << (4 + GB_JOYP_BIT_START));
            break;
        default:
            break;
        }
    }
    mem->joypad_state = state;
}

#else
void gb_input_poll(gb_mem_t *mem)
{
    (void)mem;
}
#endif

void gb_input_set_state(gb_mem_t *mem, uint8_t state)
{
    if (mem)
        mem->joypad_state = state;
}
