# EzDsl Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzDsl.md) > **EzDsl** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzDsl` is EzPacker's domain-specific language compiler and code generator toolchain. Writing compiler backend components (instruction selectors, register class hierarchies, ABI calling conventions, legalization decision tables, and binary instruction encoders) by hand in C++ is error-prone, verbose, and difficult to maintain.

EzDsl solves this problem by allowing backend architects to express target architectures declaratively in high-level DSL specification files. The `ezdsl-cli` compiler validates these descriptions through semantic analysis and generates optimized, type-safe C++20 code used directly by `EzMir`, `EzTriple`, and `EzTargets`.

```
 +-------------------------------------------------------------------------------+
 |                            Declarative DSL Files                              |
 | .tyf (Types)   .irdf (MIR Ops)    .rdf (Registers)   .tidf (Target Insts)     |
 | .edf (Encoding).ccdf (CallingConv).lrd (Legal Rules) .isf (Inst Selection)    |
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
 | - MirTypeTable.generated.h / .cpp      - TargetDesc.generated.h / .cpp        |
 | - LegalizerActionTable.generated.cpp   - InstructionSelector.generated.cpp    |
 | - TargetEncodingTable.generated.cpp    - CallingConv.generated.cpp            |
 +-------------------------------------------------------------------------------+
```

---

## 2. EzDSL Dialects & File Extensions

EzDSL comprises multiple specialized dialects, each addressing a distinct compiler subsystem:

| Extension | Language Dialect | Parser Class | Emitted Artifact |
|---|---|---|---|
| `.tyf` | `TypeDef` | `TypeDefLang` | Interned type singletons and `MirTypeTable` extensions |
| `.irdf` | `IrInstDef` | `IrInstructionDefLang` | Generic MIR opcodes, operand arities, and flags |
| `.rdf` | `RegisterDef` | `RegisterDefLang` | Hardware register banks, register classes, and aliases |
| `.tidf` / `.idf` | `TargetInstDef` | `TargetInstDefLang` | Machine instructions, operand constraints, execution latency |
| `.edf` | `EncodingDef` | `EncodingDefLang` | Binary encoding templates (ModR/M, SIB, REX, opcode maps) |
| `.ccdf` / `.ezcc` | `CallingConv` | `CallingConvDefLang` | Argument/return register assignments and shadow space |
| `.tddf` / `.tdesc` | `TargetDesc` | `TargetDescDefLang` | Target descriptor wiring, pointer widths, and alignments |
| `.lad` | `LegalizeAction` | `LegalizeActionDefLang` | 3-tier legalization action table matrix |
| `.lrd` | `LegalizeRule` | `LegalizeRuleDefLang` | Pattern-based algebraic rewrite and legalization rules |
| `.isdf` / `.isf` | `InstructionSelect` | `InstructionSelectDefLang` | Bottom-Up Maximal Munch pattern matching trees |

---

## 3. The 3-Stage Toolchain Architecture

### 3.1 Lexer & Recursive Descent Parser (`EzDsl/Lexer/`)
- Each dialect has a specialized parser class derived from or utilizing `ParseContext`:
  - `TypeDefLang`, `IrInstructionDefLang`, `RegisterDefLang`, `TargetInstDefLang`, `EncodingDefLang`, `CallingConvDefLang`, `LegalizeActionDefLang`, `LegalizeRuleDefLang`, `InstructionSelectDefLang`, `TargetDescDefLang`.
- Produces typed AST nodes declared in `EzDsl/Lexer/include/Ast/` (e.g., `TypeDefAst`, `RegisterDefAst`, `TargetInstDefAst`, `CallingConvDefAst`, `LegalizeRuleDefAst`).

### 3.2 Semantic Analysis (Sema) (`EzDsl/Sema/`)
- Driven by `PassDriver` (`SemaPasses/PassDriver.h`).
- Executes dialect passes in dependency order:
  1. `TypePass`: Validates type names, vector layouts, and scalar widths.
  2. `IrInstructionPass`: Validates opcode names, operand types, and commutative flags.
  3. `CallingConvPass`: Validates parameter registers, return registers, and shadow space sizes.
  4. `TargetInstPass`: Verifies operand count matching and register class constraints.
  5. `LegalizeActionPass` & `LegalizeRulePass`: Checks predicate function signatures, transform helper references, and type compatibility.
  6. `InstructionSelectPass`: Validates pattern DAGs, ensuring every input operand maps to an instruction operand or bound variable.
- Maintains hierarchical symbol tables (`SymbolTable.h`, `Scope.h`).

### 3.3 Code Generators (`EzDsl/CodeGenerators/`)
Classes derived from `CodeGenerator` generate clean, formatted C++20 code:

| Generator Class | Header Location | Description |
|---|---|---|
| `CppMirTypeTableGenerator` | `CodeGenerators/CppMirTypeTableGenerator.h` | Emits type table registration and query routines. |
| `CppMirInstructionGenerator` | `CodeGenerators/CppMirInstructionGenerator.h` | Emits opcode enum constants and instruction descriptor tables. |
| `CppLegalizerGenerator` | `CodeGenerators/CppLegalizerGenerator.h` | Emits the primary legality matrix and signature matching tables. |
| `CppLegalizeRuleGenerator` | `CodeGenerators/CppLegalizeRuleGenerator.h` | Emits C++ rule execution functions and pattern dispatchers. |
| `CppTargetInstructionGenerator` | `CodeGenerators/CppTargetInstructionGenerator.h` | Emits machine instruction descriptor structs. |
| `CppEncodingTableGenerator` | `CodeGenerators/CppEncodingTableGenerator.h` | Emits opcode tables, ModR/M layouts, and byte serializers. |
| `CppInstructionSelectorGenerator` | `CodeGenerators/CppInstructionSelectorGenerator.h` | Emits tree-matcher decision trees for Bottom-Up Maximal Munch. |
| `CppCallingConvGenerator` | `CodeGenerators/CppCallingConvGenerator.h` | Emits calling convention descriptor structures. |
| `CppRegisterInfoGenerator` | `CodeGenerators/CppRegisterInfoGenerator.h` | Emits register classes, physical IDs, and bitmasks. |
| `CppTargetDescGenerator` | `CodeGenerators/CppTargetDescGenerator.h` | Emits the root target descriptor class wiring all subsystems. |

---

## 4. Dialect Syntax Examples

### 4.1 Type Definitions (`types.tyf`)
```dsl
types {
    scalar i1   : bits(1);
    scalar i8   : bits(8);
    scalar i16  : bits(16);
    scalar i32  : bits(32);
    scalar i64  : bits(64);
    scalar i128 : bits(128);

    float f32 : bits(32);
    float f64 : bits(64);

    pointer ptr : bits(64);

    vector v4f32 : element(f32), count(4);
    vector v2f64 : element(f64), count(2);
}
```

### 4.2 Register Definitions (`registers.rdf`)
```dsl
registers x86_64 {
    bank GPR {
        reg rax : id(0), dwarf(0);
        reg rcx : id(1), dwarf(2);
        reg rdx : id(2), dwarf(1);
        reg rbx : id(3), dwarf(3);
        reg rsp : id(4), dwarf(7);
        reg rbp : id(5), dwarf(6);
        reg rsi : id(6), dwarf(4);
        reg rdi : id(7), dwarf(5);
        reg r8  : id(8), dwarf(8);
        reg r9  : id(9), dwarf(9);
        reg r10 : id(10), dwarf(10);
        reg r11 : id(11), dwarf(11);
        reg r12 : id(12), dwarf(12);
        reg r13 : id(13), dwarf(13);
        reg r14 : id(14), dwarf(14);
        reg r15 : id(15), dwarf(15);
    }

    class GR64 : bank(GPR), type(i64) {
        members = [rax, rcx, rdx, rbx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15];
        spill_size = 8;
        spill_alignment = 8;
    }
}
```

### 4.3 Calling Convention (`sysv.ccdf`)
```dsl
calling_convention SysV_AMD64 {
    return_regs {
        i32: [rax];
        i64: [rax, rdx];
        f32: [xmm0];
        f64: [xmm0, xmm1];
    }

    param_regs {
        integer: [rdi, rsi, rdx, rcx, r8, r9];
        float:   [xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7];
    }

    callee_saved: [rbx, rsp, rbp, r12, r13, r14, r15];
    shadow_space: 0;
}
```

### 4.4 Legalization Rules (`rules.lrd`)
```dsl
legalize_rules x86_64 {
    // Strength reduction: Multiply by positive power-of-two -> Arithmetic Left Shift
    rule MulToShl {
        match: (IMUL ?dst:i64, ?lhs:i64, ?imm:i64);
        when:  isPowTwo(?imm);
        emit:  (SHL ?dst, ?lhs, log2Pow2(?imm));
    }

    // Convert equality comparison with zero to TEST
    rule CmpZeroToTest {
        match: (CMP_EQ ?dst:i1, ?src:i64, 0);
        emit:  (TEST64rr ?dst, ?src, ?src);
    }
}
```

### 4.5 Instruction Selection (`patterns.isdf`)
```dsl
patterns x86_64 {
    pattern Add64_RR {
        match: (ADD ?dst:i64, ?src1:i64, ?src2:i64);
        emit:  (ADD64rr ?dst, ?src1, ?src2);
    }

    // Fold load into add: ADD dst, src1, (LOAD addr) -> ADD64rm dst, src1, addr
    pattern Add64_RM {
        match: (ADD ?dst:i64, ?src1:i64, (LOAD ?addr:ptr));
        emit:  (ADD64rm ?dst, ?src1, ?addr);
    }
}
```

---

## 5. Command-Line Interface (`ezdsl-cli`)

The `ezdsl-cli` executable provides complete control over parsing, verification, and code generation.

### Exact CLI Options (`Cli::CliOptions`)

```text
Usage: ezdsl-cli [options] <input-file>

Positional Arguments:
  <input-file>                   Primary DSL input file to process

Output and Namespace:
  -o, --output <path>            Destination file or directory for generated artifacts [default: .]
  -t, --target <name>            Target identifier substituted into generated code (e.g. x86_64)
  -n, --namespace <root>         Namespace root the code is emitted into [default: EzTargets]
  -I, --include <dir>            Additional search paths for imported DSL files (repeatable)

Input File Overrides:
  --rules <file.lrd>             Explicit path to legalization rules file
  --types <file.tyf>             Explicit path to types definition file
  --instructions <file.irdf>     Explicit path to IR instructions file

Emission Toggles:
  --emit-rules                   Generate legalization rewrite rules
  --emit-target-instructions     Generate target machine instruction descriptors
  --emit-target-encodings        Generate instruction encoding lookup tables
  --emit-instruction-selector    Generate tree-pattern instruction selector
  --emit-calling-conv            Generate calling convention descriptors
  --emit-registers               Generate register classes and physical IDs
  --emit-target-desc             Generate root target descriptor wiring class

Generator Overrides:
  --generator <kind>             Explicitly select code generator:
                                 (Auto, TypeTable, Instructions, Legalizer, Rules,
                                  TargetInstructions, TargetEncodings, InstructionSelector,
                                  CallingConv, RegisterInfo, TargetDesc)
  --header-only                  Emit only the C++ header (.h) artifact
  --source-only                  Emit only the C++ implementation (.cpp) artifact

Introspection and Debugging:
  --dump-info                    Print general input/output file metadata
  --dump-ast                     Print the parsed Abstract Syntax Tree
  --dump-symbols                 Print the populated semantic symbol table
  --dump-files                   Print the list of generated/expected files
  --format <text|json>           Output serialization format for dumps [default: text]
  --dry-run                      Simulate pipeline without writing files to disk
  --check-only                   Validate syntax and semantics without code generation
  -v, --verbose                  Enable verbose trace diagnostics
  -q, --quiet                    Suppress non-error diagnostic output
```

### CLI Execution Examples

```bash
# Verify syntax and semantic validity of x86-64 target rules
ezdsl-cli --check-only -t x86_64 EzTargets/X86_64/Dsl/x86_64_rules.lrd

# Emit C++ target instruction descriptors
ezdsl-cli --emit-target-instructions \
          -t x86_64 \
          -o build/generated/ \
          EzTargets/X86_64/Dsl/x86_64_instructions.tidf

# Dump AST in JSON format for external tooling inspection
ezdsl-cli --dump-ast --format json EzTargets/X86_64/Dsl/x86_64_sysv.ccdf
```

---

## 6. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| CLI Options | `EzDsl/Cli/include/Cli/CommandLineOptions.h` | `CliOptions`, `GeneratorKind`, `LanguageDialect`, `OutputFormat` |
| CLI Driver | `EzDsl/Cli/include/Cli/Driver.h` | `Driver` |
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
