# Bitemu BE1 - Game Boy Emulator
# Backend: C (core + libbitemu.so). Frontend: Python (PySide6). CLI: C (bitemu-cli).
# make all   → compila lib + cli y ejecuta frontend Python
# make help  → ver objetivos
# make DEBUG=1     → build sin optimizaciones (debug)
# PROFILE por defecto = 1 (gprof -pg, sin LTO). Release: PROFILE=0 make cli
# make FRAMEPOINTERS=1→ -fno-omit-frame-pointer -g (Linux perf, callgraphs sin dwarf)

CC = gcc
DEBUG ?= 0
PROFILE ?= 1
ifeq ($(PROFILE),1)
  # gprof: -flto y -pg suelen no mezclar; -O2 para perfil útil
  CFLAGS = -Wall -Wextra -std=c11 -I. -Iinclude -O2 -g -pg -fno-omit-frame-pointer -DNDEBUG
  LDFLAGS = -pg
else ifeq ($(DEBUG),1)
  CFLAGS = -Wall -Wextra -std=c11 -I. -Iinclude -O0 -g
  LDFLAGS =
else
  CFLAGS = -Wall -Wextra -std=c11 -I. -Iinclude -O3 -flto -DNDEBUG
  LDFLAGS = -flto
endif

# perf(Linux): stacks en -O3 suelen necesitar frame pointers + símbolos
FRAMEPOINTERS ?= 0
ifeq ($(FRAMEPOINTERS),1)
  ifneq ($(PROFILE),1)
    CFLAGS += -fno-omit-frame-pointer -g
  endif
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LIB_EXT = dylib
  LIB_LIBS = -framework AudioToolbox -framework CoreFoundation -lm
  BE1_AUDIO_SRCS = $(BE1_DIR)/audio/audio_coreaudio.c
else ifeq ($(OS),Windows_NT)
  LIB_EXT = dll
  LIB_LIBS = -lole32 -lpthread -lm
  BE1_AUDIO_SRCS = $(BE1_DIR)/audio/audio_wasapi.c
else
  LIB_EXT = so
  LIB_LIBS = -lasound -lpthread -lm
  BE1_AUDIO_SRCS = $(BE1_DIR)/audio/audio_alsa.c
endif
LIB_TARGET = libbitemu.$(LIB_EXT)

CORE_DIR = core
UTILS_DIR = core/utils
BE1_DIR = be1
BE2_DIR = be2
# BE3: reservado (futuro backend); gperf-be3 no falla si el directorio no existe.
BE3_DIR = be3
SRC_DIR = src

CORE_SRCS = $(CORE_DIR)/engine.c $(CORE_DIR)/timing.c $(CORE_DIR)/input.c $(CORE_DIR)/savestate.c $(CORE_DIR)/simd/simd.c
UTILS_SRCS = $(UTILS_DIR)/log.c $(UTILS_DIR)/helpers.c $(UTILS_DIR)/crc32.c
BE1_SRCS = $(BE1_DIR)/console.c $(BE1_DIR)/cpu/cpu.c $(BE1_DIR)/cpu/cpu_handlers.c $(BE1_DIR)/cpu/cycle_sym_main_table.c $(BE1_DIR)/cpu/cycle_sym_cb_table.c $(BE1_DIR)/ppu.c $(BE1_DIR)/apu.c $(BE1_DIR)/timer.c $(BE1_DIR)/memory.c $(BE1_DIR)/input.c $(BE1_AUDIO_SRCS)
BE2_SRCS = $(BE2_DIR)/console.c $(BE2_DIR)/memory.c $(BE2_DIR)/cpu_sync/cpu_sync.c $(BE2_DIR)/vdp/vdp.c $(BE2_DIR)/ym2612/ym2612.c $(BE2_DIR)/psg/psg.c $(BE2_DIR)/z80/z80.c $(BE2_DIR)/cpu/cpu.c $(BE2_DIR)/cpu/cpu_handlers.c $(BE2_DIR)/cpu/cycle_sym.c $(BE2_DIR)/input.c
API_SRCS = $(SRC_DIR)/bitemu.c

CLI_SRCS = main_cli.c $(API_SRCS) $(CORE_SRCS) $(UTILS_SRCS) $(BE1_SRCS) $(BE2_SRCS)
CLI_OBJS = $(CLI_SRCS:.c=.o)
CLI_TARGET = bitemu-cli

TEST_DIR   = tests/core
TEST_BE2   = tests/be2
TEST_SRCS  = $(TEST_DIR)/test_runner.c $(TEST_DIR)/test_memory.c $(TEST_DIR)/test_apu.c $(TEST_DIR)/test_timer.c $(TEST_DIR)/test_api.c $(TEST_DIR)/test_simd.c $(TEST_DIR)/test_ppu.c $(TEST_DIR)/test_mbc2.c $(TEST_DIR)/test_abi_guard.c $(TEST_BE2)/test_abi_guard.c $(TEST_BE2)/test_genesis_memory.c $(TEST_BE2)/test_genesis_full.c $(TEST_BE2)/test_genesis_cpu.c $(TEST_BE2)/test_genesis_vdp.c $(TEST_BE2)/test_genesis_ym2612.c
TEST_BIN   = build/test_runner
VENV       = frontend/venv/bin

ROM ?=

.PHONY: all clean clean-gperf clean-profile-data cli lib run help test test-core test-frontend \
	genesis-smoke-rom \
	gen-cycles-gperf \
	gperf-help gperf-be1 gperf-be1-check gperf-be2 gperf-be2-check gperf-be3 gperf-all \
	profile-help profile-cli profile-lib profile-test-core profile-report \
	perf-help perf-cli-ready perf-record-cli perf-stat-cli perf-report perf-version

all: lib cli run

help:
	@echo "Bitemu - Game Boy Emulator (C backend, Python frontend)"
	@echo "Objetivos: all, lib, cli, run, clean, test, help"
	@echo "  all          (def) Compila lib+cli y ejecuta frontend Python"
	@echo "  lib          libbitemu.so (para el frontend)"
	@echo "  cli          bitemu-cli (emulador en terminal)"
	@echo "  run          Ejecuta frontend Python (ventana)"
	@echo "  test         Corre tests de core (C) y frontend (Python)"
	@echo "  test-core    Solo tests del core C"
	@echo "  genesis-smoke-rom  ROM=… [FRAMES=N] — headless (default 120 frames)"
	@echo "  test-frontend Solo tests del frontend Python"
	@echo "  clean        Borra binarios y objetos"
	@echo "  clean-gperf  Borra cabeceras generadas por gperf (be*/gen/cycles_gperf.h)"
	@echo "  gen-cycles-gperf  Regenera be*/gen/cycles*.gperf y tablas .h auxiliares (Python)"
	@echo "  gperf-help   Uso de gperf por backend (ciclos / tablas)"
	@echo "  gperf-be1    Solo genera cabeceras gperf BE1 (sin compilar ni testear)"
	@echo "  gperf-be1-check  Opcional: gperf-be1 + test-core (regresión tras editar tablas)"
	@echo "  gperf-be2    Solo genera cabeceras gperf BE2"
	@echo "  gperf-be2-check  Opcional: gperf-be2 + test-core"
	@echo "  gperf-be3    Idem BE3 (sin error si $(BE3_DIR)/ no existe aún)"
	@echo "  gperf-all    Los tres"
	@echo "  profile-help Perfilado con GNU gprof (por defecto ya compilás con -pg)"
	@echo "  profile-cli  Recompila solo bitemu-cli con -pg"
	@echo "  profile-lib  Recompila libbitemu con -pg"
	@echo "  profile-test-core  clean + lib + test-core con -pg (genera gmon.out)"
	@echo "  profile-report  gprof → gprof.out (tras ejecutar el binario perfilado)"
	@echo "  clean-profile-data  Borra gmon.out, gprof.out, perf.data*"
	@echo "  perf-help    Linux perf (perf_event_open), ver perf-record-cli"
	@echo "  perf-cli-ready  Recompila bitemu-cli con FRAMEPOINTERS=1"
	@echo "  help         Esta ayuda"
	@echo ""
	@echo "  DEBUG=1      Build sin optimizaciones (-O0 -g, sin LTO)"
	@echo "  PROFILE=0    Build release (-O3 -flto, sin -pg). Por defecto PROFILE=1."
	@echo "  FRAMEPOINTERS=1  -fno-omit-frame-pointer -g (útil con perf record -g)"

run: lib frontend/venv/.done
	cd frontend && ./venv/bin/python main.py

frontend/venv/.done: frontend/requirements.txt
	cd frontend && python3 -m venv venv && ./venv/bin/pip install -r requirements.txt && touch venv/.done

cli: $(CLI_TARGET)

lib: $(LIB_TARGET)

$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CLI_OBJS) -o $@ $(LDFLAGS) $(LIB_LIBS)

LIB_SRCS = $(API_SRCS) $(CORE_SRCS) $(UTILS_SRCS) $(BE1_SRCS) $(BE2_SRCS)
LIB_OBJS = $(patsubst %.c,build/lib/%.o,$(LIB_SRCS))
$(LIB_TARGET): $(LIB_OBJS)
	$(CC) -shared -o $@ $(LIB_OBJS) $(LDFLAGS) $(LIB_LIBS)
build/lib/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: test-core test-frontend

FRAMES ?= 120

genesis-smoke-rom: cli
	@test -n "$(ROM)" || (echo "Uso: make genesis-smoke-rom ROM=/ruta/al/juego.md"; exit 1)
ifeq ($(UNAME_S),Darwin)
	DYLD_LIBRARY_PATH=. ./$(CLI_TARGET) -rom "$(ROM)" --cli -frames $(FRAMES)
else ifeq ($(OS),Windows_NT)
	PATH=".;$$PATH" ./$(CLI_TARGET) -rom "$(ROM)" --cli -frames $(FRAMES)
else
	LD_LIBRARY_PATH=. ./$(CLI_TARGET) -rom "$(ROM)" --cli -frames $(FRAMES)
endif

test-core: lib
	@mkdir -p build
	$(CC) $(CFLAGS) -Itests/core $(TEST_SRCS) -L. -lbitemu -o $(TEST_BIN) $(LDFLAGS)
ifeq ($(UNAME_S),Darwin)
	DYLD_LIBRARY_PATH=. ./$(TEST_BIN)
else ifeq ($(OS),Windows_NT)
	PATH=".:$$PATH" ./$(TEST_BIN)
else
	LD_LIBRARY_PATH=. ./$(TEST_BIN)
endif

test-frontend: lib frontend/venv/.done
	$(VENV)/python -m pytest tests/frontend/ -v

clean: clean-profile-data
	rm -f $(CLI_OBJS) $(CLI_TARGET) libbitemu.so libbitemu.dylib libbitemu.dll bitemu.dll
	rm -rf build

clean-profile-data:
	rm -f gmon.out gprof.out perf.data perf.data.old

# ---------------------------------------------------------------------------
# Linux perf (perf_events) — sin -pg; kernel + muestreo por PMU.
# Recomendado: make perf-cli-ready && make perf-record-cli PERF_ROM=...
# Permisos: a veces hace falta kernel.perf_event_paranoid<=1 o ejecutar como root.
# ---------------------------------------------------------------------------
PERF_ROM ?=
PERF_EXTRA ?=
PERF_FREQ ?= 997
# Ej.: PERF_FLAGS=--call-graph dwarf  (mejor stack si no usás FRAMEPOINTERS=1)
PERF_FLAGS ?=

perf-version:
	@perf --version 2>/dev/null || { echo "Instalá el paquete perf (linux-tools-\$$(uname -r) o similar)."; exit 1; }

perf-help:
	@echo "Linux perf (muestreo hardware, call graphs)"
	@echo "  1) make perf-cli-ready     # CLI con frame pointers + -g (recomendado)"
	@echo "  2) make perf-record-cli PERF_ROM=ruta/juego.gb"
	@echo "  3) perf report    # TUI; o: make perf-report (solo texto)"
	@echo "  Rápido:  make perf-stat-cli PERF_ROM=ruta/juego.gb"
	@echo "  Vars: PERF_FREQ, PERF_FLAGS (p. ej. --call-graph dwarf), PERF_EXTRA (más args al CLI)"
	@echo "  Limpiar muestreos: make clean-profile-data"

perf-cli-ready:
	rm -f $(CLI_OBJS) $(CLI_TARGET)
	@$(MAKE) FRAMEPOINTERS=1 $(CLI_TARGET)

perf-record-cli: perf-version
	@if [ "$(UNAME_S)" != "Linux" ]; then echo "perf-record-cli está pensado para Linux."; exit 1; fi
	@test -f "$(CLI_TARGET)" || { echo "Compilá antes: make perf-cli-ready   (o make cli)"; exit 1; }
	@test -n "$(PERF_ROM)" || { echo "Uso: make perf-record-cli PERF_ROM=ruta/rom.gb [PERF_EXTRA='…']"; exit 1; }
	perf record $(PERF_FLAGS) -g -F $(PERF_FREQ) -- ./$(CLI_TARGET) $(PERF_ROM) $(PERF_EXTRA)

perf-stat-cli: perf-version
	@if [ "$(UNAME_S)" != "Linux" ]; then echo "perf-stat-cli: Linux only."; exit 1; fi
	@test -f "$(CLI_TARGET)" || { echo "Compilá antes: make cli"; exit 1; }
	@test -n "$(PERF_ROM)" || { echo "Uso: make perf-stat-cli PERF_ROM=ruta/rom.gb"; exit 1; }
	perf stat -d -- ./$(CLI_TARGET) $(PERF_ROM) $(PERF_EXTRA)

perf-report:
	@test -f perf.data || { echo "Falta perf.data → make perf-record-cli PERF_ROM=…"; exit 1; }
	perf report --stdio

# ---------------------------------------------------------------------------
# GNU gprof — muestreo por función (-pg). Guía: Documentation/07-gprof.md
#   make profile-cli && ./bitemu-cli game.gb  # salir con normalidad → gmon.out
#   make profile-report   → gprof.out
# Para la lib usada por el frontend: make profile-lib (regenerá build/lib/*.o).
# ---------------------------------------------------------------------------
PROFILE_BIN ?= $(CLI_TARGET)

profile-help:
	@echo "GNU gprof — tiempo por función (-pg). No confundir con gperf (tablas de ciclos)."
	@echo "  Guía: Documentation/07-gprof.md (flat profile, columnas %time/self/calls, call graph)"
	@echo "  1) make profile-cli"
	@echo "  2) ./$(CLI_TARGET) <rom> … ; salir con normalidad → gmon.out"
	@echo "  3) make profile-report   → gprof.out   (less gprof.out)"
	@echo "  Binario distinto: make profile-report PROFILE_BIN=build/test_runner"
	@echo "  Suite C instrumentada: make profile-test-core  (luego profile-report con PROFILE_BIN si aplica)"
	@echo "  Limpieza: make clean-profile-data"
ifneq ($(UNAME_S),Linux)
	@echo "  Nota: en macOS gprof suele ir con toolchain GNU; clang usa instruments."
endif

profile-cli:
	rm -f $(CLI_OBJS) $(CLI_TARGET)
	@$(MAKE) PROFILE=1 $(CLI_TARGET)

profile-lib:
	rm -f $(LIB_OBJS) $(LIB_TARGET)
	@$(MAKE) PROFILE=1 $(LIB_TARGET)

profile-test-core:
	@$(MAKE) clean
	@$(MAKE) PROFILE=1 lib
	@$(MAKE) PROFILE=1 test-core

profile-report:
	@test -f gmon.out || { echo "Falta gmon.out: ejecutá ./$(PROFILE_BIN) (binario -pg; make cli usa -pg por defecto) y salí con normalidad."; exit 1; }
	@test -f $(PROFILE_BIN) || { echo "Falta $(PROFILE_BIN) o usá PROFILE_BIN=ruta/al/binario"; exit 1; }
	gprof -b $(PROFILE_BIN) gmon.out > gprof.out
	@echo "Escrito gprof.out"

# ---------------------------------------------------------------------------
# gperf — tablas / lookups (p. ej. ciclos por opcode, “pérdida” vs nominal).
# Fuente: scripts/gen_cycles_gperf.py → be1/gen/cycles.gperf (+ cycles_cb), be2/gen/cycles.gperf
# Salida: be1/gen/cycles_gperf.h, cycles_cb_gperf.h, cycle_mnemonic_tables.h, be2/gen/cycles_gperf.h, cycle_line_names.h
# ---------------------------------------------------------------------------
GPERF ?= gperf

gen-cycles-gperf:
	python3 scripts/gen_cycles_gperf.py

BE1_GPERF_IN    = $(BE1_DIR)/gen/cycles.gperf
BE1_GPERF_OUT   = $(BE1_DIR)/gen/cycles_gperf.h
BE1_GPERF_CB_IN = $(BE1_DIR)/gen/cycles_cb.gperf
BE1_GPERF_CB_OUT = $(BE1_DIR)/gen/cycles_cb_gperf.h
BE2_GPERF_IN  = $(BE2_DIR)/gen/cycles.gperf
BE2_GPERF_OUT = $(BE2_DIR)/gen/cycles_gperf.h
BE3_GPERF_IN  = $(BE3_DIR)/gen/cycles.gperf
BE3_GPERF_OUT = $(BE3_DIR)/gen/cycles_gperf.h

# Por defecto vacío: el .gperf suele llevar %language, %defines, etc.
GPERF_FLAGS_BE1 ?=
GPERF_FLAGS_BE2 ?=
GPERF_FLAGS_BE3 ?=

$(BE1_GPERF_OUT): $(BE1_GPERF_IN)
	@mkdir -p $(dir $@)
	$(GPERF) -I -C -t -L ANSI-C $(GPERF_FLAGS_BE1) --output-file=$@ $<

$(BE1_GPERF_CB_OUT): $(BE1_GPERF_CB_IN)
	@mkdir -p $(dir $@)
	$(GPERF) -I -C -t -L ANSI-C $(GPERF_FLAGS_BE1) --output-file=$@ $<

$(BE2_GPERF_OUT): $(BE2_GPERF_IN)
	@mkdir -p $(dir $@)
	$(GPERF) -I -C -t -L ANSI-C $(GPERF_FLAGS_BE2) --output-file=$@ $<

$(BE3_GPERF_OUT): $(BE3_GPERF_IN)
	@mkdir -p $(dir $@)
	$(GPERF) -I -C -t -L ANSI-C $(GPERF_FLAGS_BE3) --output-file=$@ $<

gperf-help:
	@echo "gperf por backend (tablas T-ciclos / ciclos 68k por línea)"
	@echo "  1) make gen-cycles-gperf   # .gperf + cycle_mnemonic_tables.h + cycle_line_names.h"
	@echo "  2) make gperf-be1 / gperf-be2   # genera cycles*_gperf.h"
	@echo "     Opcional QA: gperf-be1-check / gperf-be2-check (+ test-core, no es gprof)"
	@echo "  Entrada : $(BE1_GPERF_IN), $(BE1_GPERF_CB_IN), $(BE2_GPERF_IN), $(BE3_GPERF_IN)"
	@echo "  Salida  : $(BE1_GPERF_OUT), $(BE1_GPERF_CB_OUT), $(BE2_GPERF_OUT), ..."
	@echo "  API C   : be1/cpu/cycle_sym.h, be2/cpu/cycle_sym.h"

gperf-be1: gen-cycles-gperf
	@$(MAKE) $(BE1_GPERF_OUT) $(BE1_GPERF_CB_OUT)

gperf-be1-check: gperf-be1
	@$(MAKE) test-core

gperf-be2: gen-cycles-gperf
	@$(MAKE) $(BE2_GPERF_OUT)

gperf-be2-check: gperf-be2
	@$(MAKE) test-core

gperf-be3:
	@if [ ! -d "$(BE3_DIR)" ]; then \
		echo "gperf-be3: $(BE3_DIR) no existe aún; omitido."; \
	elif [ ! -f "$(BE3_GPERF_IN)" ]; then \
		echo "Falta $(BE3_GPERF_IN)."; exit 1; \
	else \
		$(MAKE) $(BE3_GPERF_OUT); \
	fi

gperf-all: gperf-be1 gperf-be2 gperf-be3

clean-gperf:
	rm -f $(BE1_GPERF_OUT) $(BE1_GPERF_CB_OUT) $(BE2_GPERF_OUT) $(BE3_GPERF_OUT)
	rm -f $(BE1_DIR)/gen/cycle_mnemonic_tables.h $(BE2_DIR)/gen/cycle_line_names.h
