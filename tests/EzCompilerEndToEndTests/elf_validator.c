#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ELF64 standard definitions
#define EI_NIDENT       16
#define ELFMAG0         0x7F
#define ELFMAG1         'E'
#define ELFMAG2         'L'
#define ELFMAG3         'F'

#define ELFCLASS64      2
#define ELFDATA2LSB     1
#define EV_CURRENT      1
#define ET_REL          1
#define EM_X86_64       62

#define SHT_NULL        0
#define SHT_PROGBITS    1
#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define SHT_RELA        4
#define SHT_NOBITS      8

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STB_WEAK        2

#define STT_NOTYPE      0
#define STT_OBJECT      1
#define STT_FUNC        2
#define STT_SECTION     3
#define STT_FILE        4

#define ELF64_ST_BIND(val)   (((unsigned char)(val)) >> 4)
#define ELF64_ST_TYPE(val)   ((val) & 0xF)

#pragma pack(push, 1)
typedef struct {
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;
#pragma pack(pop)

static const char *get_bind_name(uint8_t bind) {
    switch (bind) {
        case STB_LOCAL:  return "LOCAL";
        case STB_GLOBAL: return "GLOBAL";
        case STB_WEAK:   return "WEAK";
        default:         return "UNKNOWN";
    }
}

static const char *get_type_name(uint8_t type) {
    switch (type) {
        case STT_NOTYPE:  return "NOTYPE";
        case STT_OBJECT:  return "OBJECT";
        case STT_FUNC:    return "FUNC";
        case STT_SECTION: return "SECTION";
        case STT_FILE:    return "FILE";
        default:          return "UNKNOWN";
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [--dump] <elf-file>\n", argv[0]);
        return 1;
    }

    int dump = 0;
    const char *filePath = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--dump") == 0 || strcmp(argv[i], "-v") == 0) {
            dump = 1;
        } else {
            filePath = argv[i];
        }
    }

    if (!filePath) {
        fprintf(stderr, "Error: no input file specified.\n");
        return 1;
    }

    FILE *f = fopen(filePath, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filePath);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fileSize < (long)sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "Error: file '%s' too small (%ld bytes, expected at least %zu)\n",
                filePath, fileSize, sizeof(Elf64_Ehdr));
        fclose(f);
        return 1;
    }

    uint8_t *buffer = (uint8_t *)malloc((size_t)fileSize);
    if (!buffer) {
        fprintf(stderr, "Error: memory allocation failure\n");
        fclose(f);
        return 1;
    }

    if (fread(buffer, 1, (size_t)fileSize, f) != (size_t)fileSize) {
        fprintf(stderr, "Error: failed to read file '%s'\n", filePath);
        free(buffer);
        fclose(f);
        return 1;
    }
    fclose(f);

    // 1. Validate ELF Header
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *)buffer;
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
        fprintf(stderr, "Error: invalid ELF magic (%02X %02X %02X %02X)\n",
                ehdr->e_ident[0], ehdr->e_ident[1], ehdr->e_ident[2], ehdr->e_ident[3]);
        free(buffer);
        return 1;
    }

    if (ehdr->e_ident[4] != ELFCLASS64) {
        fprintf(stderr, "Error: not 64-bit ELF (class=%d)\n", ehdr->e_ident[4]);
        free(buffer);
        return 1;
    }

    if (ehdr->e_ident[5] != ELFDATA2LSB) {
        fprintf(stderr, "Error: not little-endian ELF (data=%d)\n", ehdr->e_ident[5]);
        free(buffer);
        return 1;
    }

    if (ehdr->e_type != ET_REL) {
        fprintf(stderr, "Error: not ET_REL relocatable object (e_type=%u)\n", ehdr->e_type);
        free(buffer);
        return 1;
    }

    if (ehdr->e_machine != EM_X86_64) {
        fprintf(stderr, "Error: not EM_X86_64 machine (e_machine=%u)\n", ehdr->e_machine);
        free(buffer);
        return 1;
    }

    if (ehdr->e_shentsize != sizeof(Elf64_Shdr)) {
        fprintf(stderr, "Error: invalid section header entry size (%u, expected %zu)\n",
                ehdr->e_shentsize, sizeof(Elf64_Shdr));
        free(buffer);
        return 1;
    }

    if (ehdr->e_shoff + ehdr->e_shnum * sizeof(Elf64_Shdr) > (size_t)fileSize) {
        fprintf(stderr, "Error: section header table out of bounds\n");
        free(buffer);
        return 1;
    }

    // 2. Validate Sections and Section String Table
    const Elf64_Shdr *sections = (const Elf64_Shdr *)(buffer + ehdr->e_shoff);
    const char *shstrtab = NULL;
    if (ehdr->e_shstrndx < ehdr->e_shnum) {
        const Elf64_Shdr *shstr_shdr = &sections[ehdr->e_shstrndx];
        if (shstr_shdr->sh_type == SHT_STRTAB &&
            shstr_shdr->sh_offset + shstr_shdr->sh_size <= (size_t)fileSize) {
            shstrtab = (const char *)(buffer + shstr_shdr->sh_offset);
        }
    }

    if (!shstrtab) {
        fprintf(stderr, "Error: missing or invalid .shstrtab section\n");
        free(buffer);
        return 1;
    }

    const Elf64_Shdr *symtab_shdr = NULL;
    const Elf64_Shdr *strtab_shdr = NULL;

    for (uint16_t i = 0; i < ehdr->e_shnum; ++i) {
        const Elf64_Shdr *sh = &sections[i];
        if (sh->sh_type != SHT_NOBITS && sh->sh_offset + sh->sh_size > (size_t)fileSize) {
            fprintf(stderr, "Error: section %u offset + size exceeds file bounds\n", i);
            free(buffer);
            return 1;
        }

        if (sh->sh_type == SHT_SYMTAB) {
            symtab_shdr = sh;
        } else if (sh->sh_type == SHT_STRTAB && i != ehdr->e_shstrndx) {
            strtab_shdr = sh;
        }

        if (dump) {
            const char *secName = (sh->sh_name < sections[ehdr->e_shstrndx].sh_size)
                ? (shstrtab + sh->sh_name) : "<invalid>";
            printf("  [Sec %2u] %-16s type=0x%02x flags=0x%02lx off=0x%06lx size=0x%06lx\n",
                   i, secName, sh->sh_type, (unsigned long)sh->sh_flags,
                   (unsigned long)sh->sh_offset, (unsigned long)sh->sh_size);
        }
    }

    // 3. Validate Symbol Table (.symtab)
    if (symtab_shdr) {
        if (symtab_shdr->sh_entsize != sizeof(Elf64_Sym)) {
            fprintf(stderr, "Error: invalid symtab entry size (%lu, expected %zu)\n",
                    (unsigned long)symtab_shdr->sh_entsize, sizeof(Elf64_Sym));
            free(buffer);
            return 1;
        }

        size_t numSymbols = symtab_shdr->sh_size / sizeof(Elf64_Sym);
        const Elf64_Sym *syms = (const Elf64_Sym *)(buffer + symtab_shdr->sh_offset);
        const char *strtab = NULL;

        if (symtab_shdr->sh_link < ehdr->e_shnum) {
            const Elf64_Shdr *linked_str = &sections[symtab_shdr->sh_link];
            if (linked_str->sh_type == SHT_STRTAB) {
                strtab = (const char *)(buffer + linked_str->sh_offset);
            }
        }
        if (!strtab && strtab_shdr) {
            strtab = (const char *)(buffer + strtab_shdr->sh_offset);
        }

        uint32_t firstNonLocal = symtab_shdr->sh_info;
        if (firstNonLocal > numSymbols) {
            fprintf(stderr, "Error: .symtab sh_info (%u) exceeds symbol count (%zu)\n",
                    firstNonLocal, numSymbols);
            free(buffer);
            return 1;
        }

        // Verify ELF invariant: all symbols with index < sh_info must be STB_LOCAL,
        // and all symbols with index >= sh_info must NOT be STB_LOCAL.
        uint32_t localCount = 0;
        uint32_t nonLocalCount = 0;

        for (size_t i = 0; i < numSymbols; ++i) {
            const Elf64_Sym *sym = &syms[i];
            uint8_t bind = ELF64_ST_BIND(sym->st_info);
            uint8_t type = ELF64_ST_TYPE(sym->st_info);

            const char *symName = "";
            if (strtab && sym->st_name < sections[symtab_shdr->sh_link].sh_size) {
                symName = strtab + sym->st_name;
            }

            if (i < firstNonLocal) {
                if (i > 0 && bind != STB_LOCAL) {
                    fprintf(stderr, "Error: symbol %zu ('%s') at index < sh_info (%u) has non-local binding %s\n",
                            i, symName, firstNonLocal, get_bind_name(bind));
                    free(buffer);
                    return 1;
                }
                if (i > 0) localCount++;
            } else {
                if (bind == STB_LOCAL) {
                    fprintf(stderr, "Error: local symbol %zu ('%s') found at index >= sh_info (%u)\n",
                            i, symName, firstNonLocal);
                    free(buffer);
                    return 1;
                }
                nonLocalCount++;
            }

            if (dump && i > 0) {
                printf("  [Sym %2zu] %-20s bind=%-6s type=%-7s shndx=%u val=0x%lx size=%lu\n",
                       i, symName, get_bind_name(bind), get_type_name(type),
                       sym->st_shndx, (unsigned long)sym->st_value, (unsigned long)sym->st_size);
            }
        }

        printf("[ELF Parser OK] '%s': 64-bit x86-64 ELF relocatable object (%u sections, %zu symbols: %u local, %u global/weak, symtab sh_info=%u valid).\n",
               filePath, ehdr->e_shnum, numSymbols, localCount, nonLocalCount, firstNonLocal);
    } else {
        printf("[ELF Parser OK] '%s': 64-bit x86-64 ELF relocatable object (%u sections, no symtab).\n",
               filePath, ehdr->e_shnum);
    }

    free(buffer);
    return 0;
}
