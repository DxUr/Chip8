BUILD_DIR = build
CC = gcc
CFLAGS = -Isrc -std=c23 -Wall -Wextra -O3 -DDEBUG

SRC = $(shell find src -type f -name '*.c')
OBJ = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC))

LIBS = -lraylib
BIN = $(BUILD_DIR)/chip8

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)   # Create the necessary directory structure in build/ for object files
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	$(BIN) test.ch8

clean:
	rm -rf $(BUILD_DIR)/*
