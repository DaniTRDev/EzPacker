# EzDSL: Compiler Backend Description Language Suite

**EzDSL** is a declarative Domain-Specific Language (DSL) suite engineered for compiler backends and code generators within the **EzPacker** toolchain. Inspired by modern compiler architectures (such as LLVM's TableGen and GlobalISel), EzDSL cleanly decouples target architecture definitions, hardware instruction encodings, calling conventions, legalization action matrices, IR-to-IR rewrite rules, and instruction selection patterns into specialized, human-readable sub-languages.

EzDSL provides a complete processing pipeline: Lexy-based zero-copy parsing, semantic validation passes, hierarchical symbol tables, ten C++ code generators (`CppMirTypeTableGenerator`, `CppMirInstructionGenerator`, `CppLegalizerGenerator`, `CppLegalizeRuleGenerator`, `CppTargetInstructionGenerator`, `CppEncodingTableGenerator`, `CppInstructionSelectorGenerator`, `CppCallingConvGenerator`, `CppRegisterInfoGenerator`, `CppTargetDescGenerator`), and a dedicated command-line compiler driver (`EzDslCli`).

---

## Table of Contents

1. [Language Suite Overview](#1-language-suite-overview)
2. [Common Lexical & Grammar Foundation](#2-common-lexical--grammar-foundation)
3. [Language Specifications](#3-language-specifications)
   - [Target Descriptor Definitions (`.tdesc`)](#target-descriptor-definitions-tdesc)
   - [Target Register Definitions](#target-register-definitions)
   - [Target Instruction Definitions (`.idf`)](#target-instruction-definitions-idf)
   - [Calling Convention Definitions (`.ezcc` / `.ccd`)](#calling-convention-definitions-ezcc--ccd)
   - [Generic IR Instruction Definitions (`.irdf`)](#generic-ir-instruction-definitions-irdf)
   - [Legalization Actions (`.lad`)](#legalization-actions-lad)
   - [Legalization Rewrite Rules (`.lrd`)](#legalization-rewrite-rules-lrd)
   - [Instruction Selection Patterns (`.isf`)](#instruction-selection-patterns-isf)
   - [Type Definitions (`.tyf`)](#type-definitions-tyf)
4. [Semantic Analysis & Symbol Table Architecture](#4-semantic-analysis--symbol-table-architecture)
5. [C++ Code Generators](#5-c-code-generators)
6. [CLI Driver (`EzDsl-cli`) & Options](#6-cli-driver-ezdsl-cli--options)
7. [CMake Build System Integration](#7-cmake-build-system-integration)
8. [Memory Architecture](#8-memory-architecture)

---

## 1. Language Suite Overview

| Sub-Language | Extension | Primary Domain | Generated Artifacts / Roles |
|:---|:---|:---|:---|
| **Target Descriptor** | `.tdesc` | Object formats, components, libcalls, register banks/classes and ABI defaults | `<Target>TargetDesc.h` / `.cpp` descriptor wiring, `<Target>RegisterInfo.h` |
| **Target Instruction Definition** | `.idf` | Operand constraints, mnemonics, flags and generic `ENCODING` blocks | `<Target>TargetInstructionTable.h/.cpp`, `<Target>EncodingTable.h` |
| **Calling Convention** | `.ezcc` / `.ccd` | Stack layout, preservation sets, ABI classification, argument/return rules | `<Target>CallingConvDesc.h` / `.cpp` ABI lowerers |
| **Generic IR Definition** | `.irdf` | Canonical IR opcode catalog, categories, flags | `MirInstructionSetDefs.h` (C++ MIR opcode enum & metadata) |
| **Legalization Action** | `.lad` | Type legality tables, promotions, scalar splits | `<Target>LegalizerActionTable.h` / `.cpp` |
| **Legalization Rule** | `.lrd` | IR-to-IR decomposition & pre-ISel rewrites | `<Target>LegalizerRules.h` / `.cpp` |
| **Instruction Selection** | `.isf` | Generic-to-target MIR mapping, addressing modes | `<Target>InstructionSelector.h` / `.cpp` |
| **Type Definition** | `.tyf` | Canonical IR types and bitwidths | `MirTypeTable.h` / `MirTypeTable.cpp` C++ class hierarchy |

---

## 2. Common Lexical & Grammar Foundation

All EzDSL sub-languages share a unified lexical foundation:

* **Comments**: C++ line comments (`// ...`) consumed up to newline.
* **Whitespace**: ASCII space, horizontal tabs, and newlines (`\r`, `\n`) act as token separators and are ignored outside string literals.
* **Identifiers**: Match `[a-zA-Z_][a-zA-Z0-9_]*`.
* **Literals**:
  - String Literals: Double-quoted strings (`"add $rd, $rs1, $rs2"`).
  - Integer Literals: Signed 64-bit integer values in Decimal (`42`, `-2048`), Hexadecimal (`0x1A2F`), Binary (`0b1010`), or Octal (`0o755`).
* **Source Tracking**: Every AST node wraps `DSL::Ast::Common::SourcedAstNode<T>`, binding zero-copy `SourceReference*` pointers for diagnostics.
* **Typed Identifiers**: Unified syntax across all declarations:
  $$\text{Type}(\text{Param})\text{:\$Name} \quad \text{or} \quad \text{Type:Name}$$
  - `GPR:rd`: Base register class `GPR`, identifier `rd`.
  - `simm(i12):imm12`: Immediate classifier `simm`, width parameter `i12`, identifier `imm12`.
  - `i32:$dst`: IR type `i32`, SSA register variable `$dst`.

---

## 3. Language Specifications

### Target Descriptor Definitions (`.tdesc`)
Declares the target root, its referenced definition files, sizes, object formats, register banks, classes, aliases, special registers, libcalls and component bindings:
```dsl
target X86_64 {
    instructions:  "x86_64_instructions.idf";
    calling_convs: ["x86_64_calling_conv.ezcc"];

    pointer_size: 8;
    stack_slot:   8;

    instruction_pointer: rip;
    mem_disp_type: i64;

    object_formats: [ELF, COFF];
    default_calling_conv: SysV_AMD64;

    register_bank GPR {
        classes { GPR8: 8, GPR16: 16, GPR32: 32, GPR64: 64 }
        sub_register { GPR16 <: GPR8, GPR32 <: GPR16, GPR64 <: GPR32 }
        registers {
            rax enc 0  names { rax: GPR64, eax: GPR32, ax: GPR16, al: GPR8 }
            rcx enc 1  names { rcx: GPR64, ecx: GPR32, cx: GPR16, cl: GPR8 }
        }
    }

    special {
        rip: 16
    }

    libcalls {
        __returnNothing: "__returnNothing";
        __divdi3:        "__divdi3"
    }

    components {
        frame_lowerer:        X86_64FrameLowerer;
        instruction_selector: X86_64TargetInstructionSelector
    }
}
```

### Target Register Definitions
Register banks, classes, sub-register alias hierarchies, and special registers are declared directly inside `.tdesc` files:
- **`classes`**: Defines register classes and their physical bit widths (`GPR64: 64`).
- **`sub_register`**: Declares wide-to-narrow aliasing relationships (`GPR64 <: GPR32, GPR32 <: GPR16`).
- **`registers`**: Enumerates physical registers with hardware encodings and per-class assembly names (`rax enc 0 names { rax: GPR64, eax: GPR32, ax: GPR16, al: GPR8 }`).
- **`special`**: Declares dedicated pseudo-registers (such as the instruction pointer `rip: 16`).

### Target Instruction Definitions (`.idf`)
Specifies machine instruction encodings, operand constraints, register directions, mnemonics, and side-effect flags:
```dsl
target AMD64;

target_inst ADD32rr(GPR32:dst OUT, GPR32:src1 IN, GPR32:src2 IN) {
    MNEMONIC("addl");
    FLAGS(IsCommutative);
    IMPLICIT_DEFS(EFLAGS);
};

target_inst ADD32rm(GPR32:dst OUT, GPR32:src IN, Mem32:addr IN) {
    MNEMONIC("addl");
    FLAGS(ReadsMemory);
    IMPLICIT_DEFS(EFLAGS);
};

target_inst MOV32mr(Mem32:addr OUT, GPR32:src IN) {
    MNEMONIC("movl");
    FLAGS(WritesMemory);
};
```

### Calling Convention Definitions (`.ezcc` / `.ccd`)
Declares complete ABI calling conventions, stack direction/cleanup, preservation sets, type classification, argument/return lowering, and struct-return (SRET) config:
```dsl
calling_conv SysV64 {
    STACK_ALIGN: 16;
    STACK_DIRECTION: DOWN;
    STACK_CLEANUP: CALLER;
    SHADOW_SPACE: 0;
    STACK_POINTER: GPR:rsp;
    FRAME_POINTER: GPR:rbp;

    CALLEE_SAVED: [GPR:rbx, GPR:rbp, GPR:r12, GPR:r13, GPR:r14, GPR:r15];
    CALLER_SAVED: [GPR:rax, GPR:rcx, GPR:rdx, GPR:rsi, GPR:rdi, GPR:r8, GPR:r9, GPR:r10, GPR:r11];

    CLASSIFY {
        TYPE(i1, i8, i16, i32, i64, ptr) -> INTEGER;
        TYPE(f32, f64) -> SSE;
        AGGREGATE {
            SIZE_LE(16) -> INTEGER;
            DEFAULT -> MEMORY;
        };
    };

    PASS {
        INTEGER -> REG_SEQ(GPR:rdi, GPR:rsi, GPR:rdx, GPR:rcx, GPR:r8, GPR:r9) STACK(ALIGN: 8);
        SSE -> REG_SEQ(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3, FPR:xmm4, FPR:xmm5, FPR:xmm6, FPR:xmm7) STACK(ALIGN: 8);
        MEMORY -> STACK(ALIGN: 8);
    };

    RETURN {
        INTEGER -> REG_SEQ(GPR:rax, GPR:rdx);
        SSE -> REG_SEQ(FPR:xmm0, FPR:xmm1);
        SRET_CONFIG(GPR:rdi, CONSUMES_ARG_SLOT: true, RETURN_IN: GPR:rax);
    };
};
```

### Generic IR Instruction Definitions (`.irdf`)
Declares high-level generic IR opcodes, categories, tiers, and dataflow directionality:
```dsl
inst ADD(Register:dst OUT, RegIntImm:lhs IN, RegIntImm:rhs IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
    FLAGS(IsCommutative);
}

inst LOAD(Register:dst OUT, AddressSource:addr IN) {
    CATEGORY(Memory);
    TIER(HighLevel);
    FLAGS(ReadsMemory);
}
```

### Legalization Actions (`.lad`)
Declares legality matrices for generic opcodes across types:
```dsl
action ADD {
    LEGAL(i32, i64, f32, f64);
    WIDENS(i1, i8, i16) >> i32;
    NARROWS(i128) >> i64;
};

action SEXT {
    LEGAL(i32:0, i8:1);
    LEGAL(i64:0, i32:1);
    WIDENS(i1:1) >> i8;
};
```

### Legalization Rewrite Rules (`.lrd`)
Pattern-based IR-to-IR decomposition prior to instruction selection:
```dsl
rule NarrowAddi64 {
    match {
        ADD i64:$dst, i64:$lhs, i64:$rhs;
    };
    when {
        isSubtarget32Bit();
    };
    expand {
        UNMERGE_VALUES i32:$lhs_lo, i32:$lhs_hi, i64:$lhs;
        UNMERGE_VALUES i32:$rhs_lo, i32:$rhs_hi, i64:$rhs;
        UADDO i32:$dst_lo, i1:$carry, i32:$lhs_lo, i32:$rhs_lo;
        UADDE i32:$dst_hi, i1:$carry_out, i32:$lhs_hi, i32:$rhs_hi, i1:$carry;
        MERGE_VALUES i64:$dst, i32:$dst_lo, i32:$dst_hi;
    };
};
```

### Instruction Selection Patterns (`.isf`)
Declares complex multi-variant addressing modes (`addrmode`) and generic-to-target selection patterns (`pattern`):
```dsl
target AMD64;

addrmode AddrModeRegImm(GPR64:base, simm32:disp = 0) {
    variant BaseDisp {
        match {
            ADD ptr:$base, imm(i32):$disp;
        };
        when {
            isSimm32($disp);
        };
    };
    variant BaseOnly {
        match {
            ptr:$base;
        };
    };
};

pattern Select_ADD64rm [cost = 2] {
    match {
        ADD i64:$dst, i64:$src1, (LOAD i64:$tmp, AddrModeRegImm($base, $disp));
    };
    when {
        hasOneUse($tmp);
        noInterveningStore($tmp);
    };
    select {
        ADD64rm GPR64:$dst, GPR64:$src1, [$base, $disp];
    };
};

pattern Select_ADD64ri [cost = 1] {
    match {
        ADD i64:$dst, i64:$src1, imm(i64):$imm;
    };
    when {
        isSimm32($imm);
    };
    select {
        ADD64ri GPR64:$dst, GPR64:$src1, $imm;
    };
};

pattern Select_ADD64rr [cost = 1] {
    match {
        ADD i64:$dst, i64:$src1, i64:$src2;
    };
    select {
        ADD64rr GPR64:$dst, GPR64:$src1, GPR64:$src2;
    };
};
```

### Type Definitions (`.tyf`)
Declares canonical MIR types:
```dsl
integer i1(1);
integer i8(8);
integer i16(16);
integer i32(32);
integer i64(64);
float f32(32);
float f64(64);
void void;
bindingToken __bindToken;
```

---

## 4. Semantic Analysis & Symbol Table Architecture

EzDSL features a dedicated semantic validation pipeline (`EzDsl/Sema/include/Sema/`, `EzDsl/Sema/include/SemaPasses/`):

- **`SymbolTable` & `Scope`**: Hierarchical lexical symbol table allocating through `std::pmr::memory_resource`. Manages typed `Symbol` instances across all sub-languages.
- **`TypePass`**: Ingests `TypeDefFile` ASTs, registers interned types, validates bitwidths, and populates the symbol table.
- **`IrInstructionPass`**: Ingests `IrInstDefFile` ASTs, verifies operand counts, category invariants, and directionality rules (`IN`, `OUT`, `INOUT`).
- **`CallingConvPass`**: Validates stack alignment/growth, caller/callee preservation sets, ABI classification rules, and argument/return placement declarations.
- **`TargetDescPass`**: Ingests `.tdesc` manifests, validating pointer/stack sizes, register banks, classes, sub-register alias hierarchies, bit sizes, hardware encodings, special registers, instruction pointer references, default calling convention references, object formats, libcalls and component slots.
- **`LegalizeActionPass`**: Ingests `.lad` ASTs, validates legality matrices, type constraints, and widening/narrowing targets.
- **`LegalizeRulePass`**: Ingests `.lrd` ASTs, checks SSA variable scoping between match and emit templates, and validates guard predicates.
- **`TargetInstPass`**: Ingests `.idf` ASTs, verifies operand names/directions, operand class constraints, instruction flags, implicit register defs/uses, and generic `ENCODING` blocks against the selected dialect.
- **`InstructionSelectPass`**: Ingests `.isf` ASTs, verifies addressing mode declarations and variants, pattern tree structures and nesting, predicate references, and target instructions emitted in `select` blocks.

---

## 5. C++ Code Generators

EzDSL translates verified AST and symbol table models into production C++ source and header files (`EzDsl/CodeGenerators/`):

- **`CppMirTypeTableGenerator`**: Synthesizes `MirTypeTable.h` and `MirTypeTable.cpp`. Generates direct accessor methods (`i32()`, `f64()`, `getPtr()`, `getArray()`, `getClass()`), memory-interning structures, and layout initialization logic via `IMirTargetTypeLayout`.
- **`CppMirInstructionGenerator`**: Synthesizes `MirInstructionSetDefs.h`. Emits `INSTRUCTION(name, tier, category, operands, flags)` macro tables defining opcodes, instruction categories, and operand validation metadata.
- **`CppLegalizerGenerator`** / **`CppLegalizeRuleGenerator`**: Synthesize the target legalizer action matrix and the IR-to-IR rewrite rule tables.
- **`CppTargetInstructionGenerator`**: Synthesizes `<Target>TargetInstructionTable.h` and `<Target>TargetInstructionTable.cpp`. Generates the target opcode enumeration, static instruction descriptor table (`MirTargetInstructionDesc[]`) with operand classes, directionality, latency, execution flags, and implicit registers, and exposes `create<Target>TargetInstructionTable(std::pmr::memory_resource*)`.
- **`CppEncodingTableGenerator`**: Synthesizes `<Target>EncodingTable.h`. Emits the declarative machine-encoding table consumed by the code emitter.
- **`CppInstructionSelectorGenerator`**: Synthesizes `<Target>InstructionSelector.h` and `<Target>InstructionSelector.cpp`. Generates a target-specialized `MirInstructionSelector` subclass that embodies Maximal Munch pattern matching tables, tree pattern predicates, addressing mode matching routines, and target instruction emission lowering.
- **`CppCallingConvGenerator`** / **`CppRegisterInfoGenerator`** / **`CppTargetDescGenerator`**: Synthesize the calling-convention descriptors, register bank/class metadata and the target descriptor that ties the generated components together.

---

## 6. CLI Driver (`EzDsl-cli`) & Options

`EzDslCli` is the standalone executable driver used to process EzDSL backend definitions during build time.

### CLI Usage:
```bash
EzDslCli [options] -i <input_file>
```

### Auto-Discovery by File Extension:
| File Extension | Default Language Dialect | Default Code Generator |
|:---|:---|:---|
| `.tyf` | `TypeDef` | `TypeTable` (`CppMirTypeTableGenerator`) |
| `.irdf` | `IrInstDef` | `Instructions` (`CppMirInstructionGenerator`) |
| `.lad` | `LegalizeAction` | `Legalizer` (`CppLegalizerGenerator`) |
| `.lrd` | `LegalizeRule` | `Rules` (`CppLegalizeRuleGenerator`) |
| `.idf` | `TargetInstDef` | `TargetInstructions` (`CppTargetInstructionGenerator`) |
| `.isf` | `InstructionSelect` | `InstructionSelector` (`CppInstructionSelectorGenerator`) |
| `.ezcc` / `.ccd` | `CallingConv` | `CallingConv` (`CppCallingConvGenerator`) |
| `.tdesc` | `TargetDesc` | `TargetDesc` (`CppTargetDescGenerator`) |

`--emit-target-encodings` additionally synthesizes `<Target>EncodingTable.h` from the `ENCODING` blocks of an `.idf` input.
`--emit-registers` synthesizes `<Target>RegisterInfo.h` from the register declarations of a `.tdesc` input.

### Available Options:
| Flag | Description |
|:---|:---|
| `-i, --input <file>` | Input EzDSL definition file (`.tyf`, `.irdf`, `.lad`, `.lrd`, `.idf`, `.isf`, `.ezcc`, `.ccd`, `.tdesc`). |
| `-o, --output <path>` | Output destination directory or file path (default: `.`). |
| `-I, --include <dir>` | Directory to search for imported DSL files (repeatable). |
| `--target <name>` | Target architecture name (e.g. `AMD64`, `AArch64`). |
| `--generator <gen>` | Explicit generator override: `type-table`, `instructions`, `legalizer`, `rules`, `target-instructions`, `target-encodings`, `instruction-selector`, `calling-conv`, `registers`, `target-desc`, or `auto`. |
| `--emit-type-table` | Synthesize EzMir `MirTypeTable.h` and `MirTypeTable.cpp`. |
| `--emit-instructions` | Synthesize EzMir `MirInstructionSetDefs.h`. |
| `--emit-legalizer` | Synthesize Target `LegalizerActionTable.h` and `.cpp`. |
| `--emit-rules` | Synthesize Target `LegalizerRules.h` and `.cpp`. |
| `--emit-target-instructions` | Synthesize Target `TargetInstructionTable.h` and `.cpp`. |
| `--emit-target-encodings` | Synthesize Target `EncodingTable.h` from `.idf` `ENCODING` blocks. |
| `--emit-instruction-selector` | Synthesize Target `InstructionSelector.h` and `.cpp`. |
| `--emit-calling-conv` | Synthesize Target `CallingConvDesc.h` and `.cpp`. |
| `--emit-registers` | Synthesize Target `RegisterInfo.h` from `.tdesc`. |
| `--emit-target-desc` | Synthesize Target `TargetDesc.h` and `.cpp`. |
| `--rules <file>` | Companion `.lrd` rewrite rules for a `.lad` legalizer run. |
| `--types <file>` | Dependency `.tyf` type definition file. |
| `--instructions <file>` | Dependency `.irdf` instruction definition file. |
| `--header-only` | Synthesize only the `.h` header file. |
| `--source-only` | Synthesize only the `.cpp` translation unit. |
| `--check-only` | Perform syntax and semantic validation without code generation. |
| `--dry-run` | Validate and compute outputs without writing to disk. |
| `--dump-info` | Dump file metadata, dialect and construct counts. |
| `--dump-ast` | Dump the parsed AST to stdout. |
| `--dump-symbols` | Dump the populated symbol table to stdout. |
| `--dump-files` | Dump the list of expected/generated output files. |
| `--format <fmt>` | Dump serialization format: `text` (default) or `json`. |
| `-v, --verbose` | Enable verbose diagnostic trace/debug logs. |
| `-q, --quiet` | Suppress non-essential console output. |
| `--version` | Display the tool version and exit. |
| `-h, --help` | Display command-line options. |

---

## 7. CMake Build System Integration

EzDSL integrates cleanly into CMake build workflows via helper modules in `EzMir/CMake/` and `EzTriple/CMake/`:

```cmake
# Generate C++ MirTypeTable class from types.tyf
EzDslGenerateTypeTable(
    TARGET EzMir
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/types.tyf
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/Type
)

# Generate C++ MIR Instruction Set Definitions from instructions.irdf
EzDslGenMirInstructions(
    TARGET EzMir
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/instructions.irdf
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/Instruction
)

# Generate C++ Target Instruction Table (and EncodingTable) from instructions.idf
EzDslGenTargetInstructions(
    TARGET EzTriple
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/AMD64Instructions.idf
    TARGET_NAME AMD64
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64
)

# Generate the legalizer action table (and optional companion rules)
EzDslGenLegalizerActionTable(
    TARGET EzTriple
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/AMD64LegalizerActions.lad
    RULES ${CMAKE_CURRENT_SOURCE_DIR}/AMD64LegalizerRules.lrd
    TARGET_NAME AMD64
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64
)

# Generate C++ Instruction Selector from patterns.isf
EzDslGenInstructionSelector(
    TARGET EzTriple
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/AMD64Patterns.isf
    TARGET_NAME AMD64
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64
)

# Generate C++ CallingConvDesc from a calling convention definition
EzDslGenCallingConv(
    TARGET EzTriple
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/SysV_AMD64.ezcc
    TARGET_NAME AMD64
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64
)

# Generate C++ RegisterInfo from target descriptor
EzDslGenRegisterInfo(
    TARGET EzTargetsX86_64
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64.tdesc
    TARGET_NAME x86_64
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64
)
```

---

## 8. Memory Architecture

EzDSL is built on `std::pmr::monotonic_buffer_resource` arenas:
- **Zero-allocation ASTs**: AST collections, literals, and symbol tables share a contiguous memory block.
- **Fast teardown**: Releasing the top-level arena instantly frees all AST memory without individual node deallocations.
