# Sincronización 68000 ↔ Z80 (BE2)

Módulo: `be2/cpu_sync/` (`cpu_sync.h`, `cpu_sync.c`). Consumidores: `be2/console.c`, `be2/memory.c`, `be2/z80/z80.c`.

## Objetivo

Centralizar reglas que antes estaban duplicadas o implícitas, y **cerrar el gap** de que el 68000 pudiera leer/escribir la RAM del Z80 mientras el coprocesador corre sin protocolo de bus.

## Decisiones de modelo

### 1. Eje temporal (ciclos 68k)

Todo el slice de emulación (`gen_step`) se mide en **ciclos de bus del 68000**. VDP, YM, PSG y avance de muestras de audio se anclan a ese mismo eje.

### 2. Relación Z80 / 68000

- Frecuencia Z80 modelada como **`GEN_PSG_HZ`** (NTSC) / **`GEN_PSG_HZ_PAL`** (PAL), coherente con ~master÷15.
- Frecuencia 68000: **`GEN_68000_HZ`** / **`GEN_68000_HZ_PAL`**.
- Función: `gen_cpu_sync_z80_cycles_from_68k(cycles_68k, is_pal)` → ciclos Z80 a ejecutar en el mismo intervalo de tiempo que `cycles_68k` al 68k.

**No modelado:** jitter, esperas de E/S del Z80 ni diferencias de fase entre chips más allá de esta proporción global.

### 3. Cuándo corre el Z80

`gen_cpu_sync_z80_should_run(z80_bus_req, z80_reset)`:

- El Z80 **solo avanza** si `z80_bus_req == 0` y `z80_reset != 0`.
- `z80_bus_req` es el valor que el **software 68k** escribe en **`0xA11100`** (bit 0 = pedido de bus). Valor **≠ 0 con bit0=1**: el core **no** ejecuta pasos del Z80 (misma semántica que ya tenía `gen_z80_step`).
- `z80_reset == 0`: Z80 en reset (línea `0xA11200`), no ejecuta.

### 4. BUSACK (retraso de acceso a RAM Z80)

Tras escribir **BUSREQ** con bit0=1 en `0xA11100`, el contador `z80_bus_ack_cycles` (en `genesis_impl_t`, enlazado desde `genesis_mem_t`) se carga con **`GEN_Z80_BUSACK_CYCLES_68K`**. Mientras `*z80_bus_ack_cycles > 0`, el 68k **no** puede acceder a la work RAM del Z80 aunque `bus_req ≠ 0`. El contador decrece con los mismos ciclos de bus que avanzan VDP/audio y el consumo en stall DMA (`gen_step`).

### 5. Acceso 68k a la work RAM del Z80 (`0xA00000`–`0xA0FFFF`, 8 KB espejados)

`gen_cpu_sync_m68k_may_access_z80_work_ram(z80_bus_req, z80_bus_ack_cycles)`:

- Con puntero `z80_bus_req` NULL → no acceso.
- Con `*z80_bus_req == 0` → **denegado** (Z80 puede estar corriendo).
- Con `bus_ack_rem` no NULL y `*bus_ack_rem > 0` → **denegado** (BUSACK pendiente).
- En caso contrario → permitido.

**Lecturas denegadas** usan **`gen_cpu_sync_z80_ram_contention_read(addr)`** (valor dependiente de la dirección; no un flat `0xFF`). **Escrituras** denegadas se ignoran.

### 6. Qué no cubre este módulo

| Tema | Estado |
|------|--------|
| Acceso Z80 a ROM del cartucho vía mapeo `0x8000` + bank | Sigue en `z80.c`; no pasa por `cpu_sync`. |
| YM / PSG desde 68k vs Z80 | Lockstep por ciclos 68k en `console.c`. |
| DMA VDP | `vdp.c` + `dma_stall_68k`; stalls distintos por tipo (fill / copy / 68k). |
| VDP render | Tope **20 sprites por línea** en dibujado (orden SAT); H-int solo en zona activa; `gen_vdp_pending_irq_level` alinea H-IRQ con líneas visibles. |
| Prefetch 68k | Cola de instrucción en `gen_cpu_t` (`prefetch_addr` / `ir_prefetch`); no sustituye contienda fina en `cpu_sync`. |

### 7. Burbuja de bus 68k (`gen_cpu_sync_m68k_bus_extra_cycles`)

- **Ramas / bifurcaciones:** si `last_opcode` tiene byte alto en `0x60–0x6F`, +1 (modelo burdo de prefetch/destino).
- **VDP:** +1 cuando `gen_vdp_m68k_bus_slice_extra` es verdadero: queda **`dma_stall_68k > 0`** (el 68k aún “paga” el coste del último DMA) **o** **`GEN_VDP_STATUS_DMA` sin `dma_fill_pending`** (copia/68k-DMA no-fill). Si **`dma_fill_pending`**, no se suma esta parte (el bloqueo principal es el fill vía puerto datos).
- Tope **3** ciclos extra por slice para mantener el comportamiento acotado.

## VDP / audio / stats (roadmap cerrado en core)

- **YM2612:** timers A/B aproximados (reg `$24-$27`), bits 0–1 del status; busy sigue teniendo prioridad en lectura.
- **Métricas:** `genesis_impl_t` expone `stat_cpu_cyc`, `stat_z80_cyc`, `stat_dma_stall_consumed`; API opcional `bitemu_genesis_get_core_stats` en `bitemu.h`.

## Pruebas

- `tests/be2/test_genesis_memory.c`: RAM Z80, BUSACK, timers YM vía bus.
- `tests/be2/test_genesis_vdp.c`: stall DMA fill.
- `tests/be2/test_abi_guard.c`: layout `gen_ym2612_t` ampliado.

## Cambios de comportamiento respecto a versiones anteriores

El 68k ya no puede usar la RAM Z80 sin **BUSREQ** y sin cumplir el **retraso BUSACK** modelado. Las lecturas vetadas dejan de ser siempre `0xFF`. Los save states v1 amplían el bloque GEN1 con `z80_bus_ack_cycles` y estadísticas; archivos antiguos siguen cargando vía `bst_read_section` y `extsz` parcial.
