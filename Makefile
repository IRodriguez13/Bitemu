# Bitemu BE1 - Game Boy Emulator
# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I.
LDFLAGS =

CORE_DIR = core
UTILS_DIR = core/utils
BE1_DIR = BE1

CORE_SRCS = $(CORE_DIR)/engine.c $(CORE_DIR)/timing.c $(CORE_DIR)/input.c
UTILS_SRCS = $(UTILS_DIR)/log.c $(UTILS_DIR)/helpers.c
BE1_SRCS = $(BE1_DIR)/console.c $(BE1_DIR)/cpu.c $(BE1_DIR)/ppu.c $(BE1_DIR)/apu.c $(BE1_DIR)/memory.c

SRCS = main.c $(CORE_SRCS) $(UTILS_SRCS) $(BE1_SRCS)
OBJS = $(SRCS:.c=.o)
TARGET = bitemu

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
