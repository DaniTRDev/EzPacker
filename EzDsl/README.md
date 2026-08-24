# EzDSL: Compiler Backend Description Language Suite

EzDSL is a modular, declarative Domain-Specific Language (DSL) suite engineered for compiler backends. Inspired by
LLVM's TableGen, EzDSL decouples target architecture definitions, hardware instruction encodings, GlobalISel-style
legalization matrices, IR rewrite rules, and instruction selection patterns into dedicated, specialized sub-languages.

All sub-languages share a unified lexical grammar, zero-copy source reference tracking, and an allocation-efficient AST
model powered by C++20 Polymorphic Memory Resources (`std::pmr`) and `lexy`.

---

## Table of Contents

1. [Language Suite Overview](#1-language-suite-overview)
2. [Common Lexical & Grammar Foundation](#2-common-lexical--grammar-foundation)
3. [Target Definition Language (`.tdf`)](#3-target-definition-language-tdf)
4. [Instruction Definition Language (`.idf`)](#4-instruction-definition-language-idf)
5. [Legalization Action Definition Language (`.lad`)](#5-legalization-action-definition-language-lad)
6. [Legalization Rule Definition Language (`.lrd`)](#6-legalization-rule-definition-language-lrd)
7. [Instruction Selection Definition Language (`.isf`)](#7-instruction-selection-definition-language-isf)
8. [Type Definition Language (`.tyf`)](#8-type-definition-language-tyf)
9. [Memory Architecture & Driver API](#9-memory-architecture--driver-api)

---

## 1. Language Suite Overview

| Language                   | Extension | Primary Domain                                    | Key Constructs                                                         |
|----------------------------|-----------|---------------------------------------------------|------------------------------------------------------------------------|
| **Target Definition**      | `.tdf`    | Target ISA, File Inclusions, Register Hierarchies | `target`, `include`, `bank`, `CLASS`, `TargetRegister`                 |
| **Instruction Definition** | `.idf`    | Formats, Binary Encoding, Assembly, Latencies     | `format`, `inst`, `IMPLICIT`, `FORMAT`, `FLAGS`, `ASM`, `LATENCY`      |
| **Legalization Action**    | `.lad`    | Type Legality Tables, Promotions, Scalar Splits   | `action`, `LEGAL`, `WIDENS`, `NARROWS`, `LIBCALL`, `CUSTOM`, `BITCAST` |
| **Legalization Rule**      | `.lrd`    | IR-to-IR Decomposition & Pre-ISel Rewrites        | `rule`, `match`, `when`, `expand`, custom transforms                   |
| **Instruction Selection**  | `.isf`    | Generic-to-Target MIR Mapping, Addressing Modes   | `addrmode`, `variant`, `pattern`, `emit`, `cost`                       |

---

## 2. Common Lexical & Grammar Foundation

All EzDSL sub-languages share a unified lexical foundation.

### Comments & Whitespace

* **Line Comments**: Begun with `//` and consumed up to the next newline.
* **Whitespace**: ASCII space (`0x20`), horizontal tab (`0x09`), and newlines (`\n`, `\r\n`) act as token separators and
  are ignored outside string literals.

### Identifiers & Literals

* **Identifiers**: Match `[a-zA-Z_][a-zA-Z0-9_]*`.
* **String Literals**: Double-quoted character streams `"[^"\r\n]*"`.
* **Real Literals**: Standard decimal floating-point representations (`12.34`, `.5`, `42.0`).
* **Integer Literals**: Stored as signed 64-bit integers (`int64_t`) with optional leading signs:
* **Decimal**: `0`, `42`, `-2048`
* **Hexadecimal**: `0x1A2F`, `0XFF`, `-0x10`
* **Binary**: `0b101010`, `0B0`
* **Octal**: `0o755`, `0O77`

### Source Tracking

Every AST literal and identifier wraps `DSL::Ast::Common::SourcedAstNode<T>`:

```cpp
template <typename Node>
struct SourcedAstNode {
    Node m_node;
    SourceReference *m_sourceRef{ nullptr }; // Zero-copy begin/end buffer pointers
};

```

### Typed Identifier Syntax

A uniform pattern is used across declarations and pattern matchers:

$$\text{Type}(\text{Param})\text{:\$Name} \quad \text{or} \quad \text{Type:Name}$$

* `GPR:rd` $\rightarrow$ Base type `GPR`, identifier `rd`.
* `simm(i12):imm12` $\rightarrow$ Immediate classifier `simm`, width parameter `i12`, identifier `imm12`.
* `i32:$dst` $\rightarrow$ IR type `i32`, SSA register variable `dst`.

---

## 3. Target Definition Language (`.tdf`)

The `.tdf` language declares target roots, source inclusions, register aliasing trees, and register classes grouped into
register banks.

### Grammar (EBNF)

```ebnf
TargetDefFile       ::= TargetDef EOF ;

TargetDef           ::= "target" Identifier "{" TargetItem* "}" ";"? ;
TargetItem          ::= TargetIncFile | TargetRegisterBank ;

TargetIncFile       ::= "include" Identifier StringLiteral ";" ;

TargetRegisterBank  ::= "bank" Identifier "{" TargetRegisterClass* "}" ";"? ;
TargetRegisterClass ::= "CLASS" "(" Identifier ("," TargetRegisterList)? ")" ";" ;
TargetRegisterList  ::= TargetRegister ("," TargetRegister)* ;

TargetRegister      ::= Identifier "(" Identifier? "," IntegerLiteral "," IntegerLiteral ")" ;

```

### Register Aliasing Model

Hardware registers are defined via 4 parameters:

$$\text{TargetRegister}(\text{Name}, \text{ParentName}, \text{BitWidth}, \text{BitOffset})$$

* **Root Registers**: Omit the parent identifier (e.g., `rax(, 64, 0)`).
* **Sub-Registers**: Declare parent name, width, and zero-based bit offset into parent storage (e.g., `al(ax, 8, 0)`,
  `ah(ax, 8, 8)`).

### Example: Target Definition File (`x86_64.tdf`)

```dsl
target x86_64 {
    include idf "x86_instructions.idf";
    include isf "x86_patterns.isf";
    include lad "x86_legalizerActions.lad";
    include lrd "x86_legalizerRules.lrd";

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

        CLASS(GPR16,
            ax(eax, 16, 0),
            cx(ecx, 16, 0),
            dx(edx, 16, 0),
            bx(ebx, 16, 0)
        );

        CLASS(GPR8,
            al(ax, 8, 0),
            ah(ax, 8, 8),
            cl(cx, 8, 0),
            ch(cx, 8, 8)
        );
    };
};

```

---

## 4. Instruction Definition Language (`.idf`)

The `.idf` language models physical instruction encodings, bitfield layouts, operand directionality, flags, and assembly
templates.

### Grammar (EBNF)

```ebnf
InstDefFile     ::= (InstFormatDecl | InstDecl)* EOF ;

BitSlice        ::= "[" IntegerLiteral ":" IntegerLiteral "]" ;
SlicedIdentifier::= Identifier BitSlice ;

BitExpression   ::= BitExprAtom (BitExprInfixOp BitExprAtom)* ;
BitExprAtom     ::= "(" BitExpression ")"
                  | IntegerLiteral
                  | SlicedIdentifier
                  | Identifier
                  | "~" BitExprAtom ;

FormatField     ::= Identifier BitSlice ("=" BitExpression)? ";" ;
InstFormatDecl  ::= "format" Identifier ("(" IntegerLiteral ")")? "{" FormatField* "}" ;

InstOperandDir  ::= "IN" | "OUT" | "INOUT" ;
InstArgItem     ::= Identifier ("(" Identifier ")")? ":" Identifier InstOperandDir ;

InstHeader      ::= "inst" Identifier "(" (InstArgItem ("," InstArgItem)*)? ")" "format" Identifier ;

InstFlag        ::= "isBranch" | "isCall" | "isReturn" | "isTerminator" 
                  | "mayLoad" | "mayStore" | "commutative" | "volatile" ;

InstBodyItem    ::= "IMPLICIT" "(" (InstArgItem ("," InstArgItem)*)? ")" ";"
                  | "FORMAT" "(" (BitExprAssign ("," BitExprAssign)*)? ")" ";"
                  | "FLAGS" "(" (InstFlag ("," InstFlag)*)? ")" ";"
                  | "ASM" "(" StringLiteral ")" ";"
                  | "LATENCY" "(" IntegerLiteral ")" ";" ;

BitExprAssign   ::= Identifier BitSlice? "=" BitExpression ;
InstDecl        ::= InstHeader "{" InstBodyItem* "}" ;

```

### Bit-Expression Operators & Precedence

| Precedence      | Operator   | Description                     | Associativity |
|-----------------|------------|---------------------------------|---------------|
| **1 (Highest)** | `~`        | Bitwise NOT (Unary Prefix)      | Right         |
| **2**           | `+`, `-`   | Binary Addition, Subtraction    | Left          |
| **3**           | `<<`, `>>` | Bitwise Shift Left, Shift Right | Left          |
| **4**           | `&`        | Bitwise AND                     | Left          |
| **5**           | `^`        | Bitwise XOR                     | Left          |
| **6 (Lowest)**  | `          | `                               | Bitwise OR    | Left |

### Instruction Flags

* `isBranch`: Conditional or unconditional control-flow branch.
* `isCall`: Procedure / function call instruction.
* `isReturn`: Return from subroutine.
* `isTerminator`: Blocks fallthrough / ends a basic block.
* `mayLoad`: Reads from memory.
* `mayStore`: Writes to memory.
* `commutative`: Operands are algebraically reversible ($A + B = B + A$).
* `volatile`: Has unmodeled side-effects; prevents dead-code elimination.

### Example: RISC-V Instruction Definition File (`riscv_insts.idf`)

```dsl
format RType(32) {
    opcode[6:0]   = 0b0110011;
    rd[11:7]      = 0;
    funct3[14:12] = 0;
    rs1[19:15]    = 0;
    rs2[24:20]    = 0;
    funct7[31:25] = 0;
};

format IType(32) {
    opcode[6:0]   = 0b0010011;
    rd[11:7]      = 0;
    funct3[14:12] = 0;
    rs1[19:15]    = 0;
    imm[31:20]    = 0;
};

inst ADD(GPR:rd OUT, GPR:rs1 IN, GPR:rs2 IN) format RType {
    FORMAT(
        rd = rd,
        rs1 = rs1,
        rs2 = rs2,
        funct3 = 0b000,
        funct7 = 0b0000000
    );
    ASM("add $rd, $rs1, $rs2");
    LATENCY(1);
    FLAGS(commutative);
}

inst LW(GPR:rd OUT, GPR:rs1 IN, simm(i12):offset IN) format IType {
    FORMAT(
        opcode = 0b0000011,
        rd = rd,
        funct3 = 0b010,
        rs1 = rs1,
        imm = offset[11:0]
    );
    ASM("lw $rd, ${offset}(${rs1})");
    LATENCY(3);
    FLAGS(mayLoad);
}

```

---

## 5. Legalization Action Definition Language (`.lad`)

The `.lad` language configures the Generic Machine IR legalizer. It maps Generic Opcodes and operand type signatures to
legalization actions.

### Grammar (EBNF)

```ebnf
TargetLegalizeDef       ::= InstructionLegalizeDecl* EOF ;

InstructionLegalizeDecl ::= "action" Identifier "{" (LegalizationClause ";")* "}" ";"? ;

LegalizeActionKind      ::= "LEGAL" 
                          | "WIDENS" 
                          | "NARROWS" 
                          | "LIBCALL" 
                          | "CUSTOM" 
                          | "BITCAST" 
                          | "UNSUPPORTED" ;

TypeConstraint          ::= Identifier (":" IntegerLiteral)? ;
TypeConstraintList      ::= TypeConstraint ("," TypeConstraint)* ;

LegalizationTarget      ::= Identifier | StringLiteral ;
LegalizationClause      ::= LegalizeActionKind "(" TypeConstraintList ")" (">>" LegalizationTarget)? ;

```

### Action Directives

| Action Directive | Semantic Function                                  | Target Required (`>>`) |
|------------------|----------------------------------------------------|------------------------|
| `LEGAL`          | Operation and type combination natively supported. | No                     |
| `WIDENS`         | Promote scalar types to larger legal size.         | Yes (`>> TargetType`)  |
| `NARROWS`        | Split scalar types into smaller legal sizes.       | Yes (`>> TargetType`)  |
| `LIBCALL`        | Lower operation into a runtime library function.   | Yes (`>> "symbol"`)    |
| `CUSTOM`         | Delegate lowering to C++ target hook.              | No                     |
| `BITCAST`        | Bitwise reinterpret to same-sized legal type.      | Yes (`>> TargetType`)  |
| `UNSUPPORTED`    | Explicitly reject type combination as invalid.     | No                     |

### Heterogeneous Type Slot Indexing

When operations have multiple independent types across operand positions:

* `i32` or `i32:0`: Constrains operand index 0 (Destination / Result).
* `ptr:1`: Constrains operand index 1 (Source / Pointer).

### Example: Legalizer Action File (`core_legalizer.lad`)

```dsl
action ADD {
    LEGAL(i32, i64, f32, f64);
    WIDENS(i1, i8, i16) >> i32;
    NARROWS(i128) >> i64;
};

action SDIV {
    LEGAL(i32, i64);
    WIDENS(i8, i16) >> i32;
    LIBCALL(i128) >> "__divti3";
};

action SEXT {
    LEGAL(i32:0, i8:1);
    LEGAL(i32:0, i16:1);
    LEGAL(i64:0, i32:1);
    WIDENS(i1:1) >> i8;
};

action LOAD {
    LEGAL(i8:0, ptr:1);
    LEGAL(i16:0, ptr:1);
    LEGAL(i32:0, ptr:1);
    LEGAL(i64:0, ptr:1);
    CUSTOM(v4f32:0, ptr:1);
};

```

---

## 6. Legalization Rule Definition Language (`.lrd`)

The `.lrd` language defines pattern-based, IR-to-IR legalization rules. It decomposes complex generic instructions into
legal generic IR sequences before instruction selection.

### Grammar (EBNF)

```ebnf
TargetLegalizeRuleDef ::= (LegalizeRewriteRule ";")* EOF ;

LegalizeRewriteRule   ::= "rule" Identifier "{" RuleBlock+ "}" ;
RuleBlock             ::= "match" "{" RuleInstruction* "}" ";"
                        | "when"  "{" RulePredicate* "}" ";"
                        | "expand""{" RuleInstruction* "}" ";" ;

RuleInstruction       ::= Identifier (RuleOperand ("," RuleOperand)*)? ";"
                        | RuleOperand ";" ;

RuleOperand           ::= Identifier "(" SsaVarList ")"
                        | Identifier ("(" Identifier ")")? ":" SsaVarName
                        | SsaVarName
                        | IntegerLiteral ;

SsaVarName            ::= "$" Identifier ;
SsaVarList            ::= SsaVarName ("," SsaVarName)* ;

PredicateArg          ::= SsaVarName | IntegerLiteral | Identifier ;
RulePredicate         ::= Identifier "(" (PredicateArg ("," PredicateArg)*)? ")" ";" ;

```

### Operand Matching Forms

* `$src`: Bound or unbound bare SSA virtual register.
* `i32:$dst`: Type-qualified SSA virtual register.
* `imm:$c`: Bound symbolic immediate constant.
* `imm(i32):$c`: Type-qualified symbolic immediate constant.
* `42`, `-10`, `0xFF`: Concrete literal immediate integer.
* `log2($shift)`: Compile-time transform hook evaluated on variables.

### Example: 64-Bit Addition Decomposition (`narrow_add.lrd`)

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

---

## 7. Instruction Selection Definition Language (`.isf`)

The `.isf` language maps Generic Machine IR patterns into target-specific machine instructions. It includes
multi-variant addressing mode matching.

### Grammar (EBNF)

```ebnf
ISelDefFile        ::= (AddrModeDef | ISelPattern)* EOF ;

AddrModeParam      ::= Identifier ("(" Identifier ")")? ":" Identifier ("=" IntegerLiteral)? ;
AddrModeParamList  ::= "(" (AddrModeParam ("," AddrModeParam)*)? ")" ;

AddrModeDef        ::= "addrmode" Identifier AddrModeParamList "{" AddrModeVariantList "}" ";"? ;
AddrModeVariantList::= (AddrModeVariant ";")* ;
AddrModeVariant    ::= "variant" Identifier "{" VariantBlock* "}" ;
VariantBlock       ::= "match" "{" RuleInstruction* "}" ";"
                     | "when"  "{" RulePredicate* "}" ";" ;

ISelPattern        ::= "pattern" Identifier "{" PatternBlock* "}" ";"? ;
PatternBlock       ::= "match" "{" RuleInstruction* "}" ";"
                     | "when"  "{" RulePredicate* "}" ";"
                     | "emit"  "{" RuleInstruction* "}" ";"
                     | "cost"  "(" IntegerLiteral ")" ";" ;

```

### Addressing Mode Fallback

Addressing modes group multiple match variants behind a unified parameter interface. Variants evaluate in order of
appearance:

1. The first variant whose `match` pattern and `when` guards succeed binds values to the parameter list.
2. Unbound parameters with default values automatically receive their default constant (e.g., `= 0`).

### Example: Pattern Matching with Addressing Modes (`isel_patterns.isf`)

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
    when {
        hasOneUse($base);
    };
    emit {
        LW GPR:$dst, GPR:$base, $offset;
    };
    cost(1);
};

pattern Select_ADDI {
    match {
        ADD i32:$dst, GPR:$src, simm(i12):$imm;
    };
    when {
        immInRange($imm, -2048, 2047);
    };
    emit {
        ADDI GPR:$dst, GPR:$src, $imm;
    };
    cost(1);
};

```

---

## 8. Type Definition Language

EzDsl allows defining the types of the MIR (EzMir) using a specific syntax:
```dsl
// Inside a type definition file (.tyf).
integer i8(8); // Defines an integer type, i8, with 8-bit width.
float f32(32); // Defines a floating point type, f32, with 32-bit width.
```

These types are used by EzMir and the rest of EzDsl to configure a target. If the type file is not present, types CAN'T be resolved properly.

---

## 9. Memory Architecture & Driver API

### Allocation Lifecycle

EzDSL employs a monotonic arena allocator to eliminate per-node heap allocations and reference-counting overhead:

```text
┌─────────────────────────────────────────────────────────────┐
│               std::pmr::monotonic_buffer_resource           │
└──────────────────────────────┬──────────────────────────────┘
                               │ Backs all AST nodes, collections, and strings
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                  DSL::Parser::ParseContext                  │
│  - Tracks source memory buffer & zero-copy string views     │
│  - Formats syntax errors and source reference diagnostics   │
└──────────────────────────────┬──────────────────────────────┘
                               │ DSL::Parser::Common::PmrAsList<T>
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                    Typed AST Root Output                    │
└─────────────────────────────────────────────────────────────┘

```

### Driver Integration Example

```cpp
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Parser/InstructionDefLang.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/InstructionSelDefLang.h"

#include <iostream>
#include <memory_resource>
#include <array>

int main() 
{
    std::string sourceBuffer = R"(
        target DemoArch {
            include idef "demo.idf";
            bank GPR {
                CLASS(GPR32, r0(, 32, 0), r1(, 32, 0));
            };
        };
    )";

    // 1. Initialize monotonic arena buffer (e.g. 64KB stack scratchpad)
    std::array<std::byte, 65536> stackBuffer;
    std::pmr::monotonic_buffer_resource arena(stackBuffer.data(), stackBuffer.size());

    // 2. Initialize parser context with diagnostic and memory managers
    DSL::Parser::ParseContext ctx(
        getDiagCollector(), 
        getSourceManager(), 
        sourceId, 
        &arena // Allocations from PmrAsList route directly here
    );

    // 3. Execute typed parser
    auto ast = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();

    if (!ast) 
    {
        std::cerr << "Parsing failed!\n";
        ctx.emitDiagnostics();
        return 1;
    }

    std::cout << "Successfully parsed target: " << ast->m_name.m_node << "\n";
    std::cout << "Registered banks: " << ast->m_regBanks.size() << "\n";

    return 0;
}

```