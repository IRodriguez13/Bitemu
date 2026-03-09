# Bitemu BE1 - Game Boy Emulator
# Backend: C (core + libbitemu.so). Frontend: Python (PySide6). CLI: C (bitemu-cli).
# make all   → compila lib + cli y ejecuta frontend Python
# make help  → ver objetivos

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I. -Iinclude
LDFLAGS =

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
SRC_DIR = src

CORE_SRCS = $(CORE_DIR)/engine.c $(CORE_DIR)/timing.c $(CORE_DIR)/input.c
UTILS_SRCS = $(UTILS_DIR)/log.c $(UTILS_DIR)/helpers.c $(UTILS_DIR)/crc32.c
BE1_SRCS = $(BE1_DIR)/console.c $(BE1_DIR)/cpu/cpu.c $(BE1_DIR)/cpu/cpu_handlers.c $(BE1_DIR)/ppu.c $(BE1_DIR)/apu.c $(BE1_DIR)/timer.c $(BE1_DIR)/memory.c $(BE1_DIR)/input.c $(BE1_AUDIO_SRCS)
API_SRCS = $(SRC_DIR)/bitemu.c

CLI_SRCS = main_cli.c $(API_SRCS) $(CORE_SRCS) $(UTILS_SRCS) $(BE1_SRCS)
CLI_OBJS = $(CLI_SRCS:.c=.o)
CLI_TARGET = bitemu-cli

TEST_DIR   = tests/core
TEST_SRCS  = $(TEST_DIR)/test_runner.c $(TEST_DIR)/test_memory.c $(TEST_DIR)/test_apu.c $(TEST_DIR)/test_timer.c $(TEST_DIR)/test_api.c $(TEST_DIR)/test_ppu.c $(TEST_DIR)/test_mbc2.c $(TEST_DIR)/test_abi_guard.c
TEST_BIN   = build/test_runner
VENV       = frontend/venv/bin

.PHONY: all clean cli lib run help test test-core test-frontend

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
	@echo "  test-frontend Solo tests del frontend Python"
	@echo "  clean        Borra binarios y objetos"
	@echo "  help         Esta ayuda"

run: lib frontend/venv/.done
	cd frontend && ./venv/bin/python main.py

frontend/venv/.done: frontend/requirements.txt
	cd frontend && python3 -m venv venv && ./venv/bin/pip install -r requirements.txt && touch venv/.done

cli: $(CLI_TARGET)

lib: $(LIB_TARGET)

$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CLI_OBJS) -o $@ $(LDFLAGS) $(LIB_LIBS)

LIB_SRCS = $(API_SRCS) $(CORE_SRCS) $(UTILS_SRCS) $(BE1_SRCS)
LIB_OBJS = $(patsubst %.c,build/lib/%.o,$(LIB_SRCS))
$(LIB_TARGET): $(LIB_OBJS)
	$(CC) -shared -o $@ $(LIB_OBJS) $(LDFLAGS) $(LIB_LIBS)
build/lib/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: test-core test-frontend

test-core: lib
	@mkdir -p build
	$(CC) $(CFLAGS) -Itests/core $(TEST_SRCS) -L. -lbitemu -o $(TEST_BIN)
ifeq ($(UNAME_S),Darwin)
	DYLD_LIBRARY_PATH=. ./$(TEST_BIN)
else ifeq ($(OS),Windows_NT)
	PATH=".:$$PATH" ./$(TEST_BIN)
else
	LD_LIBRARY_PATH=. ./$(TEST_BIN)
endif

test-frontend: lib frontend/venv/.done
	$(VENV)/python -m pytest tests/frontend/ -v

clean:
	rm -f $(CLI_OBJS) $(CLI_TARGET) libbitemu.so libbitemu.dylib libbitemu.dll bitemu.dll
	rm -rf build
