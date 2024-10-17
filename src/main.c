#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "log.h"
#include "vm/chip8.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        LOG(
            "usage chip8 <input file>"
        );
        return EPERM;
    }

    const char *file_name = argv[1];

    FILE *file = fopen(file_name, "rb");

    if (!file) {
        LOGF("can't open file %s\n", file_name);
        return ENOENT;
    }

    fseek(file, 0, SEEK_END);
    int64_t size = ftell(file);

    if (size > CHIP8_HEAP_SIZE - 0x200) {
        LOG("file too large");
        fclose(file);
        return EFBIG;
    }

    Chip8 *ctx = (Chip8*)malloc(sizeof(Chip8));
    if (!ctx) {
        LOG("can't allocate memory");
        fclose(file);
        return ENOMEM;
    }
    chip8_init(ctx);

    fseek(file, 0, SEEK_SET);
    if (!fread(ctx->heap + 0x200, size, 1, file)) {
        LOG("can't read file");
        free(ctx);
        fclose(file);
        return EPERM;
    }
    fclose(file);

    chip8_loop(ctx);

    chip8_fini(ctx);
    free(ctx);
    return 0;
}
