# EzDSL: Compiler Backend Description Language Suite

**EzDSL** is a declarative Domain-Specific Language (DSL) suite engineered for compiler backends and code generators within the **EzPacker** toolchain. Inspired by modern compiler architectures (such as LLVM's TableGen and GlobalISel), EzDSL cleanly decouples target architecture definitions, hardware instruction encodings, calling conventions, legalization action matrices, IR-to-IR rewrite rules, and instruction selection patterns into specialized, human-readable sub-languages.

EzDSL provides a complete processing pipeline: Lexy-based zero-copy parsing, semantic validation passes, hierarchical symbol tables, C++ code generators (`CppMirTypeTableGenerator`, `CppMirInstructionGenerator`), and a dedicated command-line compiler driver (`EzDsl-cli`).

---

## Table of Contents

1. [Language Suite Overview](#1-language-suite-overview)
2. [Common Lexical & Grammar Foundation](#2-common-lexical--grammar-foundation)
3. [Language Specifications](#3-language-specifications)
   - [Target Definitions (`.tdf`)](#target-definitions-tdf)
   - [Target Instruction Definitions (`.idf`)](#target-instruction-definitions-idf)
   - [Calling Convention Definitions (`.ccdf` / `.cdf`)](#calling-convention-definitions-ccdf--cdf)
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
| **Target Definition** | `.tdf` | Target ISA, File Inclusions, Register Hierarchies | Hardware register trees, register banks, register classes |
| **Instruction Definition** | `.idf` | Formats, Binary Encoding, Assembly, Latencies | Target machine instruction metadata, encoding tables |
| **Calling Convention** | `.ccdf` / `.cdf` | Stack Layout, Preservation Sets, ABI Classification, Calling Conventions | ABI lowering descriptors, argument placement, return rules, SRET handling |
| **Generic IR Definition** | `.irdf` | Canonical IR Opcode Catalog, Categories, Flags | `MirInstructionSetDefs.h` (C++ MIR opcode enum & metadata) |
| **Legalization Action** | `.lad` | Type Legality Tables, Promotions, Scalar Splits | Legality action matrices (`LEGAL`, `WIDENS`, `NARROWS`, etc.) |
| **Legalization Rule** | `.lrd` | IR-to-IR Decomposition & Pre-ISel Rewrites | Subtarget expansion & arithmetic lowering transforms |
| **Instruction Selection** | `.isf` | Generic-to-Target MIR Mapping, Addressing Modes | Multi-variant pattern matching and instruction emission |
| **Type Definition** | `.tyf` | Canonical IR Types and Bitwidths | `MirTypeTable.h` / `MirTypeTable.cpp` C++ class hierarchy |

---

## 2. Common Lexical & Grammar Foundation

All EzDSL sub-languages share a unified lexical foundation:

* **Comments**: C++ line comments (`// ...`) consumed up to newline.
* **Whitespace**: ASCII space, horizontal tabs, and newlines (`\r`, `\n`) act as token separators and are ignored outside string literals.
* **Identifiers**: Match `[a-zA-Z_][a-zA-Z0-9_]*`.
* **Literals**:
  - String Literals: Double-quoted strings (`"add $rd, $rs1, $rs2"`).
  - Integer Literals: Signed 64-bit integer values in Decimal (`42`, `-2048`), Hexadecimal (`0x1A2F`), Binary (`0b1010`), or Octal (`0o755`).
  - Real Literals: Standard decimal floating-point (`3.14`, `0.5`).
* **Source Tracking**: Every AST node wraps `DSL::Ast::Common::SourcedAstNode<T>`, binding zero-copy `SourceReference*` pointers for diagnostics.
* **Typed Identifiers**: Unified syntax across all declarations:
  $$\text{Type}(\text{Param})\text{:\$Name} \quad \text{or} \quad \text{Type:Name}$$
  - `GPR:rd`: Base register class `GPR`, identifier `rd`.
  - `simm(i12):imm12`: Immediate classifier `simm`, width parameter `i12`, identifier `imm12`.
  - `i32:$dst`: IR type `i32`, SSA register variable `$dst`.

---

## 3. Language Specifications

### Target Definitions (`.tdf`)
Declares target roots, inclusions, and register class hierarchies mapped to physical register banks:
```dsl
target x86_64 {
    include idf "x86_instructions.idf";
    include isf "x86_patterns.isf";
    include lad "x86_legalizerActions.lad";

    bank GPR {
        CLASS(GPR64,
            rax(, 64, 0),
            rcx(, 64, 0),
            rdx(, 64, 0),
            rbx(, 64, 0)
        );
        CLASS(GPR32,
            eax(rax, 32, 0),
            ecx(rcx, 32, 0),
            edx(rdx, 32, 0),
            ebx(rbx, 32, 0)
        );
    };
};
```

### Target Instruction Definitions (`.idf`)
Specifies binary formats, bitfield assignments, flags, and assembly templates:
```dsl
format RType(32) {
    opcode[6:0]   = 0b0110011;
    rd[11:7]      = 0;
    funct3[14:12] = 0;
    rs1[19:15]    = 0;
    rs2[24:20]    = 0;
    funct7[31:25] = 0;
};

inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format RType {
    FORMAT(
        rd = rd,
        rs1 = rs1,
        rs2 = rs2,
        funct3 = 0b000,
        funct7 = 0b0000000
    );
    ASM("add ${rd}, ${rs1}, ${rs2}");
    LATENCY(1);
    FLAGS(commutative);
}
```

### Calling Convention Definitions (`.ccdf` / `.cdf`)
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
Declares complex multi-variant addressing modes and generic-to-target selection patterns:
```dsl
addrmode AddrModeRegImm12(GPR:base, simm(i12):offset = 0) {
    variant OffsetAddr {
        match {
            ADDI $addr, GPR:$base, simm(i12):$offset;
        };
        when {
            hasOneUse($addr);
            immInRange($offset, -2048, 2047);
        };
    };
    variant BaseOnly {
        match {
            GPR:$base;
        };
    };
};

pattern Select_LW {
    match {
        LOAD i32:$dst, AddrModeRegImm12($base, $offset);
    };
    emit {
        LW GPR:$dst, GPR:$base, $offset;
    };
    cost(1);
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

EzDSL features a dedicated semantic validation pipeline (`EzDsl/include/Sema/`, `EzDsl/include/SemaPasses/`):

- **`SymbolTable` & `Scope`**: Hierarchical lexical symbol table allocating through `std::pmr::memory_resource`. Manages typed `Symbol` instances across all sub-languages.
- **`TypePass`**: Ingests `TypeDefFile` ASTs, registers interned types, validates bitwidths, and populates the symbol table.
- **`IrInstructionPass`**: Ingests `IrInstDefFile` ASTs, verifies operand counts, category invariants, and directionality rules (`IN`, `OUT`, `INOUT`).
- **`RegisterBankPass`**: Resolves register classes, banks, hardware sub-register alias hierarchies, bit-sizes, bit-offsets, and performs DFS cycle detection.
- **`InstructionDefPass`**: Ingests `InstDefFile` ASTs, checks format bitfield boundaries, operand classes, assembly placeholders, and `FORMAT` assignments.
- **`LegalizeActionPass`**: Ingests `TargetLegalizeDef` ASTs, validates legality matrices, type constraints, and widening/narrowing targets.
- **`LegalizeRulePass`**: Ingests `TargetLegalizeRuleDef` ASTs, checks SSA variable scoping between match and expand templates, and validates guard predicates.

---

## 5. C++ Code Generators

EzDSL translates verified AST and symbol table models into production C++ source and header files (`EzDsl/include/CodeGenerators/`):

- **`GenerateMirTypeTable`**: Synthesizes `MirTypeTable.h` and `MirTypeTable.cpp`. Generates direct accessor methods (`i32()`, `f64()`, `getPtr()`, `getArray()`, `getClass()`), memory-interning structures, and layout initialization logic via `IMirTargetTypeLayout`.
- **`GenerateMirIrInstructionDefs`**: Synthesizes `MirInstructionSetDefs.h`. Emits `INSTRUCTION(name, tier, category, operands, flags)` macro tables defining opcodes, instruction categories, and operand validation metadata.

---

## 6. CLI Driver (`EzDsl-cli`) & Options

`EzDsl-cli` (`ezdsl-gen`) is the standalone executable driver used to process `.tyf` and `.irdf` definitions during build time.

### CLI Usage:
```bash
ezdsl-gen [options] -i <input_file>
```

### Available Options:
| Flag | Description |
|:---|:---|
| `-i, --input <file>` | Input EzDSL definition file (`.tyf`, `.irdf`, `.tdf`, `.idf`, `.ccdf`, `.lad`, `.lrd`, `.isf`). |
| `-o, --output <path>` | Output destination directory or file path (default: `.`). |
| `-I, --include <dir>` | Directory to search for file inclusions (`include idf "..."`). |
| `--emit-type-table` | Synthesize EzMir `MirTypeTable.h` and `MirTypeTable.cpp`. |
| `--emit-instructions`| Synthesize EzMir `MirInstructionSetDefs.h`. |
| `--header-only` | Synthesize only the `.h` header file. |
| `--source-only` | Synthesize only the `.cpp` translation unit. |
| `-v, --verbose` | Enable verbose diagnostic trace logs. |
| `-h, --help` | Display command-line options. |

---

## 7. CMake Build System Integration

EzDSL integrates cleanly into CMake build workflows via helper modules in `EzMir/CMake/`:

```cmake
# Generate C++ MirTypeTable class from types.tyf
EzDslGenTypeTable(
    TARGET EzMir
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/types.tyf
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/Type
)

# Generate C++ MIR Instruction Set Definitions from instructions.irdf
EzDslGenMirInstructions(
    TARGET EzMir
    INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/instructions.irdf
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/Instruction
)
```

---

## 8. Memory Architecture

EzDSL is built on `std::pmr::monotonic_buffer_resource` arenas:
- **Zero-allocation ASTs**: AST collections, literals, and symbol tables share a contiguous memory block.
- **Fast teardown**: Releasing the top-level arena instantly frees all AST memory without individual node deallocations.
