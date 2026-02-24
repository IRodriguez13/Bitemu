# CPU (LR35902 / SM83)

**Guía del código C:** para un recorrido detallado de cómo está implementada la CPU en `be1/cpu.c` (flujo, macros, ALU, switch de opcodes, CB, interrupciones), ver **[01-cpu-implementation.md](01-cpu-implementation.md)**.

---

## English

### 5.1 Overview

The Game Boy uses the Sharp LR35902 (also known as SM83), a custom CPU derived from the Z80 and Intel 8080. It is an 8-bit CPU with 16-bit address bus, running at 4.194304 MHz.

### 5.2 Registers

| Register | Size | Purpose |
|----------|------|---------|
| A | 8-bit | Accumulator |
| F | 8-bit | Flags: Z(7), N(6), H(5), C(4). Lower 4 bits always 0. |
| B, C, D, E, H, L | 8-bit | General purpose. Can pair as BC, DE, HL (16-bit). |
| PC | 16-bit | Program counter |
| SP | 16-bit | Stack pointer |

### 5.3 Flags

- **Z (Zero)**: Result is zero
- **N (Subtract)**: Last operation was subtraction
- **H (Half-carry)**: Carry from bit 3 to 4
- **C (Carry)**: Carry from bit 7 (or bit 15 for 16-bit)

### 5.4 Instruction Set

- ~500 opcodes including CB-prefixed (bit operations, shifts).
- Variable length: 1–3 bytes.
- Variable cycles: 4–20 T-cycles per instruction.
- `gb_cpu_step()` fetches one opcode, executes, returns cycles consumed.

### 5.5 Interrupts

- **IME (Interrupt Master Enable)**: When 1, pending interrupts are serviced.
- **EI**: Enables IME **after the next instruction** (1-instruction delay).
- **DI**: Disables IME immediately.
- **HALT**: Stops execution until an interrupt occurs. Wakes when `(IF & IE) != 0`.
- **HALT bug**: If HALT is executed with IME=0 and `(IF & IE) != 0`, the CPU does not halt (DMG hardware bug).

### 5.6 Interrupt Vectors

| Source | Vector | IF bit | IE bit |
|--------|--------|--------|--------|
| VBlank | 0x40 | 0 | 0 |
| LCD STAT | 0x48 | 1 | 1 |
| Timer | 0x50 | 2 | 2 |
| Serial | 0x58 | 3 | 3 |
| Joypad | 0x60 | 4 | 4 |

### 5.7 Dispatch Logic

1. At start of each instruction: if `ime_delay`, set IME=1.
2. If IME and `(IF & IE) != 0`: push PC, jump to vector, clear IME, clear IF bit. Return 20 cycles.
3. If halted and `(IF & IE) != 0`: unhalt.
4. If halted: return 4 cycles (no instruction executed).
5. Otherwise: fetch, decode, execute; return cycles.

### 5.8 Key Implementation Details

- `ime_delay`: Set by EI; cleared when IME is enabled (after next instruction) or when interrupt is dispatched.
- RETI: Sets IME=1 (handled in opcode 0xD9).
- All interrupt handling uses priority: VBlank > LCD > Timer > Serial > Joypad.

---

## Español

**Guía del código C:** para entender al detalle la implementación en `be1/cpu.c`, ver **[01-cpu-implementation.md](01-cpu-implementation.md)**.

### 5.1 Resumen

El Game Boy usa el Sharp LR35902 (SM83), una CPU derivada del Z80 e Intel 8080. Es una CPU de 8 bits con bus de direcciones de 16 bits, a 4.194304 MHz.

### 5.2 Registros

| Registro | Tamaño | Propósito |
|----------|--------|-----------|
| A | 8 bits | Acumulador |
| F | 8 bits | Flags: Z(7), N(6), H(5), C(4). Bits bajos siempre 0. |
| B, C, D, E, H, L | 8 bits | Propósito general. Se pueden emparejar como BC, DE, HL. |
| PC | 16 bits | Contador de programa |
| SP | 16 bits | Puntero de pila |

### 5.3 Flags

- **Z (Zero)**: Resultado cero
- **N (Subtract)**: Última operación fue resta
- **H (Half-carry)**: Acarreo del bit 3 al 4
- **C (Carry)**: Acarreo del bit 7 (o 15 en 16 bits)

### 5.4 Conjunto de instrucciones

- ~500 opcodes incluyendo prefijo CB (operaciones de bits, desplazamientos).
- Longitud variable: 1–3 bytes.
- Ciclos variables: 4–20 T-ciclos por instrucción.
- `gb_cpu_step()` obtiene un opcode, ejecuta, devuelve ciclos consumidos.

### 5.5 Interrupciones

- **IME**: Cuando 1, se atienden interrupciones pendientes.
- **EI**: Activa IME **después de la siguiente instrucción** (retardo de 1 instrucción).
- **DI**: Desactiva IME de inmediato.
- **HALT**: Para hasta que ocurra una interrupción. Despierta cuando `(IF & IE) != 0`.
- **Bug HALT**: Si HALT se ejecuta con IME=0 y `(IF & IE) != 0`, la CPU no se detiene (bug DMG).

### 5.6 Vectores de interrupción

| Fuente | Vector | Bit IF | Bit IE |
|--------|--------|--------|--------|
| VBlank | 0x40 | 0 | 0 |
| LCD STAT | 0x48 | 1 | 1 |
| Timer | 0x50 | 2 | 2 |
| Serial | 0x58 | 3 | 3 |
| Joypad | 0x60 | 4 | 4 |

### 5.7 Lógica de despacho

1. Al inicio de cada instrucción: si `ime_delay`, poner IME=1.
2. Si IME y `(IF & IE) != 0`: push PC, saltar al vector, limpiar IME y bit IF. Devolver 20 ciclos.
3. Si halted y `(IF & IE) != 0`: despertar.
4. Si halted: devolver 4 ciclos (sin ejecutar instrucción).
5. Si no: fetch, decode, execute; devolver ciclos.
