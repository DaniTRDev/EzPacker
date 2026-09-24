#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <elf-file>\n", argv[0]);
        return 1;
    }

    const char *filePath = argv[1];
    FILE *f = fopen(filePath, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filePath);
        return 1;
    }

    uint8_t header[64];
    size_t bytesRead = fread(header, 1, sizeof(header), f);
    fclose(f);

    if (bytesRead < 64) {
        fprintf(stderr, "Error: file '%s' is too small (%zu bytes, expected at least 64)\n", filePath, bytesRead);
        return 1;
    }

    // Magic: 0x7F 'E' 'L' 'F'
    if (header[0] != 0x7F || header[1] != 'E' || header[2] != 'L' || header[3] != 'F') {
        fprintf(stderr, "Error: file '%s' invalid ELF magic (%02X %02X %02X %02X)\n",
                filePath, header[0], header[1], header[2], header[3]);
        return 1;
    }

    // Class: 2 = 64-bit
    if (header[4] != 2) {
        fprintf(stderr, "Error: file '%s' not 64-bit ELF (class = %d)\n", filePath, header[4]);
        return 1;
    }

    // Endianness: 1 = Little-endian
    if (header[5] != 1) {
        fprintf(stderr, "Error: file '%s' not little-endian (data = %d)\n", filePath, header[5]);
        return 1;
    }

    // Type: ET_REL = 1
    uint16_t e_type = 0;
    memcpy(&e_type, &header[16], 2);
    if (e_type != 1) {
        fprintf(stderr, "Error: file '%s' is not ET_REL (e_type = %u)\n", filePath, e_type);
        return 1;
    }

    // Machine: EM_X86_64 = 62
    uint16_t e_machine = 0;
    memcpy(&e_machine, &header[18], 2);
    if (e_machine != 62) {
        fprintf(stderr, "Error: file '%s' machine is not EM_X86_64 (e_machine = %u)\n", filePath, e_machine);
        return 1;
    }

    printf("[ELF OK] '%s' is a valid 64-bit x86-64 ELF relocatable object.\n", filePath);
    return 0;
}
