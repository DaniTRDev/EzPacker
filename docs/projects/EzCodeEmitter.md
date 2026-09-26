# EzCodeEmitter Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzCodeEmitter.md) > **EzCodeEmitter** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzCodeEmitter` is EzPacker's binary machine code emission and object file container serialization library. It bridges target-lowered `MirInstruction` sequences and concrete executable object files on disk (`.o` and `.obj`).

It provides:
1. **Dynamic Linked Code Sections (`CodeSection`)**: A node-based stream representation that allows non-linear code insertion, forward label references, and deferred alignment resolution.
2. **Contextual State Tracking (`CodeEmitterContext`)**: Management of code labels, relocations, section cursors, and scratch memory across modules.
3. **Generic Target Interface (`GenericCodeEmitter`)**: The abstract contract every target machine code emitter implements.
4. **Relocatable Object File Writers (`IObjectWriter`)**: Production implementations for **ELF64** (System V Linux/BSD) and **PE-COFF** (Microsoft Windows).

```
  Lowered MIR Functions
          |
          v
  +-----------------------------------------------------------+
  |              GenericCodeEmitter::emitInst()               |
  | (Interprets TargetInstructionDesc and emits machine bytes)|
  +-----------------------------------------------------------+
          |
          v
  +-----------------------------------------------------------+
  |                 CodeSection Linked Nodes                  |
  |  [Data Chunk] <-> [Label Marker] <-> [Align Directive]    |
  +-----------------------------------------------------------+
          |
          v (finalize() flattens nodes & resolves label offsets)
  +-----------------------------------------------------------+
  |                   CodeEmitterContext                      |
  |    Aggregates: Flattened Buffers, Labels & Relocations     |
  +-----------------------------------------------------------+
          |
          v
  +-----------------------------------------------------------+
  |             IObjectWriter (Elf64Writer / CoffWriter)      |
  |  - Formats Section Headers, Symbol Tables & Relocations   |
  |  - Emits .o (ELF64) or .obj (PE-COFF) Binary Byte Stream  |
  +-----------------------------------------------------------+
```

---

## 2. Dynamic Linked Code Sections (`CodeSection.h`)

Object emission often requires writing data non-linearly: jumping back to patch jump offsets, creating alignment padding whose final byte length is only known after all preceding basic blocks are assembled, or referencing forward labels.

To solve this, `CodeSection` models the byte stream as a doubly-linked list of `SectionNode` elements:

```cpp
enum class SectionNodeKind : uint8_t
{
    Data,  // Chunk of emitted raw bytes
    Label, // Label marker / bookmark
    Align  // Dynamic alignment directive
};

struct SectionNode
{
    SectionNodeKind m_kind;
    SectionNode *m_prev{ nullptr };
    SectionNode *m_next{ nullptr };

    // Payload for Data
    std::pmr::vector<uint8_t> m_data;

    // Payload for Label
    MirId m_labelId{ MIRID_INVALID };
    uint64_t m_calculatedOffset{ 0 };

    // Payload for Align
    size_t m_alignment{ 1 };
    uint8_t m_padByte{ 0 };
};
```

### 2.1 Section Categorization (`SectionType`)

```cpp
enum class SectionType : uint8_t
{
    Text,             // Executable machine code (".text")
    ReadOnly,         // Read-only constants without relocations (".rodata")
    ReadOnlyWithRel,  // Read-only data requiring relocations (".data.rel.ro")
    CString,          // Null-terminated string literals
    Const4,           // 4-byte scalar constants
    Const8,           // 8-byte scalar constants
    Const16AndBigger, // 16-byte-or-larger constants (SIMD)
    Data,             // Mutable initialized data (".data")
    DataWithRel,      // Mutable data requiring relocations
    NonInitialized,   // Zero-initialized unallocated storage (".bss")
    Custom,           // Target-specific metadata/exception tables
    Undefined         // External symbol reference
};
```

### 2.2 `CodeSection` Operations

```cpp
class CodeSection
{
public:
    CodeSection(SectionFlags flags,
                SectionType type,
                size_t alignment,
                TargetEndianness endianness,
                uint8_t padByte,
                std::string_view name,
                std::pmr::memory_resource *alloc);

    // Emitting data into active cursor block
    void emit8(uint8_t val);
    void emit16(uint16_t val);
    void emit32(uint32_t val);
    void emit64(uint64_t val);
    void emitBytes(const uint8_t *data, size_t size);
    void emitBytesWithEndian(const uint8_t *data, size_t size, TargetEndianness inputEndianness);

    // Node insertion & cursor management
    SectionNode *bindLabel(MirId labelId);
    void alignTo(size_t alignment);
    void setCursor(SectionNode *node);
    void resetCursorToEnd();

    // Flattening and post-finalize patching
    void finalize(); // Flattens all nodes into m_buffer, computing label offsets
    bool patch32(uint64_t offset, uint32_t val);
    bool patch64(uint64_t offset, uint64_t val);

    uint64_t getCurrentOffset() const;
};
```

---

## 3. Emission Context & Relocations (`CodeEmitterContext.h`)

`CodeEmitterContext` manages the lifetime of labels and relocations during code generation across all modules and sections:

### 3.1 Relocation Fixup Types (`TargetCodeRelocationType`)
```cpp
enum class TargetCodeRelocationType : uint8_t
{
    None,
    Absolute32,  // Direct 32-bit absolute address fixup
    Absolute64,  // Direct 64-bit absolute address fixup
    PCRel32,     // 32-bit signed PC-relative (RIP-relative) data displacement
    BranchRel32, // 32-bit signed PC-relative branch or call offset
    GOTPCREL,    // 32-bit PC-relative reference to Global Offset Table entry
    PLTRel32     // 32-bit PC-relative reference to Procedure Linkage Table entry
};
```

### 3.2 Label & Relocation Records
```cpp
struct CodeLabel
{
    CodeSection *m_definingSection{ nullptr };
    SectionNode *m_node{ nullptr };
    MirId m_id{ MIRID_INVALID };
    uint64_t m_currentOffset{ 0 };
    uint64_t m_labelAddress{ 0 };
    std::string_view m_name{};

    uint64_t getAddress() const { return m_node ? m_node->m_calculatedOffset : m_labelAddress; }
};

struct CodeRelocation
{
    TargetCodeRelocationType m_relocType{ TargetCodeRelocationType::None };
    CodeSection *m_definingSection{ nullptr };
    MirReference *m_srcRef{ nullptr }; // Originating MIR reference
    uint64_t m_address{ 0 };          // Offset from start of section
};
```

### 3.3 `CodeEmitterContext` API
```cpp
class CodeEmitterContext
{
public:
    CodeEmitterContext(DiagnosticCollector *diagCollector,
                       const std::pmr::unordered_map<SectionType, CodeSection *> &sections,
                       std::pmr::memory_resource *alloc);
    ~CodeEmitterContext();

    CodeLabel *getOrCreateLabel(CodeSection *definingSection, MirId id, std::string_view name);
    CodeLabel *getCurrentLabel() const;
    void bindLabel(CodeLabel *label);

    CodeRelocation *addReloc(MirReference *srcRef, TargetCodeRelocationType relocType);
    CodeRelocation *addRelocAt(MirReference *srcRef, TargetCodeRelocationType relocType, uint64_t address);

    CodeSection *getCurrentSection() const;
    CodeSection *getSection(SectionType type) const;

    void resetFuncState(MirFunction *currentFunc);
    CodeLabel *findLabel(MirId id) const;

    const std::pmr::unordered_map<CodeSection *, std::pmr::vector<CodeRelocation *>> &getRelocations() const;
};
```

---

## 4. Generic Target Code Emitter (`GenericCodeEmitter.h`)

The abstract interface implemented by each target architecture (e.g. `X86_64CodeEmitter`):

```cpp
class GenericCodeEmitter
{
public:
    virtual ~GenericCodeEmitter() = default;

    virtual void beginFunction(CodeEmitterContext *ctx, std::string_view name) = 0;
    virtual void beginFunction(CodeEmitterContext *ctx, MirFunction *func);

    virtual void bindLabel(MirId labelId) = 0;

    virtual void endFunction(CodeEmitterContext *ctx) = 0;
    virtual void endFunction(CodeEmitterContext *ctx, MirFunction *func);

    virtual void emitInst(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands) = 0;
};
```

---

## 5. Relocatable Object File Writers (`IObjectWriter.h`)

EzCodeEmitter serializes in-memory code sections into industry-standard binary object files:

```cpp
namespace EzCodeEmitter::ObjectFormat
{

struct ObjectSymbol
{
    std::string_view m_name;
    SectionType m_section{ SectionType::Text };
    uint64_t m_offset{ 0 };
    uint64_t m_size{ 0 };
    bool m_isGlobal{ true };
    bool m_isWeak{ false };
    bool m_isFunction{ false };
};

// Symbol binding and section resolution semantics:
// - `m_isWeak`: Emits `STB_WEAK` in ELF64.
// - `m_isGlobal`: Emits `STB_GLOBAL` in ELF64 (when not weak) or `COFF_SYM_CLASS_EXTERNAL` in COFF.
// - Local (`!m_isGlobal`): Emits `STB_LOCAL` in ELF64 or `COFF_SYM_CLASS_STATIC` in COFF.
// - Undefined symbols (`SectionType::Undefined`): Emits `SHN_UNDEF` (section 0) in ELF64 and section number 0 in COFF for external function declarations.

struct ObjectRelocEntry
{
    SectionType m_section{ SectionType::Text };
    uint64_t m_offset{ 0 };
    std::string_view m_symbolName;
    TargetCodeRelocationType m_type{ TargetCodeRelocationType::None };
    int64_t m_addend{ 0 };
};

class IObjectWriter
{
public:
    virtual ~IObjectWriter() = default;

    virtual void addSymbol(const ObjectSymbol &sym) = 0;
    virtual void addRelocation(const ObjectRelocEntry &reloc) = 0;
    virtual std::vector<uint8_t> write(const std::pmr::unordered_map<SectionType, CodeSection *> &sections) = 0;
    virtual void clear() = 0;
};

}
```

### 5.1 `Elf64Writer` (`ObjectFormat/Elf64Writer.h`)
- Generates 64-bit relocatable ELF object files (`ET_REL`).
- Builds ELF section headers (`.text`, `.rodata`, `.data`, `.bss`, `.symtab`, `.strtab`, `.rela.text`).
- Maps `TargetCodeRelocationType` into AMD64 ELF relocation types (`R_X86_64_64`, `R_X86_64_32`, `R_X86_64_PC32`, `R_X86_64_PLT32`, `R_X86_64_GOTPCREL`).

### 5.2 `CoffWriter` (`ObjectFormat/CoffWriter.h`)
- Generates Microsoft Common Object File Format (`PE-COFF`) relocatable object files.
- Builds COFF section headers (`.text$mn`, `.rdata`, `.data`).
- Maps `TargetCodeRelocationType` into AMD64 COFF relocation types (`IMAGE_REL_AMD64_ADDR64`, `IMAGE_REL_AMD64_ADDR32NB`, `IMAGE_REL_AMD64_REL32`).

---

## 6. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| Sections | `EzCodeEmitter/include/CodeSection.h` | `CodeSection`, `SectionNode`, `SectionNodeKind`, `SectionType`, `SectionFlags`, `TargetEndianness` |
| Context | `EzCodeEmitter/include/CodeEmitterContext.h` | `CodeEmitterContext`, `CodeLabel`, `CodeRelocation`, `TargetCodeRelocationType` |
| Interface | `EzCodeEmitter/include/GenericCodeEmitter.h` | `GenericCodeEmitter` |
| Results | `EzCodeEmitter/include/Encoding/EncodeResult.h` | `EncodeResult` |
| Object Writer | `EzCodeEmitter/include/ObjectFormat/IObjectWriter.h` | `IObjectWriter` |
| Object Writer | `EzCodeEmitter/include/ObjectFormat/ObjectSymbol.h` | `ObjectSymbol`, `ObjectRelocEntry` |
| Object Writer | `EzCodeEmitter/include/ObjectFormat/Elf64Writer.h` | `Elf64Writer` |
| Object Writer | `EzCodeEmitter/include/ObjectFormat/CoffWriter.h` | `CoffWriter` |
