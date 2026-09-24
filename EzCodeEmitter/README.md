# EzCodeEmitter: Machine Code Emission & Object File Serialization

[`EzCodeEmitter`](file:///E:/Repos/EzPacker/EzCodeEmitter) is the low-level machine code emission, section layout, and binary packaging engine of the **EzPacker** compiler backend. It abstracts binary serialization across diverse industry object file formats (**Windows PE/COFF** and **System V ELF64**) while delivering a non-linear, node-based emission architecture capable of late label binding, alignment padding, relocation fixups, and post-finalization binary patching.

---

## Architecture Overview

```
                         ┌────────────────────────────────────────┐
                         │           GenericCodeEmitter           │
                         │       (Target Backend / Emitter)       │
                         └───────────────────┬────────────────────┘
                                             │
                         ┌───────────────────▼────────────────────┐
                         │           CodeEmitterContext           │
                         │  - Function-Local State & Scratch      │
                         │  - O(1) Label Registry (CodeLabel)     │
                         │  - Relocation Accumulator              │
                         └───────────────────┬────────────────────┘
                                             │
             ┌───────────────────────────────┼───────────────────────────────┐
             ▼                               ▼                               ▼
    ┌─────────────────┐             ┌─────────────────┐             ┌─────────────────┐
    │   CodeSection   │             │   CodeSection   │             │   CodeSection   │
    │     (.text)     │             │    (.rodata)    │             │     (.data)     │
    └────────┬────────┘             └────────┬────────┘             └────────┬────────┘
             │                               │                               │
             ▼                               ▼                               ▼
    ┌─────────────────────────────────────────────────────────────────────────────────┐
    │                           Doubly-Linked Section Nodes                           │
    │   [DataNode: Bytes] <---> [LabelNode: LabelId] <---> [AlignNode: Alignment=16]  │
    └────────────────────────────────────────┬────────────────────────────────────────┘
                                             │
                                             ▼ finalize()
    ┌─────────────────────────────────────────────────────────────────────────────────┐
    │                       Contiguous Linearized Output Buffer                       │
    │                         std::span<const uint8_t> data                           │
    └────────────────────────────────────────┬────────────────────────────────────────┘
                                             │
                                             ▼
                         ┌────────────────────────────────────────┐
                         │         IObjectWriter Interface        │
                         │     (Symbol Tables, Relocations)       │
                         └───────────────┬────────┬───────────────┘
                                         │        │
                         ┌───────────────┘        └───────────────┐
                         ▼                                        ▼
             ┌──────────────────────┐                 ┌──────────────────────┐
             │     Elf64Writer      │                 │      CoffWriter      │
             │   (Linux .o Files)   │                 │  (Windows .obj Files)│
             └──────────────────────┘                 └──────────────────────┘
```

---

## Core Subsystems & Design

### 1. Doubly-Linked Section Node Stream (`EzCodeEmitter/include/CodeSection.h`)

Unlike conventional compilers that emit instructions into contiguous flat byte buffers and require costly memmove operations or multi-pass branch estimation, `EzCodeEmitter` structures sections as intrusive doubly-linked streams of [`SectionNode`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L61) objects:

- **[`SectionNodeKind::Data`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L53)**: Contains raw machine code bytes (`std::pmr::vector<uint8_t>`).
- **[`SectionNodeKind::Label`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L54)**: Represents a symbolic branch target or symbol anchor (`MirId m_labelId`). Holds calculated byte offset resolved during linearization.
- **[`SectionNodeKind::Align`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L55)**: Dynamic alignment directive with target-specific padding bytes (`0x90` NOP for executable code, `0x00` for read-only or mutable data).

#### Key Benefits of Node-Based Streams:
1. **$O(1)$ Arbitrary Insertion**: Jump tables, constant pools, or cold code blocks can be spliced into the stream at any point without shifting subsequent instructions.
2. **Out-of-Order Block Emission**: Functions and basic blocks can be emitted in arbitrary order; cross-block branch targets are anchored to label nodes and resolved later.
3. **Late Alignment Resolution**: NOP/zero padding bytes are computed accurately only when flattening the stream, ensuring minimal instruction padding.

---

### 2. Section & Emission Context (`EzCodeEmitter/include/CodeSection.h`, `CodeEmitterContext.h`)

* **[`CodeSection`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h)**:
  - Models a binary object section categorized by [`SectionType`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L9):
    - `Text`: Executable machine code (`.text`).
    - `ReadOnly`: Read-only constants without relocations (`.rodata`).
    - `ReadOnlyWithRel`: Read-only data requiring link-time relocations (`.data.rel.ro`).
    - `CString`: Null-terminated string literals.
    - `Const4`, `Const8`, `Const16AndBigger`: Scalar and vector constants.
    - `Data`: Mutable initialized data (`.data`).
    - `NonInitialized`: Zero-initialized BSS (`.bss`, `SHT_NOBITS`).
  - Permission flags via [`SectionFlags`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L28) (`m_readable`, `m_writable`, `m_executable`).
  - **Emission Primitives**:
    - [`emit8()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`emit16()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`emit32()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`emit64()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h): Emits scalars formatted to target endianness ([`TargetEndianness::Little`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L40) vs `Big`).
    - [`emitBytesWithEndian()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h): Emits arbitrary byte spans.
    - [`bindLabel(MirId labelId)`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L131): Inserts a label marker node at the active cursor position.
    - [`alignTo(size_t alignment)`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h#L136): Appends a dynamic alignment directive.
  - **Linearization & Binary Patching**:
    - [`finalize()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h): Linearizes all linked nodes into a continuous `std::pmr::vector<uint8_t>`, computing absolute offsets for all bound labels and computing alignment padding bytes.
    - Post-finalization patching: [`patch8()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`patch16()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`patch32()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`patch64()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), and [`patchBytesWithEndian()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h) allow modifying serialized bytes at arbitrary offsets (used by branch relaxation and relocation fixups).
    - Accessors: [`getData()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`getSize()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h), [`getLabelOffset(MirId labelId)`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeSection.h).

* **[`CodeEmitterContext`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/CodeEmitterContext.h)**:
  - Coordinates multi-section emission across a compilation unit.
  - Tracks the active section and label cursor.
  - Manages function-local labels through `CodeLabel` and maintains the function-to-module label registry.
  - Accumulates relocation descriptors via `addReloc()` tied to source `MirReference` objects and section offsets.
  - Supports clean function boundaries with `resetFuncState(MirFunction*)`.

---

### 3. Relocation Model (`EzCodeEmitter/include/EzCodeEmitterCommon.h`)

Relocations describe link-time fixups required when code references external symbols or addresses unknown at compile time:

* **[`TargetCodeRelocationType`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/EzCodeEmitterCommon.h)**:
  - `None`: Sentinel.
  - `Absolute32`: Direct 32-bit absolute address fixup.
  - `Absolute64`: Direct 64-bit absolute address fixup (e.g. constant pointer in `.rodata`).
  - `PCRel32`: 32-bit PC-relative displacement (`target - PC`).
  - `BranchRel32`: 32-bit relative displacement for branch/call instructions.
  - `GOTPCREL`: 32-bit PC-relative displacement to the Global Offset Table entry.
  - `PLTRel32`: 32-bit PC-relative displacement to Procedure Linkage Table stub.
* **[`ObjectRelocEntry`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/ObjectSymbol.h#L34)**:
  - `m_section`: Target section containing the field to patch.
  - `m_offset`: Byte offset of the field in that section.
  - `m_symbolName`: Referenced target symbol name.
  - `m_type`: [`TargetCodeRelocationType`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/EzCodeEmitterCommon.h).
  - `m_addend`: Relocation addend (used for explicit System V ELF `.rela` records).

---

### 4. Object Format Writers (`EzCodeEmitter/include/ObjectFormat/`)

The object format subsystem packages finalized code sections, symbol tables, and relocation tables into binary files compliant with standard OS linkers.

* **[`IObjectWriter`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/IObjectWriter.h)**:
  - Format-independent interface for serializing sections, symbols, and relocations:
    - [`addSymbol(const ObjectSymbol &sym)`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/IObjectWriter.h#L27): Registers exported, internal, or external symbols.
    - [`addRelocation(const ObjectRelocEntry &reloc)`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/IObjectWriter.h#L32): Registers a link-time relocation.
    - [`write(sections)`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/IObjectWriter.h#L37): Linearizes and returns the complete binary byte stream (`std::vector<uint8_t>`).
    - [`clear()`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/IObjectWriter.h#L42): Resets internal symbol and relocation accumulators for reuse.

* **[`Elf64Writer`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/Elf64Writer.h)**:
  - Generates System V AMD64 ELF64 relocatable object files (`.o`).
  - Formats:
    - Standard `Elf64_Ehdr` file header (`ET_REL`, `EM_X86_64`).
    - Section headers (`Elf64_Shdr`) for `.text`, `.rodata`, `.data`, `.bss`, and custom sections.
    - Symbol tables: `.symtab` (with local/global bindings, `STT_FUNC`, `STT_OBJECT`, `STT_NOTYPE`) and associated `.strtab`.
    - Relocation tables: `.rela.text` containing `Elf64_Rela` entries with explicit addends (`R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_PLT32`, `R_X86_64_GOTPCREL`).
    - Validated against standard Linux linkers (`ld`, `gcc`, `clang`) and `readelf`.

* **[`CoffWriter`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/CoffWriter.h)**:
  - Generates Microsoft Windows PE/COFF relocatable object files (`.obj`).
  - Formats:
    - `IMAGE_FILE_HEADER` (`IMAGE_FILE_MACHINE_AMD64`).
    - `IMAGE_SECTION_HEADER` array for `.text`, `.rdata`, `.data`, `.bss`.
    - COFF symbol table with auxiliary records and trailing COFF string table for long identifiers.
    - Section relocations (`IMAGE_REL_AMD64_ADDR64`, `IMAGE_REL_AMD64_REL32`, `IMAGE_REL_AMD64_ADDR32NB`).
    - Validated against Microsoft linkers (`link.exe`, `lld-link`) and `dumpbin`.

* **[`ObjectSymbol`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/ObjectFormat/ObjectSymbol.h#L19)**:
  - Unified symbol descriptor holding `m_name` (`std::string_view`), `m_section`, `m_offset`, `m_size`, `m_isGlobal`, and `m_isFunction`.

* **Section Factory Helpers (`EzCodeEmitter/include/Helpers.h`)**:
  - `CreateElfSections(sections, alloc)`: Populates canonical ELF layout (`.text`, `.rodata`, `.data.rel.ro`, `.data`, `.bss`).
  - `CreateCoffSections(sections, alloc)`: Populates Windows PE/COFF layout (`.text`, `.rdata`, `.data`, `.bss`, `.pdata`).
  - `CreateMachoSections(sections, alloc)`: Populates Mach-O layout (`__TEXT/__text`, `__TEXT/__const`, `__DATA/__data`, `__DATA/__bss`).

---

### 5. Target Integration Seam (`EzCodeEmitter/include/GenericCodeEmitter.h`)

* **[`GenericCodeEmitter`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/GenericCodeEmitter.h)**:
  - Abstract base interface implemented by architecture-specific machine code encoders (e.g. `X86_64CodeEmitter` in `EzTargets`):
    - `virtual void beginFunction(CodeEmitterContext *ctx, std::string_view name) = 0`
    - `virtual void bindLabel(MirId labelId) = 0`
    - `virtual EncodeResult emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands) = 0`
    - `virtual void endFunction(CodeEmitterContext *ctx) = 0`
* **[`EncodeResult`](file:///E:/Repos/EzPacker/EzCodeEmitter/include/Encoding/EncodeResult.h)**:
  - Lightweight descriptor returning emission status, byte count, and error context.

---

## Working Code Examples

### Complete ELF/COFF Object Generation

```cpp
#include "CodeSection.h"
#include "CodeEmitterContext.h"
#include "Helpers.h"
#include "ObjectFormat/Elf64Writer.h"
#include "ObjectFormat/CoffWriter.h"
#include <fstream>
#include <iostream>

void generateObjectFile(bool emitElf)
{
    std::pmr::synchronized_pool_resource pool;
    DiagnosticCollector diags;

    // 1. Initialize sections according to requested format
    std::pmr::unordered_map<SectionType, CodeSection *> sections(&pool);
    if (emitElf)
    {
        Helpers::ObjectFormat::CreateElfSections(sections, &pool);
    }
    else
    {
        Helpers::ObjectFormat::CreateCoffSections(sections, &pool);
    }

    CodeSection *textSec = sections[SectionType::Text];
    CodeEmitterContext ctx(&diags, sections, &pool);

    // 2. Emit machine code:
    // func add42:
    //   mov eax, 42   (B8 2A 00 00 00)
    //   ret           (C3)
    CodeLabel *funcLabel = ctx.getOrCreateLabel(textSec, 1, "add42");
    ctx.bindLabel(funcLabel);

    textSec->emit8(0xB8);
    textSec->emit32(42);
    textSec->emit8(0xC3);

    // Insert alignment directive before next symbol
    textSec->alignTo(16);

    // 3. Finalize sections (computes offsets & linearizes streams)
    textSec->finalize();

    // 4. Configure Object Writer
    std::unique_ptr<EzCodeEmitter::ObjectFormat::IObjectWriter> writer;
    if (emitElf)
    {
        writer = std::make_unique<EzCodeEmitter::ObjectFormat::Elf64Writer>();
    }
    else
    {
        writer = std::make_unique<EzCodeEmitter::ObjectFormat::CoffWriter>();
    }

    // Register exported function symbol
    EzCodeEmitter::ObjectFormat::ObjectSymbol sym;
    sym.m_name = "add42";
    sym.m_section = SectionType::Text;
    sym.m_offset = 0;
    sym.m_size = textSec->getSize();
    sym.m_isGlobal = true;
    sym.m_isFunction = true;
    writer->addSymbol(sym);

    // 5. Serialize binary object file
    std::vector<uint8_t> objectBytes = writer->write(sections);

    std::string outPath = emitElf ? "add42.o" : "add42.obj";
    std::ofstream out(outPath, std::ios::binary);
    out.write(reinterpret_cast<const char *>(objectBytes.data()), objectBytes.size());

    std::cout << "Successfully wrote " << objectBytes.size() << " bytes to " << outPath << "\n";
}
```

---

## Testing & CMake Integration

### Linking EzCodeEmitter
```cmake
target_link_libraries(YourTarget PRIVATE EzCodeEmitter EzMir EzCore)
target_include_directories(YourTarget PRIVATE ${EZPACKER_ROOT}/EzCodeEmitter/include)
```

### Subproject Test Suite
The tests for `EzCodeEmitter` reside in [`tests/EzCodeEmitterTestSuite/tests/`](file:///E:/Repos/EzPacker/tests/EzCodeEmitterTestSuite/tests/):
- [`T_ObjectFormatWriters.cpp`](file:///E:/Repos/EzPacker/tests/EzCodeEmitterTestSuite/tests/T_ObjectFormatWriters.cpp): Verifies ELF64 and PE/COFF header validation, section header tables, symbol tables, and relocations.
- [`T_X86_64CodeEmitter.cpp`](file:///E:/Repos/EzPacker/tests/EzCodeEmitterTestSuite/tests/T_X86_64CodeEmitter.cpp): Validates machine instruction emission across x86-64 ALU, memory, and control-flow instructions.
- [`T_X86_64Encoding.cpp`](file:///E:/Repos/EzPacker/tests/EzCodeEmitterTestSuite/tests/T_X86_64Encoding.cpp): Comprehensive bit-level validation of ModR/M, REX, SIB, and immediate machine code encodings.
- [`T_BranchRelaxation.cpp`](file:///E:/Repos/EzPacker/tests/EzCodeEmitterTestSuite/tests/T_BranchRelaxation.cpp): Tests short-to-near conditional branch relaxation and post-finalization binary patching.
- [`T_StressAndSanitizers.cpp`](file:///E:/Repos/EzPacker/tests/EzCodeEmitterTestSuite/tests/T_StressAndSanitizers.cpp): Stress-tests large section allocations, deep label hierarchies, and address sanitizer checks.
