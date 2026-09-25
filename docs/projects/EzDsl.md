# EzDsl Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzDsl.md) > **EzDsl** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzDsl` is EzPacker's domain-specific language compiler and code generator toolchain. Writing compiler backend components (instruction selectors, register class hierarchies, ABI calling conventions, legalization decision tables, and binary instruction encoders) manually in C++ is error-prone, verbose, and difficult to maintain.

EzDsl solves this problem by allowing backend architects to express target architectures declaratively in high-level DSL specification files. The `EzDslCli` compiler validates these descriptions through comprehensive syntactic and semantic analysis and generates optimized, type-safe C++20 code used directly by `EzMir`, `EzTriple`, and `EzTargets`.

```
 +-------------------------------------------------------------------------------+
 |                            Declarative DSL Files                              |
 | .tyf (Types)         .irdf (Generic MIR Ops)   .tdesc (Target Desc & Regs)    |
 | .idf (Target Insts)  .ezcc / .ccd (CallingConv).lad (Legalizer Actions)       |
 | .lrd (Rewrite Rules) .isf (Inst Selection Patterns)                           |
 +-------------------------------------------------------------------------------+
                                        |
                                        v
 +-------------------------------------------------------------------------------+
 |                          EzDsl Compiler Toolchain                             |
 |                                                                               |
 | 1. Lexer & Parser     --> Specialized AST nodes for each language dialect     |
 | 2. Semantic Analysis  --> Symbol tables, type validation, scope hierarchies   |
 | 3. Code Generators    --> Specialized C++ emitters producing C++20 code       |
 +-------------------------------------------------------------------------------+
                                        |
                                        v
 +-------------------------------------------------------------------------------+
 |                            Generated C++ Artifacts                            |
 | - MirTypeTable.h / .cpp                - MirInstructionSetDefs.h              |
 | - <Target>TargetDesc.h / .cpp          - <Target>RegisterInfo.h               |
 | - <Target>TargetInstructionTable.h/.cpp- <Target>EncodingTable.h              |
 | - <Target>LegalizerActionTable.h / .cpp- <Target>LegalizerRules.h / .cpp      |
 | - <Target>InstructionSelector.h / .cpp - <Target>CallingConvDesc.h / .cpp     |
 +-------------------------------------------------------------------------------+
```

---

## 2. EzDSL Dialects & File Extensions

EzDSL comprises multiple specialized dialects, each addressing a distinct compiler subsystem. The authoritative extension mapping in `Driver.cpp` (`kExtensionDialects`) routes input files to their respective parsers and default generators:

| Extension | Language Dialect | Parser Class | Primary Generator | Emitted Artifact(s) |
|---|---|---|---|---|
| `.tyf` | `LanguageDialect::TypeDef` | `TypeDefLang` | `CppMirTypeTableGenerator` | `MirTypeTable.h`, `MirTypeTable.cpp` |
| `.irdf` | `LanguageDialect::IrInstDef` | `IrInstructionDefLang` | `CppMirInstructionGenerator` | `MirInstructionSetDefs.h` |
| `.lad` | `LanguageDialect::LegalizeAction` | `LegalizeActionDefLang` | `CppLegalizerGenerator` | `<Target>LegalizerActionTable.h`, `.cpp` |
| `.lrd` | `LanguageDialect::LegalizeRule` | `LegalizeRuleDefLang` | `CppLegalizeRuleGenerator` | `<Target>LegalizerRules.h`, `.cpp` |
| `.idf` | `LanguageDialect::TargetInstDef` | `TargetInstDefLang` | `CppTargetInstructionGenerator` / `CppEncodingTableGenerator` | `<Target>TargetInstructionTable.h`, `.cpp` or `<Target>EncodingTable.h` |
| `.isf` | `LanguageDialect::InstructionSelect` | `InstructionSelectDefLang` | `CppInstructionSelectorGenerator` | `<Target>InstructionSelector.h`, `.cpp` |
| `.ezcc`, `.ccd` | `LanguageDialect::CallingConv` | `CallingConvDefLang` | `CppCallingConvGenerator` | `<Target>CallingConvDesc.h`, `.cpp` |
| `.tdesc` | `LanguageDialect::TargetDesc` | `TargetDescDefLang` | `CppTargetDescGenerator` / `CppRegisterInfoGenerator` | `<Target>TargetDesc.h`, `.cpp` or `<Target>RegisterInfo.h` |

---

## 3. The 3-Stage Toolchain Architecture

### 3.1 Lexer & Recursive Descent Parser (`EzDsl/Lexer/`)
- Built on top of the modern C++ parser combinator library `lexy`.
- Each dialect has a specialized parser class utilizing `ParseContext`:
  - `TypeDefLang`: Parses primitive types, pointers, bit widths, and vector shapes.
  - `IrInstructionDefLang`: Parses generic SSA instruction prototypes, operand directions (`IN`, `OUT`), categories, tiers, and flags.
  - `RegisterDefLang`: Parses register classes, bit sizes, sub-register nesting relations (`WIDE <: NARROW`), hardware register banks, and physical encoding IDs. (Embedded within `.tdesc`).
  - `TargetInstDefLang`: Parses target machine instructions, mnemonics, operand constraints, coalescing, and embedded `ENCODING` specifications.
  - `EncodingDefLang`: Parses binary encoding templates, instruction forms (`rr`, `ri`, `rm`), opcode bytes, REX prefixes, and ModR/M field mappings.
  - `CallingConvDefLang`: Parses stack frame properties, preserve lists (callee/caller), classification rules, argument passing sequences, return conventions, and varargs handling.
  - `LegalizeActionDefLang`: Parses 3-tier legalization action rules (`LEGAL`, `WIDENS`, `NARROWS`, `PROMOTES`, `EXPANDS`, `LIBCALL`, `CUSTOM`).
  - `LegalizeRuleDefLang`: Parses pattern-based algebraic rewrite and strength-reduction rules with `match`, `when`, and `emit` clauses.
  - `InstructionSelectDefLang`: Parses addressing modes (`addrmode`) and Bottom-Up Maximal Munch pattern matching trees (`pattern`).
  - `TargetDescDefLang`: Parses top-level target properties (pointer size, stack slot size, instruction pointer, supported object formats, extensions, components, and register banks).
- Produces typed AST nodes declared in `EzDsl/Lexer/include/Ast/` (e.g., `TypeDefAst`, `RegisterDefAst`, `TargetInstDefAst`, `CallingConvDefAst`, `LegalizeRuleDefAst`, `InstructionSelectDefAst`, `TargetDescDefAst`).

### 3.2 Semantic Analysis (Sema) (`EzDsl/Sema/`)
- Driven by `PassDriver` (`SemaPasses/PassDriver.h`).
- Populates and validates symbol tables in dependency order:
  1. `TypePass`: Populates built-in primitives (`i1`, `i8`, `i16`, `i32`, `i64`, `i128`, `i256`, `f32`, `f64`, `f128`, `ptr`, `_void`, `__bindToken`) and validates custom type definitions.
  2. `IrInstructionPass`: Validates opcode names, operand types, directionality, and flags.
  3. `CallingConvPass`: Validates parameter registers, return registers, shadow space sizes, and red zones against known register classes.
  4. `TargetInstPass`: Verifies operand count matching, register class constraints, and encoding field bindings.
  5. `LegalizeActionPass` & `LegalizeRulePass`: Checks predicate function signatures, transform helper references, and type compatibility.
  6. `InstructionSelectPass`: Validates pattern DAGs, ensuring every input operand maps to an instruction operand or bound variable, and validates addressing mode variants.
  7. `TargetDescPass`: Validates target components, extension hierarchies, default calling conventions, and register bank consistency.
- Maintains hierarchical symbol tables (`SymbolTable.h`, `Scope.h`) with type-safe lookup.

### 3.3 Code Generators (`EzDsl/CodeGenerators/`)
Classes derived from `CodeGenerator` generate clean, formatted, header-guarded C++20 code:

| Generator Class | Generator Enum | Consumed Dialect | Header Location | Output Files |
|---|---|---|---|---|
| `CppMirTypeTableGenerator` | `TypeTable` | `TypeDef` (`.tyf`) | `CodeGenerators/CppMirTypeTableGenerator.h` | `MirTypeTable.h`, `MirTypeTable.cpp` |
| `CppMirInstructionGenerator` | `Instructions` | `IrInstDef` (`.irdf`) | `CodeGenerators/CppMirInstructionGenerator.h` | `MirInstructionSetDefs.h` (single header) |
| `CppLegalizerGenerator` | `Legalizer` | `LegalizeAction` (`.lad`) | `CodeGenerators/CppLegalizerGenerator.h` | `<Target>LegalizerActionTable.h`, `.cpp` |
| `CppLegalizeRuleGenerator` | `Rules` | `LegalizeRule` (`.lrd`) | `CodeGenerators/CppLegalizeRuleGenerator.h` | `<Target>LegalizerRules.h`, `.cpp` |
| `CppTargetInstructionGenerator` | `TargetInstructions` | `TargetInstDef` (`.idf`) | `CodeGenerators/CppTargetInstructionGenerator.h` | `<Target>TargetInstructionTable.h`, `.cpp` |
| `CppEncodingTableGenerator` | `TargetEncodings` | `TargetInstDef` (`.idf`) | `CodeGenerators/CppEncodingTableGenerator.h` | `<Target>EncodingTable.h` (single header) |
| `CppInstructionSelectorGenerator` | `InstructionSelector` | `InstructionSelect` (`.isf`) | `CodeGenerators/CppInstructionSelectorGenerator.h` | `<Target>InstructionSelector.h`, `.cpp` |
| `CppCallingConvGenerator` | `CallingConv` | `CallingConv` (`.ezcc`, `.ccd`) | `CodeGenerators/CppCallingConvGenerator.h` | `<Target>CallingConvDesc.h`, `.cpp` |
| `CppRegisterInfoGenerator` | `RegisterInfo` | `TargetDesc` (`.tdesc`) | `CodeGenerators/CppRegisterInfoGenerator.h` | `<Target>RegisterInfo.h` (single header) |
| `CppTargetDescGenerator` | `TargetDesc` | `TargetDesc` (`.tdesc`) | `CodeGenerators/CppTargetDescGenerator.h` | `<Target>TargetDesc.h`, `.cpp` |

---

## 4. Authentic Dialect Syntax Examples

### 4.1 Type Definitions (`EzMir/types.tyf`)
Defines the primitive scalars, pointers, and SIMD vector shapes available to MIR:

```dsl
// Special & Control Types
void _void();
bindingToken __bindToken();
pointer ptr();

// Scalar Integers
integer i1(1);
integer i8(8);
integer i16(16);
integer i32(32);
integer i64(64);
integer i128(128);

// IEEE 754 Floating-Point Types
float f32(32);
float f64(64);
float f128(128);

// Vector Types (128-bit SSE)
vector v4f32(128, 128);
vector v2f64(128, 128);
vector v4i32(128, 128);
vector v2i64(128, 128);
```

### 4.2 Generic IR Instructions (`EzMir/instructions.irdf`)
Defines architecture-neutral MIR opcodes with operand arities, categories, and semantics:

```dsl
ir_inst MOV(Register:dst OUT, AnyValue:src IN) {
    CATEGORY(DataMovement);
    TIER(HighLevel);
}

ir_inst ADD(Register:dst OUT, Register:lhs IN, RegImm:rhs IN) {
    CATEGORY(Arithmetic);
    TIER(HighLevel);
    FLAGS(SizeMatch, IsCommutative);
}

ir_inst LOAD(Register:dst OUT, AddressSource:src IN) {
    CATEGORY(Memory);
    TIER(HighLevel);
    FLAGS(ReadsMemory);
}

ir_inst RET(AnyValue:val IN) {
    CATEGORY(System);
    TIER(HighLevel);
    FLAGS(IsReturn, IsTerminator);
}
```

### 4.3 Target Descriptor & Register Banks (`x86_64.tdesc`)
Defines target architecture properties, register banks, register classes, sub-register hierarchies, and CPU extensions:

```tdesc
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
            rdx enc 2  names { rdx: GPR64, edx: GPR32, dx: GPR16, dl: GPR8 }
            rbx enc 3  names { rbx: GPR64, ebx: GPR32, bx: GPR16, bl: GPR8 }
            rsp enc 4  names { rsp: GPR64, esp: GPR32, sp: GPR16, spl: GPR8 }
            rbp enc 5  names { rbp: GPR64, ebp: GPR32, bp: GPR16, bpl: GPR8 }
            rsi enc 6  names { rsi: GPR64, esi: GPR32, si: GPR16, sil: GPR8 }
            rdi enc 7  names { rdi: GPR64, edi: GPR32, di: GPR16, dil: GPR8 }
            r8  enc 8  names { r8: GPR64,  r8d: GPR32,  r8w: GPR16,  r8b: GPR8 }
            r9  enc 9  names { r9: GPR64,  r9d: GPR32,  r9w: GPR16,  r9b: GPR8 }
            r10 enc 10 names { r10: GPR64, r10d: GPR32, r10w: GPR16, r10b: GPR8 }
            r11 enc 11 names { r11: GPR64, r11d: GPR32, r11w: GPR16, r11b: GPR8 }
            r12 enc 12 names { r12: GPR64, r12d: GPR32, r12w: GPR16, r12b: GPR8 }
            r13 enc 13 names { r13: GPR64, r13d: GPR32, r13w: GPR16, r13b: GPR8 }
            r14 enc 14 names { r14: GPR64, r14d: GPR32, r14w: GPR16, r14b: GPR8 }
            r15 enc 15 names { r15: GPR64, r15d: GPR32, r15w: GPR16, r15b: GPR8 }
        }
    }

    special {
        rip: 16
    }

    extensions {
        sse  { default: true;  description: "Streaming SIMD Extensions (SSE)"; };
        sse2 { default: true;  implies: [sse]; description: "Streaming SIMD Extensions 2 (SSE2)"; };
        avx  { default: false; implies: [sse2]; description: "Advanced Vector Extensions (AVX)"; };
    }
}
```

### 4.4 ABI Calling Conventions (`x86_64_calling_conv.ezcc`)
Defines stack layout, caller/callee register preservation, parameter passing sequences, and return rules:

```ezcc
calling_convention SysV_AMD64 {
    stack {
        align: 16,
        growth: down,
        cleanup: caller,
        shadow_space: 0,
        red_zone: 128,
        sp: rsp,
        fp: rbp
    }

    preserve callee: [rbx, rsp, rbp, r12, r13, r14, r15]
    preserve caller: [rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7]

    classify {
        types [i8, i16, i32, i64, ptr] => integer
        types [f32, f64] => sse
    }

    arguments {
        pass integer => seq([rdi, rsi, rdx, rcx, r8, r9]), fallback: stack(8)
        pass sse     => seq([xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7]), fallback: stack(8)
        pass memory  => stack(8)
    }

    returns {
        pass integer => seq([rax, rdx])
        pass sse     => seq([xmm0, xmm1])
    }
}
```

### 4.5 Legalization Actions (`x86_64_legalize.lad`)
Declares target legality matrices, widening, narrowing, and promotion actions:

```lad
target AMD64;

type_set GPR_SCALARS = (i8, i16, i32, i64);

action MOV {
    LEGAL(GPR_SCALARS, ptr, f32, f64);
    WIDENS(i1) >> i32;
};

action ADD {
    LEGAL(GPR_SCALARS);
    WIDENS(i1) >> i32;
    NARROWS(i128) >> i64;
};

action SDIV {
    LEGAL(i32, i64);
    LIBCALL(i128);
};
```

### 4.6 Legalization Rules (`x86_64_rules.lrd`)
Declares pattern-matching algebraic rewrites and strength-reduction rules:

```lrd
rule SDivPow2_64 {
    match {
        SDIV i64:$dst, i64:$lhs, imm(i64):$c;
    };
    when {
        isPowTwo($c);
        isPositiveConst($c);
    };
    emit {
        SAR i64:$dst, i64:$lhs, log2Pow2($c);
    };
};

rule UDivPow2_64 {
    match {
        UDIV i64:$dst, i64:$lhs, imm(i64):$c;
    };
    when {
        isPowTwo($c);
    };
    emit {
        SHR i64:$dst, i64:$lhs, log2Pow2($c);
    };
};
```

### 4.7 Target Machine Instructions & Encodings (`x86_64_instructions.idf`)
Declares concrete machine instructions with mnemonics, operands, flags, and binary encoding layouts:

```idf
target AMD64;

target_inst ADD64rr(GPR64:dst OUT, GPR64:src1 IN, GPR64:src2 IN) {
    MNEMONIC("addq");
    FLAGS(IsCommutative);
    ENCODING {
        form: rr;
        opcode: [0x01];
        rex_w: true;
        operands { src2 => reg; dst => rm_reg; };
        coalesce: src1;
        size: dst;
    };
};

target_inst ADD64ri(GPR64:dst OUT, GPR64:src1 IN, i64:imm IN) {
    MNEMONIC("addq");
    ENCODING {
        form: ri;
        opcode: [0x83];
        opcode_digit: 0;
        rex_w: true;
        operands { dst => rm_reg; imm => imm8_signed; };
        coalesce: src1;
        size: dst;
    };
};
```

### 4.8 Instruction Selection Patterns (`x86_64_patterns.isf`)
Defines tree-matching rewrite patterns and addressing modes for Bottom-Up Maximal Munch:

```isf
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

pattern Select_ADD64rr [cost = 1] {
    match {
        ADD i64:$dst, i64:$src1, i64:$src2;
    };
    select {
        ADD64rr GPR64:$dst, GPR64:$src1, GPR64:$src2;
    };
};
```

---

## 5. Command-Line Interface (`EzDslCli`)

The `EzDslCli` executable driver (built into `bin/EzDslCli` or `bin/EzDslCli.exe`) drives parsing, semantic analysis, introspection, and C++ code generation.

### Invocation Syntax

```text
Usage: EzDslCli -i <file> [options]
```

> [!IMPORTANT]
> The `-i` (or `--input`) option is **mandatory**. `EzDslCli` does **not** accept positional input file arguments. If omitted, the driver terminates immediately with:
> `Error: Missing required input file (-i, --input <file>).`

### Complete Command-Line Options Reference

```text
Usage: EzDslCli [options] 

EzDSL Compiler Backend Driver & Code Generator Tool

Optional arguments:
  -h, --help                     shows help message and exits 
  -v, --version                  Display tool version and exit 
  -i, --input <file>             Path to the input EzDSL definition file (.tyf, .irdf, .lad, .lrd, .idf, .isf, .ezcc, .ccd, .reg, .tdesc) [default: ""]
  -o, --output <path>            Output directory or destination file path (default: .) [default: "."]
  -I, --include <dir>            Directory to search for included files (can be specified multiple times) 
  --emit-type-table              Synthesize EzMir MirTypeTable (.h and/or .cpp) 
  --emit-instructions            Synthesize EzMir MirInstructionSetDefs (.h) 
  --emit-legalizer               Synthesize Target LegalizerActionTable (.h and .cpp) 
  --emit-rules                   Synthesize Target LegalizerRules (.h and .cpp) from .lrd 
  --emit-target-instructions     Synthesize Target TargetInstructionTable (.h and .cpp) from .idf 
  --emit-target-encodings        Synthesize Target EncodingTable (.h) from .idf ENCODING blocks 
  --emit-instruction-selector    Synthesize Target InstructionSelector (.h and .cpp) from .isf 
  --emit-calling-conv            Synthesize Target CallingConvDesc (.h and .cpp) from .ezcc / .ccd 
  --emit-registers               Synthesize Target RegisterInfo (.h) from .tdesc 
  --emit-target-desc             Synthesize TargetDesc (.h and .cpp) from .tdesc 
  --rules <file>                 Path to companion .lrd rewrite rules file [default: ""]
  --types <file>                 Path to dependency .tyf type definition file [default: ""]
  --instructions <file>          Path to dependency .irdf instruction definition file [default: ""]
  --target <target>              Target architecture name for code generation (e.g. AMD64, AArch64) [default: ""]
  --namespace-root <ns>          Namespace root the generated code is emitted into (e.g. EzTargets::X86_64) [default: "EzTargets"]
  --generator <gen>              Explicit generator to execute: 'type-table', 'instructions', 'legalizer', 'rules', 'target-instructions', 'target-encodings', 'instruction-selector', 'calling-conv', 'registers', 'target-desc', or 'auto' [default: "auto"]
  --header-only                  Synthesize only header (.h) file 
  --source-only                  Synthesize only translation unit (.cpp) file 
  --dump-info                    Dump file metadata, recognized dialect, and construct counts 
  --dump-ast                     Dump the parsed AST structure 
  --dump-symbols                 Dump the populated symbol table after semantic analysis 
  --dump-files                   Dump the list of expected/generated output files 
  --dry-run                      Parse, validate, and compute outputs without writing any files to disk 
  --check-only                   Only perform syntactic and semantic validation passes 
  -v, --verbose                  Enable verbose diagnostic trace and debug output 
  -q, --quiet                    Suppress non-essential console output 
  --format <fmt>                 Output format for dump operations: 'text' or 'json' (default: text) [default: "text"]
```

### Detailed Option Semantics

#### 1. Input and Output Management
- `-i, --input <file>`: **Required**. Path to the DSL source file. Supported file extensions: `.tyf`, `.irdf`, `.lad`, `.lrd`, `.idf`, `.isf`, `.ezcc`, `.ccd`, `.tdesc`.
- `-o, --output <path>`: Destination directory for generated artifacts, or specific output file path. Defaults to `.` (current directory).
- `-I, --include <dir>`: Adds a directory to the include search list. Repeatable.
- `--target <target>`: Name of the target architecture passed to code generators (e.g. `AMD64`, `x86_64`, `riscv64`). Note: There is **no `-t` shorthand**.
- `--namespace-root <ns>`: Root C++ namespace wrapping the generated types and classes. Defaults to `EzTargets`. Note: There is **no `-n` shorthand**.

#### 2. Companion and Dependency Files
- `--rules <file>`: Supplies a companion `.lrd` rewrite rules file when generating legalizer action tables via `--emit-legalizer`.
- `--types <file>`: Supplies an explicit `.tyf` file for semantic type lookups.
- `--instructions <file>`: Supplies an explicit `.irdf` file for IR instruction definitions.

#### 3. Generator Emission Switches (`--emit-*`)
Specifying any `--emit-*` switch selects the corresponding code generator. **Only one `--emit-*` switch may be specified per invocation**; passing multiple emission flags will result in an error: `"Cannot specify multiple generator emission flags simultaneously."`
- `--emit-type-table`: Generates `MirTypeTable.h` and `MirTypeTable.cpp` from `.tyf`.
- `--emit-instructions`: Generates `MirInstructionSetDefs.h` from `.irdf`.
- `--emit-legalizer`: Generates `<Target>LegalizerActionTable.h` and `.cpp` from `.lad`.
- `--emit-rules`: Generates `<Target>LegalizerRules.h` and `.cpp` from `.lrd`.
- `--emit-target-instructions`: Generates `<Target>TargetInstructionTable.h` and `.cpp` from `.idf`.
- `--emit-target-encodings`: Generates `<Target>EncodingTable.h` from `.idf`.
- `--emit-instruction-selector`: Generates `<Target>InstructionSelector.h` and `.cpp` from `.isf`.
- `--emit-calling-conv`: Generates `<Target>CallingConvDesc.h` and `.cpp` from `.ezcc` or `.ccd`.
- `--emit-registers`: Generates `<Target>RegisterInfo.h` from `.tdesc`.
- `--emit-target-desc`: Generates `<Target>TargetDesc.h` and `.cpp` from `.tdesc`.

#### 4. Explicit Generator Override (`--generator <gen>`)
When emission flags are not passed, the generator is automatically inferred from the input file extension (`kExtensionDialects`), or can be overridden via `--generator`. Case-insensitive accepted aliases:
- `type-table`, `typetable` -> `GeneratorKind::TypeTable`
- `instructions`, `instruction` -> `GeneratorKind::Instructions`
- `legalizer`, `legalize` -> `GeneratorKind::Legalizer`
- `rules`, `rule` -> `GeneratorKind::Rules`
- `target-instructions`, `target_instructions`, `target-inst` -> `GeneratorKind::TargetInstructions`
- `target-encodings`, `target_encodings`, `encodings` -> `GeneratorKind::TargetEncodings`
- `instruction-selector`, `instruction_selector`, `isel` -> `GeneratorKind::InstructionSelector`
- `calling-conv`, `calling_conv`, `callingconv`, `cc` -> `GeneratorKind::CallingConv`
- `registers`, `register`, `register-info`, `reg` -> `GeneratorKind::RegisterInfo`
- `target-desc`, `target_desc`, `targetdesc`, `tdesc` -> `GeneratorKind::TargetDesc`
- `auto` -> Inferred from file extension (default)

#### 5. Artifact Filtering
- `--header-only`: Directs paired generators to emit only the C++ header (`.h`) file.
- `--source-only`: Directs paired generators to emit only the translation unit (`.cpp`) file.

#### 6. Introspection, Dump & Verification Flags
- `--dump-info`: Prints file size, recognized dialect, construct counts, selected generator, and expected output file paths.
- `--dump-ast`: Prints the formatted Abstract Syntax Tree after parsing.
- `--dump-symbols`: Performs semantic passes and prints the populated symbol table and scopes.
- `--dump-files`: Lists all expected output files and reports whether each exists on disk.
- `--format <text|json>`: Controls serialization format for dump operations (`text` or `json`).
- `--dry-run`: Runs the pipeline through parsing, semantic validation, and output planning without writing files to disk.
- `--check-only`: Only performs lexing, parsing, and semantic validation passes, exiting immediately with status 0 on success.
- `-v, --verbose`: Enables detailed diagnostic trace logs.
- `-q, --quiet`: Suppresses non-error console output.

---

### Real-World CLI Invocation Examples

```bash
# 1. Syntactic and semantic validation of a target's legalization rules
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_rules.lrd --check-only --target AMD64

# 2. Synthesize C++ target instruction descriptors
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_instructions.idf \
         -o build/generated/AMD64 \
         --emit-target-instructions \
         --target AMD64 \
         --namespace-root "EzTargets::X86_64"

# 3. Synthesize binary instruction encoding lookup tables from the same .idf file
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_instructions.idf \
         -o build/generated/AMD64 \
         --emit-target-encodings \
         --target AMD64 \
         --namespace-root "EzTargets::X86_64"

# 4. Synthesize calling convention descriptors from .ezcc
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_calling_conv.ezcc \
         -o build/generated/AMD64 \
         --emit-calling-conv \
         --target AMD64 \
         --namespace-root "EzTargets::X86_64"

# 5. Synthesize register information from target descriptor
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64.tdesc \
         -o build/generated/AMD64 \
         --emit-registers \
         --target AMD64 \
         --namespace-root "EzTargets::X86_64"

# 6. Synthesize instruction selector pattern matcher
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_patterns.isf \
         -o build/generated/AMD64 \
         --emit-instruction-selector \
         --target AMD64 \
         --namespace-root "EzTargets::X86_64"

# 7. Synthesize legalizer action table with companion rules file
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_legalize.lad \
         --rules EzTargets/X86_64/targets/x86_64/x86_64_rules.lrd \
         -o build/generated/AMD64 \
         --emit-legalizer \
         --target AMD64 \
         --namespace-root "EzTargets::X86_64"

# 8. Dump parsed AST in JSON format for external tooling inspection
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_calling_conv.ezcc --dump-ast --format json

# 9. Dump semantic symbol table in text format
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_legalize.lad --dump-symbols

# 10. Inspect file metadata, dialect detection, and construct counts
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64.tdesc --dump-info
```

---

## 6. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| CLI Options | `EzDsl/Cli/include/Cli/CommandLineOptions.h` | `CliOptions`, `CommandLineParser`, `GeneratorKind`, `LanguageDialect`, `OutputFormat` |
| CLI Driver | `EzDsl/Cli/include/Cli/Driver.h` | `Driver`, `DriverResult` |
| CLI Info Dumper | `EzDsl/Cli/include/Cli/InfoDumper.h` | `InfoDumper` |
| CLI Entrypoint | `EzDsl/Cli/src/Main.cpp` | `main()` |
| Parsers | `EzDsl/Lexer/include/Parser/ParseContext.h` | `ParseContext` |
| Parsers | `EzDsl/Lexer/include/Parser/TypeDefLang.h` | `TypeDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/IrInstructionDefLang.h` | `IrInstructionDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/RegisterDefLang.h` | `RegisterDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/TargetInstDefLang.h` | `TargetInstDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/EncodingDefLang.h` | `EncodingDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/CallingConvDefLang.h` | `CallingConvDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/LegalizeActionDefLang.h` | `LegalizeActionDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/LegalizeRuleDefLang.h` | `LegalizeRuleDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/InstructionSelectDefLang.h` | `InstructionSelectDefLang` |
| Parsers | `EzDsl/Lexer/include/Parser/TargetDescDefLang.h` | `TargetDescDefLang` |
| Sema | `EzDsl/Sema/include/SemaPasses/PassDriver.h` | `PassDriver` |
| Sema | `EzDsl/Sema/include/Sema/SymbolTable.h` | `SymbolTable` |
| Sema | `EzDsl/Sema/include/Sema/Scope.h` | `Scope` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CodeGenerator.h` | `CodeGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppMirTypeTableGenerator.h` | `CppMirTypeTableGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppMirInstructionGenerator.h` | `CppMirInstructionGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppLegalizerGenerator.h` | `CppLegalizerGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppLegalizeRuleGenerator.h` | `CppLegalizeRuleGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppTargetInstructionGenerator.h` | `CppTargetInstructionGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppEncodingTableGenerator.h` | `CppEncodingTableGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppInstructionSelectorGenerator.h` | `CppInstructionSelectorGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppCallingConvGenerator.h` | `CppCallingConvGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppRegisterInfoGenerator.h` | `CppRegisterInfoGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppTargetDescGenerator.h` | `CppTargetDescGenerator` |
| Code Generators | `EzDsl/CodeGenerators/include/CodeGenerators/CppSourceEmitter.h` | `CppSourceEmitter` |
