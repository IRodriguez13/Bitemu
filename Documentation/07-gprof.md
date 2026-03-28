# Perfilado con GNU gprof (Bitemu)

**gprof** no tiene nada que ver con **gperf** (generador de tablas hash del proyecto). Aquí solo se describe **gprof**: muestreo por función con `-pg`, lectura de `gmon.out` y qué mirar en la salida.

`make test` / `make test-core` son la suite de regresión; **no** sustituyen un informe de tiempo por función. Para eso usá el flujo de abajo.

---

## Flujo rápido en este repo

```bash
# 1) CLI instrumentado (-pg, sin LTO)
make profile-cli

# 2) Ejecutar y salir con normalidad (se escribe gmon.out en el directorio actual)
./bitemu-cli -rom ruta/tu.rom

# 3) Informe en gprof.out
make profile-report
less gprof.out
```

### Genesis sin frontend (ROM de test + gprof)

**gprof** mide **tiempo de CPU en tu máquina** (muestreo), no “ciclos 68k” literal. Para calor real de emulación Genesis necesitás una ROM que ejercite el core; una opción libre es **gentest** (`clbr/gentest`, `game.bin.xz` en el repo).

```bash
bash scripts/fetch_gentest_rom.sh   # descarga y deja tools/gentest_game.bin (no se commitea)
make profile-cli
./bitemu-cli -rom tools/gentest_game.bin -frames 8000   # salida limpia → gmon.out
make profile-report
less gprof.out
```

O en un paso: `bash scripts/profile_genesis_gprof.sh [ruta.rom] [frames]`.

**Importante:** si matás el proceso con `timeout`/`SIGKILL`, a veces **no** se llega a escribir `gmon.out`. Usá **`-frames N`** para terminar con `exit` normal.

**Nota sobre ciclos 68k:** para contadores por instrucción o por componente del chip, hace falta **instrumentación** o **simulador con hooks**; gprof solo atribuye tiempo a **funciones C** del proceso host.

Variaciones útiles:

| Objetivo | Comando |
|----------|---------|
| Solo librería con `-pg` (p. ej. otro binario que enlace `libbitemu`) | `make profile-lib` |
| Core C instrumentado: lib + `build/test_runner` y ejecutar tests | `make profile-test-core` |
| Informe si el binario perfilado no es el CLI | `make profile-report PROFILE_BIN=build/test_runner` |

Tras `profile-test-core`, en la raíz del repo suele aparecer `gmon.out`; entonces:

```bash
make profile-report PROFILE_BIN=build/test_runner
```

Más opciones: `make profile-help`, `make clean-profile-data` (borra `gmon.out`, `gprof.out`, etc.).

---

## ¿Qué es `gprof`?

- Es un **programa** que viene con la cadena de herramientas GCC (GNU Binutils).
- Lee **`gmon.out`**, generado al salir de un proceso compilado/enlazado con **`-pg`**.
- En Bitemu, **`PROFILE=1` es el valor por defecto** del `Makefile` (cualquier `make cli` / `make lib` queda con `-pg`). Para un binario release sin instrumentar: **`PROFILE=0 make cli`**. Los objetivos `profile-cli` / `profile-lib` / `profile-test-core` fuerzan rebuild limpio con `-pg`.

---

## Cómo leer la salida (flat profile)

Tras `make profile-report`, abrís **`gprof.out`**. La sección **Flat profile** es la más directa para ver “dónde se va el tiempo”.

### Ejemplo ilustrativo (no es una corrida real)

```
Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total
 time   seconds   seconds    calls  ms/call  ms/call  name
 60.00      0.12     0.12     8000     0.01     0.01  ym_op_output
 20.00      0.16     0.04     8000     0.01     0.01  ym_step_envelope
 10.00      0.18     0.02        1    20.00   200.00  gen_ym2612_run_cycles
  5.00      0.19     0.01     6000     0.00     0.00  ym_advance_phase
  5.00      0.20     0.01     8000     0.00     0.00  ym_tl_linear_atten
```

### Columnas

| Columna | Significado |
|--------|-------------|
| **% time** | Fracción del tiempo total atribuido a esa función (lo primero que conviene ordenar mentalmente). |
| **cumulative** | Tiempo acumulado hasta esa fila (segundos), en el orden en que lista gprof. |
| **self** | Tiempo “dentro” de la función, sin contar tiempo en callees. |
| **calls** | Número de veces que se contó entrar a la función (según el muestreo). |
| **self ms/call** | `self / calls`, en ms por llamada. |
| **total ms/call** | Tiempo por llamada incluyendo lo que hacen las funciones llamadas. |
| **name** | Nombre demangled / símbolo. |

### Qué optimizar

- Priorizá funciones con **alto % time** (regla de Pareto: pocas funciones suelen concentrar la mayor parte).
- Mejorar algo con **bajo % time** suele dar poco retorno salvo que esté en un camino crítico muy optimizable.

---

## Call graph

Más abajo en `gprof.out`, la sección **Call graph** muestra **quién llama a quién** y cómo se reparte el tiempo entre caller y callee. Sirve para entender por qué una función aparece alto en el flat profile (¿la llaman muchas veces desde un hot path?, ¿una sola llamada muy cara?).

---

## Notas prácticas

- **LTO:** en Bitemu, `PROFILE=1` desactiva `-flto` porque suele interferir con `-pg`.
- **Optimización:** el perfil por defecto usa **`-O2`** con `PROFILE=1`; los porcentajes reflejan ese binario, no necesariamente `-O3`+LTO.
- **macOS:** `gprof` suele ir con toolchain GNU; con Clang puro a veces conviene otras herramientas (p. ej. Instruments). `make profile-help` indica la nota en no-Linux.
- **Linux `perf`:** para muestreo del kernel y flamegraphs distintos, ver `make perf-help`.

---

## gperf (tablas de ciclos) vs gprof

- **gperf** en este proyecto: tablas generadas (`cycles*.gperf` → `*_gperf.h`) y API en `be1/cpu/cycle_sym.h`, `be2/cpu/cycle_sym.h` para **ciclos de referencia por opcode / línea de decode**, no para medir tiempo real de CPU en tu máquina.
- **gprof**: mide dónde el **binario que corriste** gastó tiempo (muestreo estadístico). Comandos: `make gperf-help` vs `make profile-help`.
