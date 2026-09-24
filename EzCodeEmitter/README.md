# EzCodeEmitter: Binary Code Emission & Object Section Architecture

`EzCodeEmitter` is the low-level machine code emission, section layout, and binary patching engine of the **EzPacker** compiler backend. It abstracts binary serialization across diverse object file formats (**Windows PE/COFF**, **System V ELF**, and **Apple Mach-O**) while providing a non-linear, node-based emission model capable of late label binding, alignment padding, and relocation fixups.

---

## Key Features

1. **Doubly-Linked Node Section Stream**:
   - Implements non-linear code emission via linked stream nodes (`SectionNodeKind::Data`, `SectionNodeKind::Label`, `SectionNodeKind::Align`).
   - Allows out-of-order insertion, jump table generation, and data embedding before final linearization.
2. **Late Serialization & Binary Patching**:
   - `finalize()` flattens the node graph into a contiguous byte buffer, computing precise label offsets and resolving dynamic alignment padding in a single pass.
   - Safe post-finalization binary patching methods (`patch32()`, `patch64()`, `patchBytesWithEndian()`) for branch displacements and relocation fixups.
3. **Object Format Section Helpers**:
   - Pre-configured factory helpers generating standard sections compliant with **COFF** (`CreateCoffSections`), **ELF** (`CreateElfSections`), and **Mach-O** (`CreateMachoSections`).
   - Handles text, constants, read-only data, relocatable data, uninitialized BSS (`SHT_NOBITS`), and unwind tables.
4. **Target Relocation Management**:
   - Relocation descriptors (`CodeRelocation`) supporting standard link-time relocation types:
     - `Absolute32`, `Absolute64`
     - `PCRel32`, `BranchRel32` (RIP/PC-relative data and branch fixups)
     - `GOTPCREL`, `PLTRel32` (Global Offset Table and Procedure Linkage Table calls)
5. **Configurable Endianness Support**:
   - Native little-endian and big-endian emission with automatic byte-swapping (`emit16`, `emit32`, `emit64`, `emitBytesWithEndian`).
6. **PMR Memory Management**:
   - Built on `std::pmr::memory_resource` for fast arena allocations and deterministic teardown without heap fragmentation.

---

## Architecture Overview

```
                         ┌─────────────────────────────┐
                         │     GenericCodeEmitter      │
                         │ (Target Backend / Emitter)  │
                         └──────────────┬──────────────┘
                                        │
                         ┌──────────────▼──────────────┐
                         │     CodeEmitterContext      │
                         │  - Current Bound Label      │
                         │  - Relocation Registry      │
                         │  - Function-Local Scratch   │
                         └──────────────┬──────────────┘
                                        │
           ┌────────────────────────────┼────────────────────────────┐
           ▼                            ▼                            ▼
  ┌─────────────────┐          ┌─────────────────┐          ┌─────────────────┐
  │   CodeSection   │          │   CodeSection   │          │   CodeSection   │
  │     (.text)     │          │    (.rodata)    │          │     (.data)     │
  └────────┬────────┘          └────────┬────────┘          └────────┬────────┘
           │                            │                            │
           ▼                            ▼                            ▼
  ┌───────────────────────────────────────────────────────────────────────────┐
  │                        Doubly-Linked Section Nodes                        │
  │ [DataNode: Bytes] <---> [LabelNode: LabelId] <---> [AlignNode: Align=16]  │
  └─────────────────────────────────────┬─────────────────────────────────────┘
                                        │
                                        ▼ finalize()
  ┌───────────────────────────────────────────────────────────────────────────┐
  │                    Contiguous Linearized Output Buffer                    │
  │                      std::span<const uint8_t> data                        │
  └───────────────────────────────────────────────────────────────────────────┘
```

---

## Core Classes & Concepts

### 1. `CodeSection` (`EzCodeEmitter/include/CodeSection.h`)
Represents an individual binary section. Features:
- **`emit8` / `emit16` / `emit32` / `emit64`**: Emits scalars formatted to target endianness.
- **`bindLabel(MirId labelId)`**: Inserts a label marker node at the active cursor position.
- **`alignTo(size_t alignment)`**: Inserts a dynamic alignment directive with target padding bytes (`0x90` NOP for text, `0x00` for data).
- **`finalize()`**: Linearizes all nodes into a continuous `std::pmr::vector<uint8_t>`, computing absolute offsets for all bound labels.
- **`patch32` / `patch64`**: Modifies serialized bytes at arbitrary offsets post-finalization.

### 2. `CodeEmitterContext` (`EzCodeEmitter/include/CodeEmitterContext.h`)
Coordinates multi-section emission across a compilation unit:
- Tracks the active section and label cursor.
- Manages function-local labels and relocations, flushing them to module-wide tables via `resetFuncState(MirFunction*)`.
- Provides O(1) label lookup and creation (`getOrCreateLabel()`).
- Collects relocations (`addReloc()`) tied to source `MirReference` objects and relocation fixup types.

### 3. Object Format Presets (`EzCodeEmitter/include/Helpers.h`)
Factory functions in `namespace Helpers::ObjectFormat`:
- **`CreateCoffSections`**: Sets up Windows PE/COFF sections (`.text`, `.rdata`, `.data`, `.bss`, `.pdata`).
- **`CreateElfSections`**: Sets up Linux/System V ELF sections (`.text`, `.rodata`, `.data.rel.ro`, `.data`, `.bss`, `.custom`).
- **`CreateMachoSections`**: Sets up Apple Mach-O segments and sections (`__TEXT/__text`, `__TEXT/__const`, `__TEXT/__cstring`, `__DATA/__data`, `__DATA/__bss`).

### 4. `GenericCodeEmitter` (`EzCodeEmitter/include/GenericCodeEmitter.h`)
The abstract base interface implemented by architecture-specific machine code emitters (e.g. x86_64, AArch64, RISC-V):
- `beginFunction(CodeEmitterContext *ctx, std::string_view name)`
- `bindLabel(MirId labelId)`
- `emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands)`
- `endFunction(CodeEmitterContext *ctx)`

---

## Usage Example

```cpp
#include "CodeSection.h"
#include "CodeEmitterContext.h"
#include "Helpers.h"
#include <iostream>

int main()
{
    std::pmr::synchronized_pool_resource pool;
    DiagnosticCollector diagCollector;

    // 1. Initialize Canonical ELF Section Layout
    std::pmr::unordered_map<SectionType, CodeSection *> sections(&pool);
    Helpers::ObjectFormat::CreateElfSections(sections, &pool);

    CodeSection *textSection = sections[SectionType::Text];
    CodeSection *rodataSection = sections[SectionType::ReadOnly];

    // 2. Initialize Code Emission Context
    CodeEmitterContext context(&diagCollector, sections, &pool);

    // 3. Emit x86_64 Machine Code:
    // mov eax, [rip + string_offset]
    // ret
    CodeLabel *entryLabel = context.getOrCreateLabel(textSection, 1, "main");
    context.bindLabel(entryLabel);

    // x86_64: mov eax, 42 -> B8 2A 00 00 00
    textSection->emit8(0xB8);
    textSection->emit32(42);

    // x86_64: ret -> C3
    textSection->emit8(0xC3);

    // Insert alignment directive before next symbol
    textSection->alignTo(16);

    // 4. Finalize Sections (Computes label offsets & linearizes buffers)
    textSection->finalize();
    rodataSection->finalize();

    // 5. Inspect Emitted Output
    std::span<const uint8_t> textBytes = textSection->getData();
    std::cout << "Emitted " << textBytes.size() << " bytes into .text\n";
    for (uint8_t byte : textBytes)
    {
        std::cout << std::hex << (int)byte << " ";
    }
    std::cout << "\n";

    return 0;
}
```

---

## Building and Linking

`EzCodeEmitter` is built as a static CMake library.

```cmake
target_link_libraries(YourTarget PRIVATE EzCodeEmitter EzMir EzCore)
target_include_directories(YourTarget PRIVATE ${EZPACKER_ROOT}/EzCodeEmitter/include)
```
