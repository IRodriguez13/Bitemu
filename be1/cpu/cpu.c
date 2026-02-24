/**
 * Bitemu - Game Boy CPU (LR35902 / SM83)
 * Fetch-decode-execute
 */

#include "cpu.h"
#include "../memory.h"
#include "../gb_constants.h"
#include <stddef.h>

#define HL ((uint16_t)((cpu->h << 8) | cpu->l))
#define BC ((uint16_t)((cpu->b << 8) | cpu->c))
#define DE ((uint16_t)((cpu->d << 8) | cpu->e))

#define R(r) (*((uint8_t *[]){&cpu->b, &cpu->c, &cpu->d, &cpu->e, &cpu->h, &cpu->l, NULL, &cpu->a})[r])

static uint8_t read_r8(gb_cpu_t *cpu, gb_mem_t *mem, int r)
{
    return (r == 6) ? gb_mem_read(mem, HL) : R(r);
}

static void write_r8(gb_cpu_t *cpu, gb_mem_t *mem, int r, uint8_t v)
{
    if (r == 6)
        gb_mem_write(mem, HL, v);
    else
        R(r) = v;
}

void gb_cpu_init(gb_cpu_t *cpu)
{
    cpu->pc = 0;
    cpu->sp = 0;
    cpu->a = cpu->b = cpu->c = cpu->d = cpu->e = cpu->h = cpu->l = 0;
    cpu->f = 0;
    cpu->halted = 0;
    cpu->ime = 0;
    cpu->ime_delay = 0;
}

void gb_cpu_reset(gb_cpu_t *cpu)
{
    cpu->pc = 0x0100; /* Entry point (sin boot ROM) */
    cpu->sp = 0xFFFE;
    cpu->a = 0x01;
    cpu->f = 0xB0; /* Post-boot state */
    cpu->b = 0x00;
    cpu->c = 0x13;
    cpu->d = 0x00;
    cpu->e = 0xD8;
    cpu->h = 0x01;
    cpu->l = 0x4D;
    cpu->halted = 0;
    cpu->ime = 0;
    cpu->ime_delay = 0;
}

static inline void set_z(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_Z) | (v ? F_Z : 0); }
static inline void set_n(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_N) | (v ? F_N : 0); }
static inline void set_h(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_H) | (v ? F_H : 0); }
static inline void set_c(gb_cpu_t *cpu, int v) { cpu->f = (cpu->f & ~F_C) | (v ? F_C : 0); }
static inline int flag_z(gb_cpu_t *cpu) { return (cpu->f & F_Z) != 0; }
static inline int flag_c(gb_cpu_t *cpu) { return (cpu->f & F_C) != 0; }

static int dispatch_interrupt(gb_cpu_t *cpu, gb_mem_t *m)
{
    uint8_t if_reg = m->io[GB_IO_IF] & 0x1F;
    uint8_t ie = m->ie & 0x1F;
    uint8_t pending = if_reg & ie;
    if (!pending)
        return 0;

    uint16_t vec;
    uint8_t mask;
    if (pending & GB_IF_VBLANK)
    {
        vec = GB_IV_VBLANK;
        mask = GB_IF_VBLANK;
    }
    else if (pending & GB_IF_LCDC)
    {
        vec = GB_IV_LCDC;
        mask = GB_IF_LCDC;
    }
    else if (pending & GB_IF_TIMER)
    {
        vec = GB_IV_TIMER;
        mask = GB_IF_TIMER;
    }
    else if (pending & GB_IF_SERIAL)
    {
        vec = GB_IV_SERIAL;
        mask = GB_IF_SERIAL;
    }
    else
    {
        vec = GB_IV_JOYPAD;
        mask = GB_IF_JOYPAD;
    }

    cpu->ime = 0;
    cpu->ime_delay = 0;
    m->io[GB_IO_IF] &= ~mask;
    cpu->sp -= 2;
    gb_mem_write16(m, cpu->sp, cpu->pc);
    cpu->pc = vec;
    return 20;
}

static void alu_add(gb_cpu_t *cpu, uint8_t a, uint8_t b, int carry)
{
    uint16_t r = a + b + carry;
    uint8_t res = (uint8_t)r;
    set_z(cpu, res == 0);
    set_n(cpu, 0);
    set_h(cpu, ((a & 0xF) + (b & 0xF) + carry) > 0xF);
    set_c(cpu, r > 0xFF);
    cpu->a = res;
}

static void alu_sub(gb_cpu_t *cpu, uint8_t a, uint8_t b, int carry)
{
    uint16_t r = a - b - carry;
    uint8_t res = (uint8_t)r;
    set_z(cpu, res == 0);
    set_n(cpu, 1);
    set_h(cpu, (int)(a & 0xF) - (int)(b & 0xF) - carry < 0);
    set_c(cpu, r > 0xFF);
    cpu->a = res;
}

static void alu_and(gb_cpu_t *cpu, uint8_t v)
{
    cpu->a &= v;
    set_z(cpu, cpu->a == 0);
    set_n(cpu, 0);
    set_h(cpu, 1);
    set_c(cpu, 0);
}

static void alu_xor(gb_cpu_t *cpu, uint8_t v)
{
    cpu->a ^= v;
    set_z(cpu, cpu->a == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    set_c(cpu, 0);
}

static void alu_or(gb_cpu_t *cpu, uint8_t v)
{
    cpu->a |= v;
    set_z(cpu, cpu->a == 0);
    set_n(cpu, 0);
    set_h(cpu, 0);
    set_c(cpu, 0);
}

static void alu_cp(gb_cpu_t *cpu, uint8_t a, uint8_t b)
{
    set_z(cpu, a == b);
    set_n(cpu, 1);
    set_h(cpu, (int)(a & 0xF) - (int)(b & 0xF) < 0);
    set_c(cpu, a < b);
}

int gb_cpu_step(gb_cpu_t *cpu, struct gb_mem *mem)
{
    gb_mem_t *m = (gb_mem_t *)mem;

    /* EI: IME se activa después de la siguiente instrucción */
    if (cpu->ime_delay)
    {
        cpu->ime_delay = 0;
        cpu->ime = 1;
    }

    /* Interrupt check: si IME y hay pending, despachar */
    if (cpu->ime)
    {
        int c = dispatch_interrupt(cpu, m);
        if (c)
            return c;
    }

    /* HALT: despertar si hay interrupt pendiente (IF&IE) */
    if (cpu->halted)
    {
        if ((m->io[GB_IO_IF] & m->ie & 0x1F))
            cpu->halted = 0;
        else
            return 4;
    }

    uint8_t op = gb_mem_read(m, cpu->pc++);
    int cycles = 4;

    if (op == 0xCB)
    {
        op = gb_mem_read(m, cpu->pc++);
        cycles = 8;
        int r = op & 7;
        uint8_t v = read_r8(cpu, m, r);

        switch (op >> 3)
        {

        case 0x00:
            v = (v << 1) | (v >> 7);
            set_c(cpu, v & 1);
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            write_r8(cpu, m, r, v);
            break; /* RLC */

        case 0x01:
        {
            int c = v & 1;
            v = (v >> 1) | (c << 7);
            set_c(cpu, c);
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            write_r8(cpu, m, r, v);
        }
        break; /* RRC */

        case 0x02:
        {
            int c = flag_c(cpu);
            set_c(cpu, v >> 7);
            v = (v << 1) | c;
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            write_r8(cpu, m, r, v);
        }
        break; /* RL */

        case 0x03:
        {
            int c = flag_c(cpu);
            set_c(cpu, v & 1);
            v = (v >> 1) | (c << 7);
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            write_r8(cpu, m, r, v);
        }
        break; /* RR */

        case 0x04:
            set_c(cpu, v >> 7);
            v <<= 1;
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            write_r8(cpu, m, r, v);
            break; /* SLA */

        case 0x05:
            set_c(cpu, v & 1);
            v = (int8_t)v >> 1;
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            write_r8(cpu, m, r, v);
            break; /* SRA */

        case 0x06:
            v = (v >> 4) | (v << 4);
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            set_c(cpu, 0);
            write_r8(cpu, m, r, v);
            break; /* SWAP */

        case 0x07:
            set_c(cpu, v & 1);
            v >>= 1;
            set_z(cpu, (v &= 0xFF) == 0);
            set_n(cpu, 0);
            set_h(cpu, 0);
            write_r8(cpu, m, r, v);
            break; /* SRL */

        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
            set_z(cpu, !(v & (1 << (op >> 3 & 7))));
            set_n(cpu, 0);
            set_h(cpu, 1);
            break; /* BIT */

        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:

            v &= ~(1 << (op >> 3 & 7));
            write_r8(cpu, m, r, v);
            break; /* RES */

        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:

            v |= (1 << (op >> 3 & 7));
            write_r8(cpu, m, r, v);
            break; /* SET */

        default:
            break;
        }
        if (r == 6)
            cycles = 16;
        return cycles;
    }

    switch (op)
    {
    case 0x00:
        break; /* NOP */
    case 0x01:
        cpu->c = gb_mem_read(m, cpu->pc++);
        cpu->b = gb_mem_read(m, cpu->pc++);
        cycles = 12;
        break;
    case 0x02:
        gb_mem_write(m, BC, cpu->a);
        break;
    case 0x03:
    {
        uint16_t bc = BC + 1;
        cpu->b = bc >> 8;
        cpu->c = bc & 0xFF;
    }
    break;
    case 0x04:
    {
        uint8_t b = cpu->b + 1;
        set_z(cpu, b == 0);
        set_n(cpu, 0);
        set_h(cpu, (cpu->b & 0xF) == 0xF);
        cpu->b = b;
    }
    break;
    case 0x05:
    {
        uint8_t b = cpu->b - 1;
        set_z(cpu, b == 0);
        set_n(cpu, 1);
        set_h(cpu, (cpu->b & 0xF) == 0);
        cpu->b = b;
    }
    break;
    case 0x06:
        cpu->b = gb_mem_read(m, cpu->pc++);
        break;
    case 0x07:
    {
        uint8_t a = cpu->a;
        set_c(cpu, a >> 7);
        cpu->a = (a << 1) | (a >> 7);
        set_z(cpu, 0);
        set_n(cpu, 0);
        set_h(cpu, 0);
    }
    break;
    case 0x08:
    {
        uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
        cpu->pc += 2;
        gb_mem_write16(m, a, cpu->sp);
        cycles = 20;
    }
    break;
    case 0x09:
    {
        uint16_t hl = (cpu->h << 8) | cpu->l, bc = BC;
        uint32_t r = hl + bc;
        set_n(cpu, 0);
        set_h(cpu, ((hl & 0xFFF) + (bc & 0xFFF)) > 0xFFF);
        set_c(cpu, r > 0xFFFF);
        cpu->h = (r >> 8) & 0xFF;
        cpu->l = r & 0xFF;
    }
    break;
    case 0x0A:
        cpu->a = gb_mem_read(m, BC);
        break;
    case 0x0B:
    {
        uint16_t bc = BC - 1;
        cpu->b = bc >> 8;
        cpu->c = bc & 0xFF;
    }
    break;
    case 0x0C:
    {
        uint8_t c = cpu->c + 1;
        set_z(cpu, c == 0);
        set_n(cpu, 0);
        set_h(cpu, (cpu->c & 0xF) == 0xF);
        cpu->c = c;
    }
    break;
    case 0x0D:
    {
        uint8_t c = cpu->c - 1;
        set_z(cpu, c == 0);
        set_n(cpu, 1);
        set_h(cpu, (cpu->c & 0xF) == 0);
        cpu->c = c;
    }
    break;
    case 0x0E:
        cpu->c = gb_mem_read(m, cpu->pc++);
        break;
    case 0x0F:
    {
        uint8_t a = cpu->a;
        set_c(cpu, a & 1);
        cpu->a = (a >> 1) | (a << 7);
        set_z(cpu, 0);
        set_n(cpu, 0);
        set_h(cpu, 0);
    }
    break;

    case 0x10:
        cpu->pc++;
        break; /* STOP */

    case 0x11:
        cpu->e = gb_mem_read(m, cpu->pc++);
        cpu->d = gb_mem_read(m, cpu->pc++);
        cycles = 12;
        break;

    case 0x12:
        gb_mem_write(m, DE, cpu->a);
        break;

    case 0x13:
    {
        uint16_t de = DE + 1;
        cpu->d = de >> 8;
        cpu->e = de & 0xFF;
    }
    break;

    case 0x14:
    {
        uint8_t d = cpu->d + 1;
        set_z(cpu, d == 0);
        set_n(cpu, 0);
        set_h(cpu, (cpu->d & 0xF) == 0xF);
        cpu->d = d;
    }
    break;

    case 0x15:
    {
        uint8_t d = cpu->d - 1;
        set_z(cpu, d == 0);
        set_n(cpu, 1);
        set_h(cpu, (cpu->d & 0xF) == 0);
        cpu->d = d;
    }
    break;

    case 0x16:
        cpu->d = gb_mem_read(m, cpu->pc++);
        break;

    case 0x17:
    {
        uint8_t a = cpu->a;
        int c = flag_c(cpu);
        set_c(cpu, a >> 7);
        cpu->a = (a << 1) | c;
        set_z(cpu, 0);
        set_n(cpu, 0);
        set_h(cpu, 0);
    }
    break;

    case 0x18:
    {
        int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
        cpu->pc += e;
        cycles = 12;
    }
    break;

    case 0x19:
    {
        uint16_t hl = HL, de = DE;
        uint32_t r = hl + de;
        set_n(cpu, 0);
        set_h(cpu, ((hl & 0xFFF) + (de & 0xFFF)) > 0xFFF);
        set_c(cpu, r > 0xFFFF);
        cpu->h = (r >> 8) & 0xFF;
        cpu->l = r & 0xFF;
    }
    break;

    case 0x1A:
        cpu->a = gb_mem_read(m, DE);
        break;

    case 0x1B:
    {
        uint16_t de = DE - 1;
        cpu->d = de >> 8;
        cpu->e = de & 0xFF;
    }
    break;

    case 0x1C:
    {
        uint8_t e = cpu->e + 1;
        set_z(cpu, e == 0);
        set_n(cpu, 0);
        set_h(cpu, (cpu->e & 0xF) == 0xF);
        cpu->e = e;
    }
    break;

    case 0x1D:
    {
        uint8_t e = cpu->e - 1;
        set_z(cpu, e == 0);
        set_n(cpu, 1);
        set_h(cpu, (cpu->e & 0xF) == 0);
        cpu->e = e;
    }
    break;

    case 0x1E:
        cpu->e = gb_mem_read(m, cpu->pc++);
        break;

    case 0x1F:
    {
        uint8_t a = cpu->a;
        int c = flag_c(cpu);
        set_c(cpu, a & 1);
        cpu->a = (a >> 1) | (c << 7);
        set_z(cpu, 0);
        set_n(cpu, 0);
        set_h(cpu, 0);
    }
    break;

    case 0x20:
    {
        int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
        if (!flag_z(cpu))
        {
            cpu->pc += e;
            cycles = 12;
        }
        else
            cycles = 8;
    }
    break;

    case 0x21:
        cpu->l = gb_mem_read(m, cpu->pc++);
        cpu->h = gb_mem_read(m, cpu->pc++);
        cycles = 12;
        break;

    case 0x22:
        gb_mem_write(m, HL, cpu->a);
        cpu->l++;
        if (cpu->l == 0)
            cpu->h++;
        break;

    case 0x23:
    {
        uint16_t hl = HL + 1;
        cpu->h = hl >> 8;
        cpu->l = hl & 0xFF;
    }
    break;

    case 0x24:
    {
        uint8_t h = cpu->h + 1;
        set_z(cpu, h == 0);
        set_n(cpu, 0);
        set_h(cpu, (cpu->h & 0xF) == 0xF);
        cpu->h = h;
    }
    break;

    case 0x25:
    {
        uint8_t h = cpu->h - 1;
        set_z(cpu, h == 0);
        set_n(cpu, 1);
        set_h(cpu, (cpu->h & 0xF) == 0);
        cpu->h = h;
    }
    break;

    case 0x26:
        cpu->h = gb_mem_read(m, cpu->pc++);
        break;

    case 0x27: /* DAA - simplified */
    {
        uint8_t a = cpu->a;
        int c = flag_c(cpu), h = (cpu->f & F_H) != 0, n = (cpu->f & F_N) != 0;
        if (!n)
        {
            if (c || a > 0x99)
            {
                a += 0x60;
                set_c(cpu, 1);
            }
            if (h || (a & 0xF) > 9)
                a += 6;
        }
        else
        {
            if (c)
                a -= 0x60;
            if (h)
                a -= 6;
        }
        cpu->a = a & 0xFF;
        set_z(cpu, cpu->a == 0);
        set_h(cpu, 0);
    }
    break;

    case 0x28:
    {
        int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
        if (flag_z(cpu))
        {
            cpu->pc += e;
            cycles = 12;
        }
        else
            cycles = 8;
    }
    break;

    case 0x29:
    {
        uint16_t hl = HL;
        uint32_t r = hl + hl;
        set_n(cpu, 0);
        set_h(cpu, ((hl & 0xFFF) * 2) > 0xFFF);
        set_c(cpu, r > 0xFFFF);
        cpu->h = (r >> 8) & 0xFF;
        cpu->l = r & 0xFF;
    }
    break;

    case 0x2A:
        cpu->a = gb_mem_read(m, HL);
        cpu->l++;
        if (cpu->l == 0)
            cpu->h++;
        break;

    case 0x2B:
    {
        uint16_t hl = HL - 1;
        cpu->h = hl >> 8;
        cpu->l = hl & 0xFF;
    }
    break;

    case 0x2C:
    {
        uint8_t l = cpu->l + 1;
        set_z(cpu, l == 0);
        set_n(cpu, 0);
        set_h(cpu, (cpu->l & 0xF) == 0xF);
        cpu->l = l;
    }
    break;

    case 0x2D:
    {
        uint8_t l = cpu->l - 1;
        set_z(cpu, l == 0);
        set_n(cpu, 1);
        set_h(cpu, (cpu->l & 0xF) == 0);
        cpu->l = l;
    }
    break;

    case 0x2E:
        cpu->l = gb_mem_read(m, cpu->pc++);
        break;

    case 0x2F:
        cpu->a = ~cpu->a;
        set_n(cpu, 1);
        set_h(cpu, 1);
        break;

    case 0x30:
    {
        int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
        if (!flag_c(cpu))
        {
            cpu->pc += e;
            cycles = 12;
        }
        else
            cycles = 8;
    }
    break;

    case 0x31:
        cpu->sp = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
        cpu->pc += 2;
        cycles = 12;
        break;

    case 0x32:
        gb_mem_write(m, HL, cpu->a);
        cpu->l--;
        if (cpu->l == 0xFF)
            cpu->h--;
        break;

    case 0x33:
        cpu->sp++;
        break;

    case 0x34:
    {
        uint8_t old = gb_mem_read(m, HL);
        uint8_t v = old + 1;
        set_z(cpu, v == 0);
        set_n(cpu, 0);
        set_h(cpu, (old & 0xF) == 0xF);
        gb_mem_write(m, HL, v);
        cycles = 12;
    }
    break;

    case 0x35:
    {
        uint8_t old = gb_mem_read(m, HL);
        uint8_t v = old - 1;
        set_z(cpu, v == 0);
        set_n(cpu, 1);
        set_h(cpu, (old & 0xF) == 0);
        gb_mem_write(m, HL, v);
        cycles = 12;
    }
    break;

    case 0x36:
        gb_mem_write(m, HL, gb_mem_read(m, cpu->pc++));
        cycles = 12;
        break;

    case 0x37:
        set_n(cpu, 0);
        set_h(cpu, 0);
        set_c(cpu, 1);
        break;

    case 0x38:
    {
        int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
        if (flag_c(cpu))
        {
            cpu->pc += e;
            cycles = 12;
        }
        else
            cycles = 8;
    }
    break;

    case 0x39:
    {
        uint16_t hl = HL;
        uint32_t r = hl + cpu->sp;
        set_n(cpu, 0);
        set_h(cpu, ((hl & 0xFFF) + (cpu->sp & 0xFFF)) > 0xFFF);
        set_c(cpu, r > 0xFFFF);
        cpu->h = (r >> 8) & 0xFF;
        cpu->l = r & 0xFF;
    }
    break;

    case 0x3A:
        cpu->a = gb_mem_read(m, HL);
        cpu->l--;
        if (cpu->l == 0xFF)
            cpu->h--;
        break;

    case 0x3B:
        cpu->sp--;
        break;

    case 0x3C:
    {
        uint8_t a = cpu->a + 1;
        set_z(cpu, a == 0);
        set_n(cpu, 0);
        set_h(cpu, (cpu->a & 0xF) == 0xF);
        cpu->a = a;
    }
    break;

    case 0x3D:
    {
        uint8_t a = cpu->a - 1;
        set_z(cpu, a == 0);
        set_n(cpu, 1);
        set_h(cpu, (cpu->a & 0xF) == 0);
        cpu->a = a;
    }
    break;

    case 0x3E:
        cpu->a = gb_mem_read(m, cpu->pc++);
        break;

    case 0x3F:
        set_n(cpu, 0);
        set_h(cpu, 0);
        set_c(cpu, !flag_c(cpu));
        break;

    default:
        if (op >= 0x40 && op <= 0x7F)
        {
            int dst = (op >> 3) & 7, src = op & 7;
            if (op == 0x76)
            {
                /* HALT bug: IME=0 y (IF&IE)!=0 -> no halt */
                if (!cpu->ime && (m->io[GB_IO_IF] & m->ie & 0x1F))
                    ; /* skip halt */
                else
                    cpu->halted = 1;
                break;
            } /* HALT */
            write_r8(cpu, m, dst, read_r8(cpu, m, src));
            if (dst == 6 || src == 6)
                cycles = 8;
            break;
        }
        if (op >= 0x80 && op <= 0xBF)
        {
            uint8_t v = read_r8(cpu, m, op & 7);
            if ((op & 7) == 6)
                cycles = 8;
            switch ((op >> 3) & 7)
            {
            case 0:
                alu_add(cpu, cpu->a, v, 0);
                break;
            case 1:
                alu_add(cpu, cpu->a, v, flag_c(cpu));
                break;
            case 2:
                alu_sub(cpu, cpu->a, v, 0);
                break;
            case 3:
                alu_sub(cpu, cpu->a, v, flag_c(cpu));
                break;
            case 4:
                alu_and(cpu, v);
                break;
            case 5:
                alu_xor(cpu, v);
                break;
            case 6:
                alu_or(cpu, v);
                break;
            case 7:
                alu_cp(cpu, cpu->a, v);
                break;
            }
            break;
        }
        if (op >= 0xC0)
        {
            switch (op)
            {
            case 0xC0:
                if (!flag_z(cpu))
                {
                    cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
                    cpu->sp += 2;
                    cycles = 20;
                }
                else
                    cycles = 8;
                break;
            case 0xC1:
                cpu->c = gb_mem_read(m, cpu->sp++);
                cpu->b = gb_mem_read(m, cpu->sp++);
                cycles = 12;
                break;
            case 0xC2:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (!flag_z(cpu))
                {
                    cpu->pc = a;
                    cycles = 16;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xC3:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc = a;
                cycles = 16;
            }
            break;
            case 0xC4:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (!flag_z(cpu))
                {
                    cpu->sp -= 2;
                    gb_mem_write16(m, cpu->sp, cpu->pc);
                    cpu->pc = a;
                    cycles = 24;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xC5:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, BC);
                cycles = 16;
                break;
            case 0xC6:
                alu_add(cpu, cpu->a, gb_mem_read(m, cpu->pc++), 0);
                cycles = 8;
                break;
            case 0xC7:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x00;
                cycles = 16;
                break;
            case 0xC8:
                if (flag_z(cpu))
                {
                    cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
                    cpu->sp += 2;
                    cycles = 20;
                }
                else
                    cycles = 8;
                break;
            case 0xC9:
                cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
                cpu->sp += 2;
                cycles = 16;
                break;
            case 0xCA:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (flag_z(cpu))
                {
                    cpu->pc = a;
                    cycles = 16;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xCB:
                break; /* handled above */
            case 0xCC:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (flag_z(cpu))
                {
                    cpu->sp -= 2;
                    gb_mem_write16(m, cpu->sp, cpu->pc);
                    cpu->pc = a;
                    cycles = 24;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xCD:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = a;
                cycles = 24;
            }
            break;
            case 0xCE:
                alu_add(cpu, cpu->a, gb_mem_read(m, cpu->pc++), flag_c(cpu));
                cycles = 8;
                break;
            case 0xCF:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x08;
                cycles = 16;
                break;

            case 0xD0:
                if (!flag_c(cpu))
                {
                    cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
                    cpu->sp += 2;
                    cycles = 20;
                }
                else
                    cycles = 8;
                break;
            case 0xD1:
                cpu->e = gb_mem_read(m, cpu->sp++);
                cpu->d = gb_mem_read(m, cpu->sp++);
                cycles = 12;
                break;
            case 0xD2:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (!flag_c(cpu))
                {
                    cpu->pc = a;
                    cycles = 16;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xD4:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (!flag_c(cpu))
                {
                    cpu->sp -= 2;
                    gb_mem_write16(m, cpu->sp, cpu->pc);
                    cpu->pc = a;
                    cycles = 24;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xD5:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, DE);
                cycles = 16;
                break;
            case 0xD6:
                alu_sub(cpu, cpu->a, gb_mem_read(m, cpu->pc++), 0);
                cycles = 8;
                break;
            case 0xD7:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x10;
                cycles = 16;
                break;
            case 0xD8:
                if (flag_c(cpu))
                {
                    cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
                    cpu->sp += 2;
                    cycles = 20;
                }
                else
                    cycles = 8;
                break;
            case 0xD9:
                cpu->pc = gb_mem_read(m, cpu->sp) | (gb_mem_read(m, cpu->sp + 1) << 8);
                cpu->sp += 2;
                cpu->ime = 1;
                cycles = 16;
                break; /* RETI */
            case 0xDA:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (flag_c(cpu))
                {
                    cpu->pc = a;
                    cycles = 16;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xDC:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                if (flag_c(cpu))
                {
                    cpu->sp -= 2;
                    gb_mem_write16(m, cpu->sp, cpu->pc);
                    cpu->pc = a;
                    cycles = 24;
                }
                else
                    cycles = 12;
            }
            break;
            case 0xDE:
                alu_sub(cpu, cpu->a, gb_mem_read(m, cpu->pc++), flag_c(cpu));
                cycles = 8;
                break;
            case 0xDF:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x18;
                cycles = 16;
                break;

            case 0xE0:
                gb_mem_write(m, 0xFF00 | gb_mem_read(m, cpu->pc++), cpu->a);
                cycles = 12;
                break;
            case 0xE1:
                cpu->l = gb_mem_read(m, cpu->sp++);
                cpu->h = gb_mem_read(m, cpu->sp++);
                cycles = 12;
                break;
            case 0xE2:
                gb_mem_write(m, 0xFF00 | cpu->c, cpu->a);
                break;
            case 0xE5:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, HL);
                cycles = 16;
                break;
            case 0xE6:
                alu_and(cpu, gb_mem_read(m, cpu->pc++));
                cycles = 8;
                break;
            case 0xE7:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x20;
                cycles = 16;
                break;
            case 0xE8:
            {
                int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
                uint32_t r = cpu->sp + e;
                set_z(cpu, 0);
                set_n(cpu, 0);
                set_h(cpu, ((cpu->sp & 0xF) + (e & 0xF)) > 0xF);
                set_c(cpu, ((cpu->sp & 0xFF) + (e & 0xFF)) > 0xFF);
                cpu->sp = (uint16_t)r;
                cycles = 16;
            }
            break;
            case 0xE9:
                cpu->pc = HL;
                cycles = 4;
                break;
            case 0xEA:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                gb_mem_write(m, a, cpu->a);
                cycles = 16;
            }
            break;
            case 0xEE:
                alu_xor(cpu, gb_mem_read(m, cpu->pc++));
                cycles = 8;
                break;
            case 0xEF:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x28;
                cycles = 16;
                break;

            case 0xF0:
                cpu->a = gb_mem_read(m, 0xFF00 | gb_mem_read(m, cpu->pc++));
                cycles = 12;
                break;
            case 0xF1:
            {
                uint8_t f = gb_mem_read(m, cpu->sp++) & 0xF0;
                cpu->a = gb_mem_read(m, cpu->sp++);
                cpu->f = f;
            }
                cycles = 12;
                break;
            case 0xF2:
                cpu->a = gb_mem_read(m, 0xFF00 | cpu->c);
                break;
            case 0xF3:
                cpu->ime = 0;
                cpu->ime_delay = 0;
                break; /* DI */
            case 0xF5:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, (cpu->a << 8) | (cpu->f & 0xF0));
                cycles = 16;
                break;
            case 0xF6:
                alu_or(cpu, gb_mem_read(m, cpu->pc++));
                cycles = 8;
                break;
            case 0xF7:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x30;
                cycles = 16;
                break;
            case 0xF8:
            {
                int8_t e = (int8_t)gb_mem_read(m, cpu->pc++);
                uint32_t r = cpu->sp + e;
                set_z(cpu, 0);
                set_n(cpu, 0);
                set_h(cpu, ((cpu->sp & 0xF) + (e & 0xF)) > 0xF);
                set_c(cpu, ((cpu->sp & 0xFF) + (e & 0xFF)) > 0xFF);
                cpu->h = (r >> 8) & 0xFF;
                cpu->l = r & 0xFF;
                cycles = 12;
            }
            break;
            case 0xF9:
                cpu->sp = HL;
                cycles = 8;
                break;
            case 0xFA:
            {
                uint16_t a = gb_mem_read(m, cpu->pc) | (gb_mem_read(m, cpu->pc + 1) << 8);
                cpu->pc += 2;
                cpu->a = gb_mem_read(m, a);
                cycles = 16;
            }
            break;
            case 0xFB:
                cpu->ime_delay = 1;
                break; /* EI: IME after next instr */
            case 0xFE:
                alu_cp(cpu, cpu->a, gb_mem_read(m, cpu->pc++));
                cycles = 8;
                break;
            case 0xFF:
                cpu->sp -= 2;
                gb_mem_write16(m, cpu->sp, cpu->pc);
                cpu->pc = 0x38;
                cycles = 16;
                break;

            default:
                break;
            }
            break;
        }
        break;
    }

    return cycles;
}
