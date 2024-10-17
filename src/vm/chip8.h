#pragma once

#include <stdint.h>


#define CHIP8_HEAP_SIZE 4096
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define SCALE_FACTOR 10

typedef struct {
    uint8_t display[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint8_t heap[CHIP8_HEAP_SIZE];
    uint8_t reg[16];
    uint16_t stack[64];
    uint16_t regi;
    uint16_t pc;
    uint8_t dt;
    uint8_t st;
    int8_t sp;
} Chip8;

void chip8_init(Chip8 *p_ctx);
void chip8_fini(Chip8 *p_ctx);
void chip8_loop(Chip8 *p_ctx);
