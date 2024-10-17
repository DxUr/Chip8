#include "chip8.h"

#include <stdint.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "log.h"

static uint8_t fonts[] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0,
    0x20, 0x60, 0x20, 0x20, 0x70,
    0xf0, 0x10, 0xf0, 0x80, 0xf0,
    0xf0, 0x10, 0xf0, 0x10, 0xf0,
    0x90, 0x90, 0xf0, 0x10, 0x10,
    0xf0, 0x80, 0xf0, 0x10, 0xf0,
    0xf0, 0x80, 0xf0, 0x90, 0xf0,
    0xf0, 0x10, 0x20, 0x40, 0x40,
    0xf0, 0x90, 0xf0, 0x90, 0xf0,
    0xf0, 0x90, 0xf0, 0x10, 0xf0,
    0xf0, 0x90, 0xf0, 0x90, 0x90,
    0xe0, 0x90, 0xe0, 0x90, 0xe0,
    0xf0, 0x80, 0x80, 0x80, 0xf0,
    0xe0, 0x90, 0x90, 0x90, 0xe0,
    0xf0, 0x80, 0xf0, 0x80, 0xf0,
    0xf0, 0x80, 0xf0, 0x80, 0x80
};

void chip8_init(Chip8 *p_ctx) {
    p_ctx->pc = 0x200;
    p_ctx->sp = -1;
    p_ctx->regi = 0;
    memset(p_ctx->heap, 0, sizeof(p_ctx->heap));
    memset(p_ctx->reg, 0, sizeof(p_ctx->reg));
    memset(p_ctx->display, 0, sizeof(p_ctx->display));
    memcpy(p_ctx->heap + CHIP8_HEAP_SIZE - sizeof(fonts) - 1, fonts, sizeof(fonts));
    InitWindow(SCREEN_WIDTH * SCALE_FACTOR, SCREEN_HEIGHT * SCALE_FACTOR, "CHIP-8 Emulator");
    SetTargetFPS(600);
}

void chip8_fini(Chip8 *) {
    CloseWindow();
}

void chip8_loop(Chip8 *p_ctx) {
    clock_t then = clock();
    while (!WindowShouldClose()) {
        if (p_ctx->pc >= CHIP8_HEAP_SIZE) {
            LOGF("invalid pc: 0x%X\n", p_ctx->pc);
            return;
        }

        if (p_ctx->dt || p_ctx->st) {
            clock_t now = clock();
            clock_t diff = now - then;
            if (diff >= CLOCKS_PER_SEC / 100) {
                if (p_ctx->dt) {
                    p_ctx->dt = p_ctx->dt - diff < 0 ? 0 : p_ctx->dt - diff;
                }
                if (p_ctx->st) {
                    p_ctx->st = p_ctx->st - diff < 0 ? 0 : p_ctx->st - diff;
                    if (!p_ctx->st) {
                        putchar('\a'); // Beep!
                    }
                }
                then = clock();
            }
        }

        uint16_t ins;
        ((uint8_t*)&ins)[1] = p_ctx->heap[p_ctx->pc];
        ((uint8_t*)&ins)[0] = p_ctx->heap[p_ctx->pc + 1];
        // LOGF("instruction: 0x%X\n", ins);
        uint8_t x, y, n, nn;
        uint16_t nnn;
        switch (ins >> 12) {
        case 0x0:
            n = ins & 0x000f;
            if (!n) {
                memset(p_ctx->display, 0, sizeof(p_ctx->display));
                p_ctx->pc += 2;
            } else {
                if (p_ctx->sp < 0) {
                    LOG("program end or returning from an empty stack");
                    return;
                }
                p_ctx->pc = p_ctx->stack[p_ctx->sp--];
            }
            break;
        case 0x1:
            p_ctx->pc = ins & 0x0fff;
            break;
        case 0x2:
            nnn = ins & 0x0fff;
            if (++p_ctx->sp > (int8_t)(sizeof(p_ctx->stack) / sizeof(p_ctx->stack[0]))) {
                LOG("stack overflow");
                return;
            }
            p_ctx->stack[p_ctx->sp] = p_ctx->pc + 2;
            p_ctx->pc = nnn;
            break;
        case 0x3:
            x = (ins & 0x0f00) >> 8;
            nn = ins & 0x00ff;
            p_ctx->pc += p_ctx->reg[x] == nn ? 4 : 2;
            break;
        case 0x4:
            x = (ins & 0x0f00) >> 8;
            nn = ins & 0x00ff;
            p_ctx->pc += p_ctx->reg[x] != nn ? 4 : 2;
            break;
        case 0x5:
            x = (ins & 0x0f00) >> 8;
            y = (ins & 0x00f0) >> 4;
            p_ctx->pc += p_ctx->reg[x] == p_ctx->reg[y] ? 4 : 2;
            break;
        case 0x6:
            x = (ins & 0x0f00) >> 8;
            nn = ins & 0x00ff;
            p_ctx->reg[x] = nn;
            p_ctx->pc += 2;
            break;
        case 0x7:
            x = (ins & 0x0f00) >> 8;
            nn = ins & 0x00ff;
            p_ctx->reg[x] += nn;
            p_ctx->pc += 2;
            break;
        case 0x8:
            x = (ins & 0x0f00) >> 8;
            y = (ins & 0x00f0) >> 4;
            n = ins & 0x000f;
            switch (n) {
            case 0x0:
                p_ctx->reg[x] = p_ctx->reg[y];
                break;
            case 0x1:
                p_ctx->reg[x] = p_ctx->reg[x] | p_ctx->reg[y];
                break;
            case 0x2:
                p_ctx->reg[x] = p_ctx->reg[x] & p_ctx->reg[y];
                break;
            case 0x3:
                p_ctx->reg[x] = p_ctx->reg[x] ^ p_ctx->reg[y];
                break;
            case 0x4:
                p_ctx->reg[x] += p_ctx->reg[y];
                p_ctx->reg[0xf] = (p_ctx->reg[x] + p_ctx->reg[y]) >> 8;
                break;
            case 0x5:
                p_ctx->reg[x] -= p_ctx->reg[y];
                p_ctx->reg[0xf] = p_ctx->reg[x] > p_ctx->reg[y] ? 0 : 1;
                break;
            case 0x6:
                p_ctx->reg[x] = p_ctx->reg[y] >> 1;
                p_ctx->reg[0xf] = p_ctx->reg[y] & 0x01;
                break;
            case 0x7:
                p_ctx->reg[x] -= p_ctx->reg[y];
                p_ctx->reg[0xf] = p_ctx->reg[x] < p_ctx->reg[y] ? 0 : 1;
                break;
            case 0xe:
                p_ctx->reg[x] = p_ctx->reg[y] << 1;
                p_ctx->reg[0xf] = (p_ctx->reg[y] & 0x80) >> 7;
                break;
            default:
                LOGF("invalid instruction: 0x%X, pc: 0x%X\n", ins, p_ctx->pc);
                return;
            }
            p_ctx->pc += 2;
            break;
        case 0x9:
            // 9XY0	Skip the following instruction if the value of register VX is not equal to the value of register VY
            x = (ins & 0x0f00) >> 8;
            y = (ins & 0x00f0) >> 4;
            p_ctx->pc += p_ctx->reg[x] != p_ctx->reg[y] ? 4 : 2;
            break;
        case 0xa:
            // ANNN	Store memory address NNN in register I
            nnn = ins & 0x0fff;
            p_ctx->regi = nnn;
            p_ctx->pc += 2;
            break;
        case 0xb:
            // BNNN	Jump to address NNN + V0
            nnn = ins & 0x0fff;
            p_ctx->pc = nnn + p_ctx->reg[0];
            break;
        case 0xc:
            // CXNN	Set VX to a random number with a mask of NN
            x = (ins & 0x0f00) >> 8;
            nn = ins & 0x00ff;
            p_ctx->reg[x] = ((uint8_t)rand()) & nn;
            p_ctx->pc += 2;
            break;
        case 0xd:
            // DXYN Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
            x = (ins & 0x0F00) >> 8;
            y = (ins & 0x00F0) >> 4;
            n = ins & 0x000F;

            p_ctx->reg[0xF] = 0;
            for (int i = 0; i < n; i++) {
                uint8_t spriteByte = p_ctx->heap[p_ctx->regi + i];
                for (int j = 0; j < 8; j++) {
                    if ((spriteByte & (0x80 >> j)) != 0) {
                        int idx = (p_ctx->reg[x] % SCREEN_WIDTH + j + ((p_ctx->reg[y] % SCREEN_HEIGHT + i) * SCREEN_WIDTH)) % (SCREEN_WIDTH * SCREEN_HEIGHT);
                        if (p_ctx->display[idx] == 1) {
                            p_ctx->reg[0xF] = 1;
                        }
                        p_ctx->display[idx] ^= 1;
                    }
                }
            }
            p_ctx->pc += 2;
            break;
        case 0xe:
            x = (ins & 0x0f00) >> 8;
            n = ins & 0x00ff;
            if (n == 0x9e) {
                // EX9E Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
                p_ctx->pc += IsKeyDown(p_ctx->reg[x] + 48) ? 4 : 2;
            } else {
                // EXA1	Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
                p_ctx->pc += !IsKeyDown(p_ctx->reg[x] + 48) ? 4 : 2;
            }
            break;
        case 0xf:
            x = (ins & 0x0f00) >> 8;
            n = ins & 0x00ff;
            switch (n) {
            case 0x15:
                // FX15	Set the delay timer to the value of register VX
                p_ctx->dt = p_ctx->reg[x];
                then = clock();
                break;
            case 0x07:
                // FX07	Store the current value of the delay timer in register VX
                p_ctx->reg[x] = p_ctx->dt;
                break;
            case 0x18:
                // FX18	Set the sound timer to the value of register VX
                p_ctx->st = p_ctx->reg[x];
                then = clock();
                break;
            case 0x0A:
                // FX0A	Wait for a keypress and store the result in register VX
                while (1) {
                    x = (ins & 0x0f00) >> 8;
                    uint8_t key = GetKeyPressed() - 48;
                    if (key <= 0xf) {
                        p_ctx->reg[x] = key;
                        break;
                    }
                    sleep(1);
                }
                break;
            case 0x1E:
                // FX1E	Add the value stored in register VX to register I
                p_ctx->regi += p_ctx->reg[x];
                break;
            case 0x29:
                // FX29	Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
                p_ctx->regi = CHIP8_HEAP_SIZE - sizeof(fonts) - 1 + p_ctx->reg[x] * 5;
                break;
            case 0x33:
                // FX33	Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I+1, and I+2
                p_ctx->heap[p_ctx->regi] = (p_ctx->reg[x] / 100);
                p_ctx->heap[p_ctx->regi + 1] = (p_ctx->reg[x] / 10) % 10;
                p_ctx->heap[p_ctx->regi + 2] = (p_ctx->reg[x] % 10);
                break;
            case 0x55:
                // FX55	Store the values of registers V0 to VX inclusive in memory starting at address I. I is set to I + X + 1 after operation
                memcpy(p_ctx->heap + p_ctx->regi, p_ctx->reg, x + 1);
                p_ctx->regi += x + 1;
                break;
            case 0x65:
                // FX65	Fill registers V0 to VX inclusive with the values stored in memory starting at address I. I is set to I + X + 1 after operation
                memcpy(p_ctx->reg, p_ctx->heap + p_ctx->regi, x + 1);
                p_ctx->regi += x + 1;
                break;
            default:
                LOGF("invalid instruction: 0x%X, pc: 0x%X\n", ins, p_ctx->pc);
                return;    
            }
            p_ctx->pc += 2;
            break;
        default:
            LOGF("invalid instruction: 0x%X, pc: 0x%X\n", ins, p_ctx->pc);
            return;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        
        for (int i = 0; i < SCREEN_WIDTH; i++) {
            for (int j = 0; j < SCREEN_HEIGHT; j++) {
                if (p_ctx->display[i + j * SCREEN_WIDTH] == 1) {
                    DrawRectangle(i * SCALE_FACTOR, j * SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR, WHITE);
                }
            }
        }
        EndDrawing();
    }
}
