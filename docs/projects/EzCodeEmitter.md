# EzCodeEmitter Subproject Documentation

[EzPacker Documentation Index](../index.md) > **EzCodeEmitter**

---

## 1. Overview & Architectural Role

`EzCodeEmitter` is the binary serialization and machine code generation library of EzPacker. Once the compiler backend has lowered, selected, and register-allocated machine instructions, `EzCodeEmitter` translates these instructions into raw binary byte streams, manages object sections, tracks internal/external relocations, and writes valid relocatable object files conforming to industry-standard formats:
- **ELF64** (`.o`) for Linux, FreeBSD, and macOS.
- **PE/COFF** (`.obj`) for Microsoft Windows.

```
       +-------------------------------------------------------------+
       |                  Lowered Machine Instructions               |
       +-------------------------------------------------------------+
                                      |
                                      v
                       +-----------------------------+
                       |     GenericCodeEmitter      |
                       |  (e.g., X86_64CodeEmitter)  |
                       +-----------------------------+
                                      |
       +------------------------------+------------------------------+
       |                                                             |
       v                                                             v
+-------------------------------+                     +-------------------------------+
|          CodeSection          |                     |       ObjectRelocEntry        |
|  - Text (.text)               |                     |  - Offset within section      |
|  - Data (.data)               |                     |  - Target symbol              |
|  - Rodata (.rodata)           |                     |  - Relocation Type (PLT32..)  |
|  - Bss (.bss)                 |                     |  - Addend                     |
+-------------------------------+                     +-------------------------------+
       |                                                             |
       +------------------------------+------------------------------+
                                      |
                                      v
                       +-----------------------------+
                       |        IObjectWriter        |
                       +-----------------------------+
                                      |
                     +----------------+----------------+
                     |                                 |
                     v                                 v
          +--------------------+             +--------------------+
          |    Elf64Writer     |             |     CoffWriter     |
          |  (System V ELF64)  |             | (Microsoft PE/COFF)|
          +--------------------+             +--------------------+
                     |                                 |
                     v                                 v
               output.o (Linux)                output.obj (Windows)
```

---

## 2. Core Abstractions & Classes

### 2.1 `GenericCodeEmitter` (`include/GenericCodeEmitter.h`)
The target-agnostic interface through which compilation engines drive code emission. Target-specific emitters (e.g. `X86_64CodeEmitter` in `EzTargets`) derive from this class.

- `beginFunction(CodeEmitterContext *ctx, MirFunction *func)`: Initializes emission for a function, setting up label tracking and starting the function symbol.
- `bindLabel(MirId labelId)`: Binds a basic block label ID to the current byte offset in `.text`, resolving pending forward jumps.
- `emitInstruction(CodeEmitterContext *ctx, MirInstruction *inst)`: Encodes a single target instruction into raw bytes and appends it to the active code section.
- `endFunction(CodeEmitterContext *ctx, MirFunction *func)`: Finalizes function emission, calculates function size, and marks the function symbol boundary.
- `finalize(CodeEmitterContext *ctx)`: Runs post-emission passes such as branch relaxation (converting 8-bit short jumps to 32-bit near jumps when necessary).

### 2.2 `CodeEmitterContext` (`include/CodeEmitterContext.h`)
The working state container passed through the emission pipeline:
- Holds the target memory resource.
- Owns the map of active sections (`std::pmr::unordered_map<SectionType, CodeSection*>`).
- Tracks defined labels and their linear byte offsets.
- Collects `ObjectSymbol` definitions and `ObjectRelocEntry` records.

### 2.3 `CodeSection` (`include/CodeSection.h`)
Encapsulates an individual binary section:
- **`SectionType`**: `Text` (executable code), `Data` (initialized writable data), `Rodata` (read-only constants and string literals), `Bss` (uninitialized zeroed data).
- **Data Buffer**: Contiguous byte buffer (`std::pmr::vector<uint8_t>`).
- **Alignment**: Section alignment requirement in bytes (e.g. 16-byte alignment).
- **Relocations**: List of relocation entries targeting this section.

### 2.4 `ObjectSymbol` (`include/ObjectFormat/ObjectSymbol.h`)
Defines a symbol exported to or required by the object file:
- `m_name`: Symbol name string.
- `m_binding`: `Local`, `Global`, or `Weak`.
- `m_type`: `Function`, `Object`, or `Section`.
- `m_section`: Target section index.
- `m_value`: Byte offset within the section.
- `m_size`: Size of the symbol in bytes.

### 2.5 `ObjectRelocEntry` (`include/ObjectFormat/ObjectSymbol.h`)
Describes a relocation fixup to be resolved by the system linker:
- `m_section`: The section containing the location that needs patching.
- `m_offset`: Byte offset from the start of the section to the relocation site.
- `m_symbolName`: Symbol referenced by the relocation.
- `m_type`: Target-specific relocation opcode (e.g. `R_X86_64_PLT32` or `IMAGE_REL_AMD64_REL32`).
- `m_addend`: Constant offset added to the symbol value.

---

## 3. Object File Writers (`IObjectWriter`)

EzPacker defines a format-agnostic interface `IObjectWriter` (`include/ObjectFormat/IObjectWriter.h`) so the compiler driver can write either format without modifying emission logic:

```cpp
class IObjectWriter {
  public:
    virtual ~IObjectWriter() = default;
    virtual void addSymbol(const ObjectSymbol &sym) = 0;
    virtual void addRelocation(const ObjectRelocEntry &reloc) = 0;
    virtual std::vector<uint8_t> write(const std::pmr::unordered_map<SectionType, CodeSection*> &sections) = 0;
    virtual void clear() = 0;
};
```

### 3.1 `Elf64Writer` (`include/ObjectFormat/Elf64Writer.h`)
Emits standard relocatable 64-bit ELF files (`.o`) conforming to the System V Application Binary Interface:
- **ELF Header (`Elf64_Ehdr`)**: Sets machine type (`EM_X86_64`), file type (`ET_REL`), entry point (0), and section header offsets.
- **Section Headers (`Elf64_Shdr`)**: Declares `.text`, `.data`, `.rodata`, `.bss`, `.symtab`, `.strtab`, `.shstrtab`, and `.rela.text`.
- **String Tables**: Emits `.shstrtab` (section names) and `.strtab` (symbol names).
- **Symbol Table (`.symtab`)**: Serializes `Elf64_Sym` entries, correctly separating local symbols from global symbols as mandated by ELF standards.
- **Relocation Sections (`.rela.text`)**: Serializes `Elf64_Rela` entries with explicit 64-bit addends (`r_offset`, `r_info`, `r_addend`). Supports `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_PLT32`, `R_X86_64_GOTPCREL`.

### 3.2 `CoffWriter` (`include/ObjectFormat/CoffWriter.h`)
Emits Microsoft PE/COFF relocatable object files (`.obj`) compatible with Microsoft `link.exe`, LLVM `lld-link`, and MinGW GCC:
- **COFF File Header (`IMAGE_FILE_HEADER`)**: Sets machine architecture (`IMAGE_FILE_MACHINE_AMD64`), number of sections, timestamp, and pointer to symbol table.
- **Section Headers (`IMAGE_SECTION_HEADER`)**: Declares `.text`, `.data`, `.rdata`, `.bss` with flags (`IMAGE_SCN_CNT_CODE`, `IMAGE_SCN_MEM_EXECUTE`, `IMAGE_SCN_MEM_READ`, `IMAGE_SCN_ALIGN_16BYTES`).
- **Symbol Table**: Emits standard 18-byte `IMAGE_SYMBOL` records. Short names (<= 8 chars) are stored inline; longer names are stored as byte offsets into the trailing string table.
- **COFF Relocations**: Emits `IMAGE_RELOCATION` records (`r_vaddr`, `r_symndx`, `r_type`) supporting `IMAGE_REL_AMD64_ADDR64`, `IMAGE_REL_AMD64_REL32`, `IMAGE_REL_AMD64_ADDR32NB`.

---

## 4. Usage Example: Serializing an Object File

```cpp
#include "ObjectFormat/Elf64Writer.h"
#include "ObjectFormat/CoffWriter.h"
#include "CodeSection.h"
#include <fstream>

// Create sections
std::pmr::monotonic_buffer_resource arena;
CodeSection textSection(SectionType::Text, &arena);

// Emit raw x86_64 instructions: mov eax, 42; ret
const uint8_t code[] = { 0xb8, 0x2a, 0x00, 0x00, 0x00, 0xc3 };
textSection.appendBytes(code, sizeof(code));

std::pmr::unordered_map<SectionType, CodeSection*> sections(&arena);
sections[SectionType::Text] = &textSection;

// Configure ELF64 Writer
EzCodeEmitter::ObjectFormat::Elf64Writer writer;
ObjectSymbol sym{
    .m_name = "get_answer",
    .m_binding = SymbolBinding::Global,
    .m_type = SymbolType::Function,
    .m_section = 1, // .text
    .m_value = 0,
    .m_size = sizeof(code)
};
writer.addSymbol(sym);

// Serialize to binary vector
std::vector<uint8_t> objectBytes = writer.write(sections);

// Write to file
std::ofstream out("answer.o", std::ios::binary);
out.write(reinterpret_cast<const char*>(objectBytes.data()), objectBytes.size());
```

---

## 5. API Reference & Further Reading

- Generated Doxygen API documentation: [Doxygen Documentation Index](../doxygen/index.html)
- Next subproject: [EzTriple Subproject Documentation](EzTriple.md)
- Return to [EzPacker Landing Page](../index.md)
