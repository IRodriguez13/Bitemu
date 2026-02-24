# CPU (LR35902) – Guía del código en `be1/cpu.c`

Este documento recorre **cómo está implementada** la emulación de la CPU del Game Boy en C: estructura del archivo, flujo de ejecución y dónde está cada cosa.

---

## 1. Idea general

La CPU del Game Boy (Sharp LR35902 / SM83) se emula en **alto nivel** en C:

- **Un paso de CPU** = una llamada a `gb_cpu_step(cpu, mem)`.
- En cada paso: se hace **fetch** de un opcode, **decode** (switch por valor del opcode) y **execute** (cambiar registros, memoria, flags).
- La función devuelve los **ciclos consumidos** (T-states) para que el resto del sistema (timer, PPU, etc.) avance el tiempo.

No hay microcódigo ni pipeline real: es un **interpretador** de instrucciones.

---

## 2. Estructura del archivo

### 2.1 Cabecera y macros (aprox. líneas 1–28)

- **`HL`, `BC`, `DE`**: macros que forman el par de registros de 16 bits a partir de los dos registros de 8 bits (ej. `HL = (H<<8)|L`).
- **`R(r)`**: array “fantasma” que mapea índice `r` (0–7) al registro correcto:
  - 0→B, 1→C, 2→D, 3→E, 4→H, 5→L, 6→*(HL) en memoria, 7→A.  
  - El caso `r==6` es “dirección (HL)”; el resto son registros.

### 2.2 Lectura/escritura de “registro” (líneas 18–28)

- **`read_r8(cpu, mem, r)`**: lee el “registro” `r`. Si `r==6`, lee de la dirección `HL` en memoria; si no, usa `R(r)`.
- **`write_r8(cpu, mem, r, v)`**: escribe `v` en el “registro” `r`. Si `r==6`, escribe en la dirección `HL`; si no, en el registro correspondiente.

Así, todas las instrucciones que usan “registro” (incluidas las que operan sobre `(HL)`) pasan por estas funciones y el switch solo usa el índice 0–7.

### 2.3 Inicialización y reset (líneas 30–56)

- **`gb_cpu_init()`**: pone todo a 0 (PC, SP, registros, flags, halted, IME, ime_delay).
- **`gb_cpu_reset()`**: estado “post‑boot” típico:
  - PC = 0x0100 (punto de entrada sin boot ROM),
  - SP = 0xFFFE,
  - A=0x01, F=0xB0, B=0, C=0x13, D=0, E=0xD8, H=0x01, L=0x4D,
  - IME/halted a 0.

### 2.4 Flags (líneas 58–63)

- **`set_z`, `set_n`, `set_h`, `set_c`**: ponen o quitan el bit del flag en `cpu->f` (F_Z=0x80, F_N=0x40, F_H=0x20, F_C=0x10).
- **`flag_z`, `flag_c`**: leen si el flag está puesto (para saltos condicionales y carry en ALU).

### 2.5 Interrupciones (líneas 65–108)

- **`dispatch_interrupt(cpu, m)`**:
  - Lee IF e IE, calcula `pending = IF & IE`.
  - Elige **un** vector por prioridad: VBlank → LCD STAT → Timer → Serial → Joypad.
  - Apaga IME y `ime_delay`, limpia el bit correspondiente en IF, hace **push** de PC (16 bits) en la pila, pone PC = vector.
  - Devuelve **20 ciclos**.

### 2.6 ALU (líneas 110–165)

Funciones que actualizan A y flags (y a veces solo flags):

- **`alu_add(cpu, a, b, carry)`**: A = a + b + carry; flags Z, N=0, H (medio carry), C.
- **`alu_sub(cpu, a, b, carry)`**: A = a - b - carry; flags Z, N=1, H, C.
- **`alu_and`, `alu_xor`, `alu_or`**: operación bit a bit con A; Z, N=0; H=1 solo en AND.
- **`alu_cp(cpu, a, b)`**: compara (resta sin guardar resultado); solo actualiza Z, N, H, C.

---

## 3. Flujo de `gb_cpu_step` (líneas 167–~1300)

Cada llamada a `gb_cpu_step` hace, **en este orden**:

### 3.1 EI (retardo de una instrucción)

```c
if (cpu->ime_delay) {
    cpu->ime_delay = 0;
    cpu->ime = 1;
}
```

La instrucción **EI** (0xFB) solo pone `ime_delay = 1`. IME se activa **aquí**, al inicio del **siguiente** paso.

### 3.2 Comprobar interrupción

Si `cpu->ime` está activo, se llama a `dispatch_interrupt()`. Si hay una interrupción pendiente, se despacha y la función **retorna 20 ciclos** sin ejecutar ninguna instrucción más.

### 3.3 HALT

Si `cpu->halted`:

- Si hay algo pendiente en `IF & IE`, se “despierta” (`halted = 0`) y se sigue.
- Si no, se devuelven **4 ciclos** y no se ejecuta ninguna instrucción (la CPU “duerme” un ciclo de reloj).

### 3.4 Fetch y prefijo CB

- Se lee el opcode: `op = gb_mem_read(m, cpu->pc++)`.
- Ciclos base: **4**.

Si **op == 0xCB**:

- Se lee el **segundo byte** como opcode extendido.
- Ciclos base para CB: **8** (o **16** si se accede a `(HL)`, es decir `r==6`).
- El switch usa **`op >> 3`** para el grupo y **`op & 7`** para el registro:
  - 0x00–0x07: rotaciones/desplazamientos (RLC, RRC, RL, RR, SLA, SRA, SWAP, SRL).
  - 0x08–0x0F: **BIT** (test de bit): solo actualizan Z, N=0, H=1.
  - 0x10–0x17: **RES** (poner bit a 0).
  - 0x18–0x1F: **SET** (poner bit a 1).
- Después del switch CB se hace `return cycles`.

### 3.5 Switch principal (opcodes 0x00–0xFF)

Si no fue CB, se entra al **switch (op)**.

- **0x00**: NOP.
- **0x01–0x3F**: opcodes “regulares” por valor (LD, INC, DEC, rotaciones, ADD HL, DAA, etc.). Cada `case` implementa exactamente una instrucción (a veces agrupando varias que hacen lo mismo con distintos registros en otro nivel).
- **0x40–0x7F**: en el **default** se trata como bloque:
  - **LD r,r’** (incluye LD (HL),(HL) que es HALT):  
    `dst = (op>>3)&7`, `src = op&7`.  
    Caso especial **0x76 (HALT)**:
    - Si IME=0 y (IF&IE)!=0 → **HALT bug**: no se pone `halted` (no se duerme).
    - Si no → `cpu->halted = 1`.
  - Si no es HALT: `write_r8(..., dst, read_r8(..., src))`. Ciclos extra si se usa (HL).
- **0x80–0xBF**: bloque ALU con A:
  - `v = read_r8(..., op&7)`.
  - Según `(op>>3)&7`: ADD, ADC, SUB, SBC, AND, XOR, OR, CP. Se usan las funciones `alu_*`.
- **0xC0 en adelante**: otro switch con:
  - RET condicional/inc condicional, POP, JP, CALL, push, RST, RETI, etc.
  - **0xF3**: DI → `ime = 0`, `ime_delay = 0`.
  - **0xFB**: EI → `ime_delay = 1`.
  - **0xD9**: RETI → pop PC y **IME = 1**.

Al final del switch se **return cycles** (el valor se va actualizando en cada caso con `cycles = 8`, `12`, `16`, etc.).

---

## 4. Cómo encontrar una instrucción en el código

- **Opcodes 0x00–0x3F**: buscar `case 0xXX:` en el switch principal (desde ~línea 336). Ej.: NOP=0x00, LD BC,nn=0x01, LD (BC),A=0x02, etc.
- **LD r,r’ (0x40–0x7F)**: en el `default`, bloque `if (op >= 0x40 && op <= 0x7F)`. HALT = 0x76.
- **ALU A,r (0x80–0xBF)**: mismo `default`, bloque `if (op >= 0x80 && op <= 0xBF)` con el switch interno ADD/ADC/SUB/SBC/AND/XOR/OR/CP.
- **Saltos, CALL, RET, RST, EI/DI/RETI (0xC0–0xFF)**: en el mismo `default`, bloque `if (op >= 0xC0)` y el switch por `op`.
- **Prefijo CB**: justo después de `if (op == 0xCB)`, switch por `op >> 3` (grupos) y dentro se usa `op & 7` para el registro.

Para una tabla opcode → nombre de instrucción podés usar cualquier referencia del LR35902 (por ejemplo “Game Boy CPU (SM83) instruction set” o “Pastraiser”).

---

## 5. Resumen de “dónde está qué”

| Qué | Dónde (aprox.) |
|-----|-----------------|
| Registros y flags | `cpu.h`: struct `gb_cpu`; en cpu.c, macros HL/BC/DE y R(r). |
| read/write por índice r | `read_r8` / `write_r8` (r=6 → dirección HL). |
| Init / reset | `gb_cpu_init`, `gb_cpu_reset`. |
| Interrupciones | `dispatch_interrupt`; prioridad y vectores en esta función. |
| EI/DI/HALT/RETI | En `gb_cpu_step`: inicio (ime_delay→IME); bloque HALT; en switch: 0xF3, 0xFB, 0x76 (en bloque 0x40–0x7F), 0xD9. |
| ALU (ADD, SUB, AND, …) | Funciones `alu_*`; usadas en bloque 0x80–0xBF y en algunos opcodes concretos (ej. ADD A,n). |
| Prefijo CB (rotaciones, BIT, RES, SET) | Tras `if (op == 0xCB)`, switch por `op >> 3`. |
| Resto de instrucciones | Switch principal por `op`, y en el `default` los rangos 0x40–0x7F, 0x80–0xBF, 0xC0+ con su propio switch. |
| Ciclos | Variable `cycles`; se inicializa a 4 (o 8 para CB) y se sobrescribe en cada caso; al final `return cycles`. |

---

## 6. Documentación de referencia

- **Especificación de la CPU (registros, flags, interrupciones)**: `Documentation/05-subsystems/01-cpu.md`.
- **Este archivo**: guía del **código** en `be1/cpu.c` para seguir el flujo y localizar cada parte de la emulación.

Si querés depurar una instrucción concreta: poné un breakpoint en `gb_cpu_step`, ejecutá hasta que `cpu->pc` sea la dirección del opcode que te interesa y entrá al `case` correspondiente; los ciclos que devuelve esa ejecución son los que se usan para el timer y el resto del sistema.
