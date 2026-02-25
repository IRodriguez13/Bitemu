# Bitemu BE1 - Game Boy Emulator
# make        → bitemu (Rust GUI)
# make cli    → bitemu-cli (C headless)

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

.PHONY: all clean cli gui

all: gui

gui:
	cd rust && CARGO_TARGET_DIR="$$(pwd)/target" cargo build --release
	cp rust/target/release/bitemu bitemu

cli: $(CLI_TARGET)

$(CLI_TARGET): $(CLI_OBJS)
	$(CC) $(CLI_OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLI_OBJS) $(CLI_TARGET) bitemu
	cd rust && cargo clean
