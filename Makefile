# Bitemu BE1 - Game Boy Emulator
# Backend: C (core + libbitemu.so). Frontend: Python (PySide6). CLI: C (bitemu-cli).
# make all   → compila lib + cli y ejecuta frontend Python
# make help  → ver objetivos

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I. -Iinclude
LDFLAGS =

CORE_DIR = core
UTILS_DIR = core/utils
BE1_DIR = be1
SRC_DIR = src

CORE_SRCS = $(CORE_DIR)/engine.c $(CORE_DIR)/timing.c $(CORE_DIR)/input.c
UTILS_SRCS = $(UTILS_DIR)/log.c $(UTILS_DIR)/helpers.c
BE1_SRCS = $(BE1_DIR)/console.c $(BE1_DIR)/cpu/cpu.c $(BE1_DIR)/cpu/cpu_handlers.c $(BE1_DIR)/ppu.c $(BE1_DIR)/apu.c $(BE1_DIR)/timer.c $(BE1_DIR)/memory.c $(BE1_DIR)/input.c
API_SRCS = $(SRC_DIR)/bitemu.c

CLI_SRCS = main_cli.c $(API_SRCS) $(CORE_SRCS) $(UTILS_SRCS) $(BE1_SRCS)
CLI_OBJS = $(CLI_SRCS:.c=.o)
CLI_TARGET = bitemu-cli

.PHONY: all clean cli lib run help

all: lib cli run

help:
	@echo "Bitemu - Game Boy Emulator (C backend, Python frontend)"
	@echo "Objetivos: all, lib, cli, run, clean, help"
	@echo "  all   (def) Compila lib+cli y ejecuta frontend Python"
	@echo "  lib   libbitemu.so (para el frontend)"
	@echo "  cli   bitemu-cli (emulador en terminal)"
	@echo "  run   Ejecuta frontend Python (ventana)"
	@echo "  clean Borra binarios y objetos"
	@echo "  help  Esta ayuda"

run: lib frontend/venv/.done
	cd frontend && ./venv/bin/python main.py

frontend/venv/.done: frontend/requirements.txt
	cd frontend && python3 -m venv venv && ./venv/bin/pip install -r requirements.txt && touch venv/.done

cli: $(CLI_TARGET)

lib: libbitemu.so

$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CLI_OBJS) -o $@ $(LDFLAGS)

LIB_SRCS = $(API_SRCS) $(CORE_SRCS) $(UTILS_SRCS) $(BE1_SRCS)
LIB_OBJS = $(patsubst %.c,build/lib/%.o,$(LIB_SRCS))
libbitemu.so: $(LIB_OBJS)
	$(CC) -shared -o $@ $(LIB_OBJS) $(LDFLAGS)
build/lib/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLI_OBJS) $(CLI_TARGET) libbitemu.so
	rm -rf build
