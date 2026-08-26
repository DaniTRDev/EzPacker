# EzPacker Backend 1.0 Complete Engineering Specification & Step-by-Step Implementation Guide

This document contains the **in-depth implementation details, architectural trade-offs, semantic analysis algorithms, C++ code generation specifications, EzTriple target synthesis design, and concrete test plans** to deliver a **stable 1.0 release** of the EzPacker compiler backend.

---

# Table of Contents
1. [Master Architecture Pipeline](#1-master-architecture-pipeline)
2. [Current Project State & Completion Audit](#2-current-project-state--completion-audit)
3. [Architectural Decisions & Trade-Offs (Pros & Cons)](#3-architectural-decisions--trade-offs-pros--cons)
4. [Phase 1: EzDSL Frontend & Semantic Analysis (STATUS: 100% COMPLETE)](#4-phase-1-ezdsl-frontend--semantic-analysis-status-100-complete)
   - [x] [1.1 Calling Convention DSL (`.ccdf`) Parser & AST](#11-calling-convention-dsl-ccdf-parser--ast)
   - [x] [1.2 Unified Symbol Table & Cross-Language Resolution](#12-unified-symbol-table--cross-language-resolution)
   - [x] [1.3 Target Definition Semantic Pass (`RegisterBankPass`)](#13-target-definition-semantic-pass-registerbankpass)
   - [x] [1.4 Target Instruction Semantic Pass (`TargetInstPass`)](#14-target-instruction-semantic-pass-targetinstpass)
   - [x] [1.5 Legalization Action Semantic Pass (`LegalizeActionPass`)](#15-legalization-action-semantic-pass-legalizeactionpass)
   - [x] [1.6 Legalization Rewrite Rule Semantic Pass (`LegalizeRulePass`)](#16-legalization-rewrite-rule-semantic-pass-legalizerulepass)
   - [x] [1.7 Instruction Selection Semantic Pass (`InstSelPass`)](#17-instruction-selection-semantic-pass-instselpass)
   - [x] [1.8 Calling Convention Semantic Pass (`CallingConvPass`)](#18-calling-convention-semantic-pass-callingconvpass)
   - [x] [1.9 IR Instruction & Type Definition Passes (`IrInstructionPass`, `TypePass`)](#19-ir-instruction--type-definition-passes-irinstructionpass-typepass)
5. [Phase 2: EzDSL C++ Code Generators & Full EzTriple Target Synthesis](#5-phase-2-ezdsl-c-code-generators--full-eztriple-target-synthesis)
   - [x] [2.1 Type Table Generator (`CppMirTypeTableGenerator`)](#21-type-table-generator-cppmirtypetablegenerator)
   - [x] [2.2 IR Instruction Definition Generator (`CppMirInstructionGenerator`)](#22-ir-instruction-definition-generator-cppmirinstructiongenerator)
   - [x] [2.3 Target Register & Bank Model Generator (`CppTargetBankGenerator`)](#23-target-register--bank-model-generator-cpptargetbankgenerator)
   - [x] [2.4 Target Instruction & Binary Encoder Generator (`CppTargetInstGenerator`)](#24-target-instruction--binary-encoder-generator-cpptargetinstgenerator)
   - [x] [2.5 Target Type Layout Generator (`CppTargetTypeLayoutGenerator`)](#25-target-type-layout-generator-cpptargettypelayoutgenerator)
   - [x] [2.6 2D Legality Action Matrix Generator (`CppLegalizerGenerator`)](#26-2d-legality-action-matrix-generator-cpplegalizergenerator)
   - [x] [2.7 Legalization Rewrite Rule Engine Generator (`CppLegalizerRuleGenerator`)](#27-legalization-rewrite-rule-engine-generator-cpplegalizerrulegenerator)
   - [x] [2.8 Multi-Variant ISel Table Generator (`CppISelTableGenerator`)](#28-multi-variant-isel-table-generator-cppiseltablegenerator)
   - [x] [2.9 Calling Convention Descriptor Generator (`CppCallingConvGenerator`)](#29-calling-convention-descriptor-generator-cppcallingconvgenerator)
   - [x] [2.10 Target Descriptor & Binary Descriptor Glue Generator (`CppTargetDescGenerator`)](#210-target-descriptor--binary-descriptor-glue-generator-cpptargetdescgenerator)
   - [x] [2.11 Multi-Target Driver CLI (`EzDsl-cli`) & Target Pipeline Dispatcher](#211-multi-target-driver-cli-ezdsl-cli--target-pipeline-dispatcher)
   - [x] [2.12 CMake Target Integration Suite (`EzDslGenBackend.cmake`)](#212-cmake-target-integration-suite-ezdslgenbackendcmake)
6. [Phase 3: EzMir Core & Execution Engines](#6-phase-3-ezmir-core--execution-engines)
   - [x] [3.1 Function-Scoped Monotonic Arenas & Core MIR Data Structures](#31-function-scoped-monotonic-arenas--core-mir-data-structures)
   - [x] [3.2 Core Analysis Passes (`CodeFlowAnalysisPass`, `LivenessAnalysisPass`, `NonSsaToSsaPass`)](#32-core-analysis-passes-codeflowanalysispass-livenessanalysispass-nonssatossapass)
   - [ ] [3.3 MIR Invariant Verifier Pass (`MirVerifierPass`)](#33-mir-invariant-verifier-pass-mirverifierpass)
   - [x] [3.4 Generic Legalizer Engine (`MirLegalizerPass`)](#34-generic-legalizer-engine-mirlegalizerpass)
   - [x] [3.5 Generic Instruction Selector Engine (`MirInstructionSelectorPass`)](#35-generic-instruction-selector-engine-mirinstructionselectorpass)
7. [Phase 4: EzTriple & Backend Hardening](#7-phase-4-eztriple--backend-hardening)
   - [x] [4.1 Target & Binary Descriptor Architecture (`TargetDesc`, `TargetBinaryDesc`)](#41-target--binary-descriptor-architecture-targetdesc-targetbinarydesc)
   - [x] [4.2 ABI Lowerer Engine (`MirAbiLowerer`, `MirAbiLowererPass`)](#42-abi-lowerer-engine-mirabilowerer-mirabilowererpass)
   - [x] [4.3 Frame Lowerer Engine (`MirFrameLowerer`, `MirFrameLowererPass`)](#43-frame-lowerer-engine-mirframelowerer-mirframelowererpass)
   - [x] [4.4 Chaitin-Briggs Register Allocator (`MirRegisterAllocator`, `MirRegisterAllocatorPass`)](#44-chaitin-briggs-register-allocator-mirregisterallocator-mirregisterallocatorpass)
8. [Phase 5: EzCodeEmitter & Direct Object Writers](#8-phase-5-ezcodeemitter--direct-object-writers)
   - [x] [5.1 Object Emitter Core (`CodeSection`, `CodeEmitterContext`, `GenericCodeEmitter`)](#51-object-emitter-core-codesection-codeemittercontext-genericcodeemitter)
   - [ ] [5.2 ELF64 Object Writer (`ElfObjectWriter`)](#52-elf64-object-writer-elfobjectwriter)
   - [ ] [5.3 PE/COFF64 Object Writer (`CoffObjectWriter`)](#53-pecoff64-object-writer-coffobjectwriter)
   - [ ] [5.4 Mach-O 64-bit Object Writer (`MachoObjectWriter`)](#54-mach-o-64-bit-object-writer-machoobjectwriter)
9. [Phase 6: End-to-End Testing & 1.0 Release Checklist](#9-phase-6-end-to-end-testing--10-release-checklist)

---

# 1. Master Architecture Pipeline

```
 ┌───────────────────────────────────────────────────────────────────────────────────────────────────┐
 │                                           EzDSL Frontend                                          │
 │  .tdf (Registers) │ .idf (Insts) │ .lad (Legality) │ .lrd (Rules) │ .isf (ISel) │ .ccdf (ABIs)   │
 └─────────────────────────────────────────────────┬─────────────────────────────────────────────────┘
                                                   │ Two-Phase Multi-Pass Sema Validation & Symbol Interning
                                                   ▼
 ┌───────────────────────────────────────────────────────────────────────────────────────────────────┐
 │                                       EzDSL Code Generators                                       │
 │  - CppTargetBankGenerator       - CppTargetInstGenerator & Binary Encoders (Encode_<Op>)          │
 │  - CppTargetTypeLayoutGenerator - CppLegalizerGenerator (2D Action Matrix)                        │
 │  - CppLegalizerRuleGenerator    - CppISelTableGenerator (Decision Tree)                           │
 │  - CppCallingConvGenerator      - CppTargetDescGenerator (<Target>TargetDesc & BinaryDescs)       │
 └─────────────────────────────────────────────────┬─────────────────────────────────────────────────┘
                                                   │ Synthesized EzTriple Target Plugin C++ Code
                                                   ▼
 ┌───────────────────────────────────────────────────────────────────────────────────────────────────┐
 │                                   EzMir & EzTriple 1.0 Pipeline                                   │
 │                                                                                                   │
 │  Generic MIR (SSA Form)                                                                           │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [MirVerifierPass] ─────────── Verify SSA Invariants, Dominance, Block Terminators & PHIs         │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [MirLegalizerPass] ────────── Driven by Synthesized .lad Action Tables & .lrd Rewriters          │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [MirInstructionSelectorPass]  Driven by Synthesized .isf Matchers & AddrMode Decision Trees      │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [MirAbiLowererPass] ───────── Driven by Synthesized Calling Convention Descriptors (.ccdf)       │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [LivenessAnalysisPass] ────── Def-Use Chains, Live-In & Live-Out Sets, Virtual Reg Intervals     │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [MirRegisterAllocatorPass] ── Chaitin-Briggs Graph Coloring, Coalescing & Spill Rewriter         │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [MirFrameLowererPass] ─────── Prologue/Epilogue Insertion (PEI), Stack Offsets & DAlloc Lowering │
 │         │                                                                                         │
 │         ▼                                                                                         │
 │  [EzCodeEmitter] ───────────── Node-based Section Finalization, Encoders & Direct Object Writers  │
 └───────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

# 2. Current Project State & Completion Audit

| Submodule | Feature / Component | Status | Location / Test Coverage |
|:---|:---|:---:|:---|
| **EzDsl (Lexy Parsers & ASTs)** | Calling Convention DSL (`.ccdf`) | ✅ **DONE** | `EzDsl/include/Parser/CallingConvDefLang.h`, `T_CallingConvDefLang.cpp` |
| **EzDsl (Lexy Parsers & ASTs)** | Target Definition DSL (`.tdf`) | ✅ **DONE** | `EzDsl/include/Parser/TargetDefLang.h`, `T_TargetDefLang.cpp` |
| **EzDsl (Lexy Parsers & ASTs)** | Instruction Definition DSL (`.idf`) | ✅ **DONE** | `EzDsl/include/Parser/InstructionDefLang.h`, `T_InstructionDefLang.cpp` |
| **EzDsl (Lexy Parsers & ASTs)** | Legalize Action DSL (`.lad`) | ✅ **DONE** | `EzDsl/include/Parser/LegalizeActionDefLang.h`, `T_LegalizeActionDefLang.cpp` |
| **EzDsl (Lexy Parsers & ASTs)** | Legalize Rule DSL (`.lrd`) | ✅ **DONE** | `EzDsl/include/Parser/LegalizeRuleDefLang.h`, `T_LegalizeRuleDefLang.cpp` |
| **EzDsl (Lexy Parsers & ASTs)** | Instruction Selection DSL (`.isf`) | ✅ **DONE** | `EzDsl/include/Parser/InstructionSelDefLang.h`, `T_InstructionSelDefLang.cpp` |
| **EzDsl (Lexy Parsers & ASTs)** | IR Instruction DSL (`.irdf`) | ✅ **DONE** | `EzDsl/include/Parser/IrInstructionDefLang.h`, `T_IrInstructionDefLang.cpp` |
| **EzDsl (Lexy Parsers & ASTs)** | Type Definition DSL (`.tyf`) | ✅ **DONE** | `EzDsl/include/Parser/TypeDefLang.h`, `T_TypeDefLang.cpp` |
| **EzDsl (Sema Passes)** | Unified Symbol Table & Scope Model | ✅ **DONE** | `EzDsl/include/Sema/SymbolTable.h`, `Scope.h`, `Symbol.h` |
| **EzDsl (Sema Passes)** | `RegisterBankPass` (TDF Validation) | ✅ **DONE** | `EzDsl/src/SemaPasses/RegisterBankPass.cpp`, `T_Sema_RegisterBankPass.cpp` |
| **EzDsl (Sema Passes)** | `TargetInstPass` (IDF Validation) | ✅ **DONE** | `EzDsl/src/SemaPasses/TargetInstPass.cpp`, `T_Sema_TargetInstPass.cpp` |
| **EzDsl (Sema Passes)** | `LegalizeActionPass` (LAD Validation) | ✅ **DONE** | `EzDsl/src/SemaPasses/LegalizeActionPass.cpp`, `T_Sema_LegalizeActionPass.cpp` |
| **EzDsl (Sema Passes)** | `LegalizeRulePass` (LRD Validation) | ✅ **DONE** | `EzDsl/src/SemaPasses/LegalizeRulePass.cpp`, `T_Sema_LegalizeRulePass.cpp` |
| **EzDsl (Sema Passes)** | `InstSelPass` (ISF Validation) | ✅ **DONE** | `EzDsl/src/SemaPasses/InstSelPass.cpp`, `T_Sema_InstSelPass.cpp` |
| **EzDsl (Sema Passes)** | `CallingConvPass` (CCDF Validation) | ✅ **DONE** | `EzDsl/src/SemaPasses/CallingConvPass.cpp`, `T_Sema_CallingConvPass.cpp` |
| **EzDsl (Sema Passes)** | `IrInstructionPass` & `TypePass` | ✅ **DONE** | `EzDsl/src/SemaPasses/IrInstructionPass.cpp`, `TypePass.cpp` |
| **EzDsl (Code Generators)** | `CppMirTypeTableGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppMirTypeTableGenerator.cpp`, `T_EzDslCli_GenTypeTable.cpp` |
| **EzDsl (Code Generators)** | `CppMirInstructionGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppMirInstructionGenerator.cpp`, `T_EzDslCli_GenMirInstruction.cpp` |
| **EzDsl (Code Generators)** | `CppTargetBankGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppTargetBankGenerator.cpp`, `T_EzDslCli_GenRegisterBanks.cpp` |
| **EzDsl (Code Generators)** | `CppTargetInstGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppTargetInstGenerator.cpp`, `T_EzDslCli_GenTargetInst.cpp` |
| **EzDsl (Code Generators)** | `CppTargetTypeLayoutGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppTargetTypeLayoutGenerator.cpp`, `T_EzDslCli_GenTargetTypeLayout.cpp` |
| **EzDsl (Code Generators)** | `CppLegalizerGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppLegalizerGenerator.cpp`, `T_EzDslCli_GenLegalizer.cpp` |
| **EzDsl (Code Generators)** | `CppLegalizerRuleGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppLegalizerRuleGenerator.cpp`, `T_EzDslCli_GenLegalizer.cpp` |
| **EzDsl (Code Generators)** | `CppISelTableGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppISelTableGenerator.cpp`, `T_EzDslCli_GenISelTable.cpp` |
| **EzDsl (Code Generators)** | `CppCallingConvGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppCallingConvGenerator.cpp`, `T_EzDslCli_GenCallingConv.cpp` |
| **EzDsl (Code Generators)** | `CppTargetDescGenerator` | ✅ **DONE** | `EzDsl/src/CodeGenerators/CppTargetDescGenerator.cpp`, `T_EzDslCli_GenTargetDesc.cpp` |
| **EzDsl (CLI Driver)** | Unified Multi-File Target Pipeline | ✅ **DONE** | `EzDsl/src/Driver/Main.cpp` |
| **EzTriple (CMake)** | Target Synthesis CMake Function | ✅ **DONE** | `EzTriple/CMake/EzDslGenBackend.cmake` |
| **EzMir (Core)** | MIR Functions, Blocks, Instructions, Operands | ✅ **DONE** | `EzMir/include/` (`MirFunction`, `MirBlock`, `MirInstruction`, `MirOperand`) |
| **EzMir (Core)** | Monotonic PMR Buffer Arenas & Builders | ✅ **DONE** | `EzMir/include/Builder/MirBuilder.h`, `MirBuilderContext.h` |
| **EzMir (Passes)** | `CodeFlowAnalysisPass` & `LivenessAnalysisPass` | ✅ **DONE** | `EzMir/src/MirPasses/Passes/` (`T_CodeFlowPass.cpp`, `T_LivenessAnalysis.cpp`) |
| **EzMir (Passes)** | `NonSsaToSsaPass` | ✅ **DONE** | `EzMir/src/MirPasses/Passes/NonSsaToSsaPass.cpp`, `T_NonSsaToSsa.cpp` |
| **EzMir (Passes)** | `MirVerifierPass` | ⏳ **PENDING** | *Phase 3.3: SSA dominance, single terminator, phi invariants* |
| **EzTriple (Passes)** | `MirLegalizer` & `MirLegalizerPass` | ✅ **DONE** | `EzTriple/src/Legalizer/`, `T_MirLegalizer.cpp` |
| **EzTriple (Passes)** | `MirInstructionSelector` & `MirInstructionSelectorPass` | ✅ **DONE** | `EzTriple/src/InstructionSelector/`, `T_MirInstructionSelector.cpp` |
| **EzTriple (Architecture)** | `TargetDesc` & `TargetBinaryDesc` Interfaces | ✅ **DONE** | `EzTriple/include/Descriptors/TargetDesc.h`, `TargetBinaryDesc.h` |
| **EzTriple (Passes)** | `MirAbiLowerer` & `MirAbiLowererPass` | ✅ **DONE** | `EzTriple/src/AbiLowerer/MirAbiLowerer.cpp`, `T_MirAbiLowerer.cpp` |
| **EzTriple (Passes)** | `MirRegisterAllocator` & `MirRegisterAllocatorPass` | ✅ **DONE** | `EzTriple/src/RegisterAllocator/MirRegisterAllocator.cpp`, `MirRegisterAllocatorPass.cpp` |
| **EzTriple (Passes)** | `MirFrameLowerer` & `MirFrameLowererPass` | ✅ **DONE** | `EzTriple/src/FrameLowerer/MirFrameLowerer.cpp`, `T_MirFrameLowerer.cpp` |
| **EzCodeEmitter** | `CodeSection`, `CodeEmitterContext`, Helpers | ✅ **DONE** | `EzCodeEmitter/include/CodeSection.h`, `CodeEmitterContext.h`, `GenericCodeEmitter.h` |
| **EzCodeEmitter** | Direct Object Writers (ELF64, COFF64, Mach-O) | ⏳ **PENDING** | *Phase 5: Binary serialization to disk from `CodeSection` buffers* |

---

# 3. Architectural Decisions & Trade-Offs (Pros & Cons)

### Design Alternative 1: Multi-Pass vs. Single-Pass Semantic Checking

| Approach | Mechanism | Pros | Cons | Verdict |
|:---|:---|:---|:---|:---|
| **Single-Pass Sema** | Validate and emit symbols during AST traversal in lexical order. | Fast, single traversal. | Fails on forward references (e.g. `.idf` using a register class defined later in `.tdf`, or `.lad` referencing types before type file inclusion). | ❌ Rejected |
| **Two-Phase Multi-Pass (Selected)** | **Phase A (Discovery):** Register all top-level symbols (types, register classes, instructions, formats, patterns) in `SymbolTable`.<br>**Phase B (Validation):** Perform structural, bitfield, type, and pattern semantic validation with full cross-symbol resolution. | Clean handling of out-of-order declarations, cross-file includes, robust error diagnostics without cascade failures. | Requires storing the AST in PMR memory across both passes. | ✅ **Selected** (PMR memory makes AST retention trivial and free). |

### Design Alternative 2: Legalization Action Lookup Strategy

| Strategy | Mechanism | Pros | Cons | Verdict |
|:---|:---|:---|:---|:---|
| **Sparse Hash Map** | `std::unordered_map<uint64_t, LegalizeAction>` keyed by `(Opcode << 32) \| TypeId`. | Simple generation, small memory footprint for sparsely populated tables. | Hash collision overhead, branch misses, slower inner loop during heavy legalization passes. | ❌ Rejected |
| **Dense 2D Matrix Flat Array (Selected)** | Static 2D array: `g_LegalizeMatrix[OpCodeCount][TypeSlotCount][MirTypeId]` storing `LegalizeAction` byte enum. | **$O(1)$ constant-time lookup** (single direct index computation), zero pointer indirection, cache-line friendly. | Slightly larger generated data section (~tens of KB for full target matrix). | ✅ **Selected** (Critical for compiler throughput). |

### Design Alternative 3: Instruction Selection Pattern Matching Engine

| Strategy | Mechanism | Pros | Cons | Verdict |
|:---|:---|:---|:---|:---|
| **Linear Pattern Matching** | Iterates linearly over every `.isf` pattern until one matches. | Easiest code generator. | $O(N)$ matching per instruction; slow for targets with hundreds of patterns. | ❌ Rejected |
| **Decision Tree with Cost Heuristic (Selected)** | Code generator aggregates patterns by generic root opcode (`LOAD`, `ADD`, etc.), builds a nested decision tree checking operand kinds and addressing modes, and picks the lowest `cost(N)` match. | **$O(\text{tree depth})$ fast dispatch**, deterministic conflict resolution via explicit cost metric, natural fallback. | Generator algorithm requires tree construction. | ✅ **Selected** |

---

# 4. Phase 1: EzDSL Frontend & Semantic Analysis (STATUS: 100% COMPLETE)

---

## 1.1 Calling Convention DSL (`.ccdf`) Parser & AST

### Syntax Grammar Specification
```dsl
calling_conv SystemV_AMD64 {
    STACK_ALIGN(16);
    SHADOW_SPACE(0);
    RED_ZONE(128);

    CALLEE_SAVED(GPR:rbx, GPR:rsp, GPR:rbp, GPR:r12, GPR:r13, GPR:r14, GPR:r15);
    CALLER_SAVED(GPR:rax, GPR:rcx, GPR:rdx, GPR:rsi, GPR:rdi, GPR:r8, GPR:r9, GPR:r10, GPR:r11);

    ARGS {
        ASSIGN(i1, i8, i16, i32, i64, ptr) >> REG_SEQ(GPR:rdi, GPR:rsi, GPR:rdx, GPR:rcx, GPR:r8, GPR:r9) >> STACK;
        ASSIGN(f32, f64) >> REG_SEQ(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3, FPR:xmm4, FPR:xmm5, FPR:xmm6, FPR:xmm7) >> STACK;
    };

    RETURNS {
        ASSIGN(i1, i8, i16, i32, i64, ptr) >> REG_SEQ(GPR:rax, GPR:rdx);
        ASSIGN(f32, f64) >> REG_SEQ(FPR:xmm0, FPR:xmm1);
        INDIRECT_SRET >> GPR:rdi;
    };
};
```

### AST Structures (`EzDsl/include/Ast/CallingConvDefLangAst.h`)
```cpp
namespace DSL::Ast::CallingConvDef {

enum class ReturnStrategy {
    DirectReg,
    IndirectSret
};

struct RegisterAssignRule {
    std::pmr::vector<Common::Identifier> m_types;
    std::pmr::vector<Common::Identifier> m_registers;
    bool m_fallbackToStack{ true };
};

struct ReturnAssignRule {
    ReturnStrategy m_strategy{ ReturnStrategy::DirectReg };
    std::pmr::vector<Common::Identifier> m_types;
    std::pmr::vector<Common::Identifier> m_registers;
    std::optional<Common::Identifier> m_sretRegister;
};

struct CallingConvDecl {
    Common::Identifier m_name;
    uint32_t m_stackAlignment{ 16 };
    uint32_t m_shadowSpaceSize{ 0 };
    uint32_t m_redZoneSize{ 0 };
    std::pmr::vector<Common::Identifier> m_calleeSaved;
    std::pmr::vector<Common::Identifier> m_callerSaved;
    std::pmr::vector<RegisterAssignRule> m_argRules;
    std::pmr::vector<ReturnAssignRule> m_returnRules;
};

struct CallingConvDefFile {
    std::pmr::vector<CallingConvDecl> m_callingConvs;
};

} // namespace DSL::Ast::CallingConvDef
```

---

## 1.2 Unified Symbol Table & Cross-Language Resolution

### Symbol Types & Data Representation (`EzDsl/include/Sema/Symbol.h`)
```cpp
namespace Sema::Symbols {

struct TargetSymbol {
    std::string_view m_targetName;
    std::pmr::vector<SymbolId> m_banks;
};

struct RegisterSymbol {
    std::string_view m_name;
    std::string_view m_parentName;
    size_t m_bitSize;
    size_t m_bitOffset;
    SymbolId m_classId;
};

struct RegisterClassSymbol {
    std::string_view m_name;
    SymbolId m_bankId;
    std::pmr::vector<SymbolId> m_registers;
};

struct FormatFieldSymbol {
    std::string_view m_name;
    uint32_t m_startBit;
    uint32_t m_endBit;
};

struct InstructionFormatSymbol {
    std::string_view m_name;
    uint32_t m_bitWidth;
    std::pmr::vector<FormatFieldSymbol> m_fields;
};

struct TargetInstructionSymbol {
    std::string_view m_name;
    SymbolId m_formatId;
    std::pmr::vector<DSL::Ast::InstDef::InstOperand> m_args;
    std::pmr::vector<DSL::Ast::InstDef::InstOperand> m_implicitArgs;
    std::string_view m_asmTemplate;
    uint32_t m_latency;
    std::pmr::vector<DSL::Ast::InstDef::InstFlag> m_flags;
};

struct LegalizeActionSymbol {
    std::string_view m_genericOpcode;
    std::pmr::vector<DSL::Ast::LegalizeActionDef::LegalizeActionClause> m_clauses;
};

struct ISelPatternSymbol {
    std::string_view m_patternName;
    DSL::Ast::InstSelDef::ISelPattern m_patternAst;
};

} // namespace Sema::Symbols
```

---

## 1.3 Target Definition Semantic Pass (`TargetDefPass`)

### Validation Algorithm
```
Procedure ValidateTargetDef(targetAst, symTable, diagCollector):
  1. Register TargetSymbol in global scope.
  2. For each bank in targetAst.m_regBanks:
       Declare RegisterBankSymbol in symTable.
       For each CLASS in bank.m_classes:
         Declare RegisterClassSymbol in bank scope.
         For each reg in class.m_registers:
           Check (size > 0), check (offset + size <= parent.size if parent present).
           Check no duplicate register names.
           Declare RegisterSymbol in class scope.
  3. Cycle Detection on Sub-Registers:
       Build adjacency graph: Parent -> Children.
       Run DFS/Tarjan cycle detection. If cycle found -> emit Diag_Error with source reference.
```

### Negative Test Cases (`T_Sema_TargetDefPass.cpp`)
- `TestSubRegisterBitOverflow`: Register `eax` size 32, offset 40 in `rax` (64-bit) -> Error: "Sub-register 'eax' exceeds parent 'rax' boundary (40 + 32 > 64)".
- `TestCyclicRegisterAlias`: `rax` parent `eax`, `eax` parent `rax` -> Error: "Cyclic register alias detected: rax -> eax -> rax".
- `TestDuplicateRegisterInClass`: Two `rax` in same `GPR64` -> Error: "Duplicate register 'rax' in class 'GPR64'".

---

## 1.4 Target Instruction Semantic Pass (`InstructionDefPass`)

### Validation Algorithm
```
Procedure ValidateInstructionDef(instDefAst, symTable, diagCollector):
  1. For each format in instDefAst.m_formats:
       Check format.bitWidth in {8, 16, 32, 64, 128}.
       Sort fields by startBit.
       Check for bit overlaps: for i from 0 to N-2:
         If fields[i].endBit >= fields[i+1].startBit:
           Emit Diag_Error: "Bitfield overlap between fields[i] and fields[i+1]".
       Declare InstructionFormatSymbol in symTable.

  2. For each inst in instDefAst.m_instructions:
       Verify inst.m_formatName exists in symTable as InstructionFormatSymbol.
       For each operand in inst.m_args:
         If operand.kind == Register:
           Verify operand.typeOrClass exists in symTable as RegisterClassSymbol.
         Verify operand directions (IN/OUT/INOUT).
       Verify FORMAT(...) assignments:
         For each assignment field = expr:
           Verify 'field' exists in format.
           Verify bit slice width of expr matches field bitwidth.
       Declare TargetInstructionSymbol in symTable.
```

---

## 1.5 Legalization Action Semantic Pass (`LegalizeActionPass`)

### Validation Algorithm
```
Procedure ValidateLegalizeAction(actionAst, symTable, diagCollector):
  1. For each action in actionAst.m_instructionActions:
       Verify action.m_instName exists in symTable as GenericIrInstruction (from .irdf).
       Maintain LegalityMap: (TypeSlotIndex, TypeName) -> ActionKind.
       For each clause in action.m_actions:
         For each typeConstraint in clause.m_types:
           Verify typeConstraint.m_type exists in symTable as Type (from .tyf).
           SlotIdx = typeConstraint.m_typeIndex.value_or(0).
           If LegalityMap.contains(SlotIdx, typeConstraint.m_type):
             Emit Diag_Error: "Conflicting legalization action for opcode OP on slot SlotIdx and type Type".
           LegalityMap[SlotIdx, typeConstraint.m_type] = clause.m_kind.
           If clause.m_kind in {WidenScalar, NarrowScalar, Bitcast}:
             Verify clause.m_targetType present and valid in symTable.
           If clause.m_kind == Libcall:
             Verify clause.m_libcallSymbol is non-empty.
```

---

## 1.6 Legalization Rewrite Rule Semantic Pass (`LegalizeRulePass`)

### Validation Algorithm
```
Procedure ValidateLegalizeRule(ruleAst, symTable, diagCollector):
  1. For each rule in ruleAst.m_rules:
       BoundVars = Set<String>().
       // A. Check Match Block
       For each matchInst in rule.m_matchPatterns:
         Verify matchInst.opcode exists in symTable.
         For each operand in matchInst.m_operands:
           If operand is SsaVariable:
             If operand.type present: verify type in symTable.
             BoundVars.insert(operand.name).

       // B. Check When Guard
       For each predicate in rule.m_predicates:
         Verify predicate.name in TargetHookCatalog.
         For each arg in predicate.arguments:
           If arg is SsaVariable:
             Verify arg.name in BoundVars -> else Emit Diag_Error: "Unbound variable in predicate".

       // C. Check Expand Block
       For each expandInst in rule.m_expansionSequence:
         Verify expandInst.opcode in symTable.
         For each operand in expandInst.m_operands:
           If operand is SsaVariable:
             If operand is read:
               Verify operand.name in BoundVars -> else Emit Diag_Error: "Unbound SSA variable read in expand".
             If operand is written:
               BoundVars.insert(operand.name).
```

---

## 1.7 Instruction Selection Semantic Pass (`InstructionSelPass`)

### Validation Algorithm
```
Procedure ValidateInstructionSel(iselAst, symTable, diagCollector):
  1. For each addrmode in iselAst.m_addrModes:
       Params = addrmode.m_parameters.
       For each variant in addrmode.m_variants:
         VariantBound = ExtractBoundVariables(variant.m_matchPatterns).
         For each param in Params:
           If param.name not in VariantBound and param.defaultValue is null:
             Emit Diag_Error: "Variant misses required parameter binding without default value".

  2. For each pattern in iselAst.m_patterns:
       PatternBound = ExtractBoundVariables(pattern.m_matchPatterns).
       For each emitInst in pattern.m_emitSequence:
         Verify emitInst.opcode exists as TargetInstruction in symTable.
         For each operand in emitInst.m_operands:
           Verify operand variable in PatternBound.
```

---

## 1.8 Calling Convention Semantic Pass (`CallingConvPass`)

### Validation Algorithm
```
Procedure ValidateCallingConv(ccAst, symTable, diagCollector):
  1. For each cc in ccAst.m_callingConvs:
       Verify cc.m_stackAlignment in {4, 8, 16, 32, 64}.
       CalleeSet = Set(cc.m_calleeSaved).
       CallerSet = Set(cc.m_callerSaved).
       Overlap = CalleeSet & CallerSet.
       If Overlap is not empty:
         Emit Diag_Error: "Registers cannot be both CALLEE_SAVED and CALLER_SAVED".
       Verify all registers in CalleeSet, CallerSet, ARGS, and RETURNS exist in symTable.
```

# 5. Phase 2: EzDSL C++ Code Generators & Full EzTriple Target Synthesis

To enable EzDSL to generate a **complete, standalone `EzTriple` target** (e.g. `AMD64`, `RiscV64`, `ARM64`) without requiring handwritten backend glue, EzDSL must synthesize 8 specific C++ code artifacts and integrate them via a unified CLI driver and CMake integration.

---

## 2.1 Type Table Generator (`CppMirTypeTableGenerator`) [DONE]
- **File Output:** `MirTypeTable.h`, `MirTypeTable.cpp`
- **Location:** `EzDsl/include/CodeGenerators/CppMirTypeTableGenerator.h`, `src/CodeGenerators/CppMirTypeTableGenerator.cpp`
- **Responsibilities:** Emits singleton `MirTypeTable` with all built-in and target-declared scalar, pointer, vector, and struct types.
- **Test Coverage:** `tests/EzDslTestSuite/tests/T_EzDslCli_GenTypeTable.cpp`

---

## 2.2 IR Instruction Definition Generator (`CppMirInstructionGenerator`) [DONE]
- **File Output:** `MirInstructionSet.h`
- **Location:** `EzDsl/include/CodeGenerators/CppMirInstructionGenerator.h`, `src/CodeGenerators/CppMirInstructionGenerator.cpp`
- **Responsibilities:** Emits generic MIR instruction opcodes, mnemonic tables, operand count metadata, and builder helper declarations.
- **Test Coverage:** `tests/EzDslTestSuite/tests/T_EzDslCli_GenMirInstruction.cpp`

---

## 2.3 Target Register & Bank Model Generator (`CppTargetBankGenerator`) [DONE]
- **File Output:** `<Target>RegisterBanks.h`, `<Target>RegisterBanks.cpp`
- **Input DSL:** `.tdf` (Target Definition File)
- **Synthesized Structures:**
  1. **Register Enumeration:**
     ```cpp
     enum class TargetReg : uint16_t {
         NoRegister = 0,
         RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
         R8, R9, R10, R11, R12, R13, R14, R15,
         EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI,
         // ...
         TARGET_REG_COUNT
     };
     ```
  2. **Physical Register Descriptors (`MirRegister`):**
     - Emits static array of physical `MirRegister` instances with bit sizes, offsets within parent registers, and parent links.
  3. **Register Class Descriptors (`MirRegisterClass`):**
     - Emits `MirRegisterClass` instances (e.g. `GPR64`, `GPR32`, `FPR64`) holding member register IDs, spill size, and spill alignment.
  4. **Register Bank Descriptors (`MirRegisterBank`):**
     - Emits `MirRegisterBank` instances (e.g. `GPRBank`, `FPRBank`) grouping classes together.
  5. **Sub-Register Aliasing Bitmask Table:**
     - Emits constant-time register overlap and interference checking:
       ```cpp
       bool TargetRegistersOverlap(TargetReg regA, TargetReg regB);
       const std::pmr::vector<TargetReg> &GetSubRegisters(TargetReg reg);
       const std::pmr::vector<TargetReg> &GetSuperRegisters(TargetReg reg);
       ```

---

## 2.4 Target Instruction & Binary Encoder Generator (`CppTargetInstGenerator`) [DONE]
- **File Output:** `<Target>InstructionDefs.h`, `<Target>InstructionDefs.cpp`, `<Target>BinaryEncoder.cpp`
- **Input DSL:** `.idf` (Instruction Definition File), `.tdf`
- **Synthesized Structures:**
  1. **Target Opcode Enumeration:**
     ```cpp
     enum class TargetOpCode : uint16_t {
         TARGET_OPCODE_START = 1000,
         MOV_r64_r64,
         ADD_r64_r64,
         ADD_r64_imm32,
         MOV_r64_m64,
         MOV_m64_r64,
         // ...
         TARGET_OPCODE_COUNT
     };
     ```
  2. **Target Instruction Descriptors (`MirTargetInstructionDesc`):**
     - Emits static table describing instruction latency, operand counts, register class constraints per operand, implicit defs/uses, and instruction classification flags (`isTerminator`, `isBranch`, `isCall`, `isReturn`, `isMove`, `isLoad`, `isStore`).
  3. **Bitfield-Packing Binary Encoders (`Encode_<OpCode>`):**
     - For each format in `.idf`, synthesizes exact C++ bit shifting and bitmasking into `CodeSection`:
     ```cpp
     void Encode_ADD_r64_r64(CodeSection *sec, const MirInstruction *inst) {
         uint32_t rex = 0x48; // REX.W
         uint8_t opcode = 0x01;
         uint8_t modrm = 0xC0 | (GetRegNum(inst->getOperand(1)) << 3) | GetRegNum(inst->getOperand(0));
         sec->emit8(rex);
         sec->emit8(opcode);
         sec->emit8(modrm);
     }
     ```
  4. **Target Instruction Disassembler / Printer:**
     - Emits `<Target>InstPrinter::print(std::ostream &os, const MirInstruction *inst)` substituting operands into the declared `asmTemplate`.

---

## 2.5 Target Type Layout Generator (`CppTargetTypeLayoutGenerator`) [DONE]
- **File Output:** `<Target>TypeLayout.h`, `<Target>TypeLayout.cpp`
- **Input DSL:** `.tyf`, Target machine pointer size & alignment rules
- **Synthesized Structures:**
  - Concrete class `<Target>TargetTypeLayout : public IMirTargetTypeLayout`:
    - `getTypeSize(MirType *type)`
    - `getTypeAlignment(MirType *type)`
    - `getStructMemberOffset(MirStructType *structType, size_t index)`
    - `padStructLayout(MirStructType *structType)`

---

## 2.6 2D Legality Action Matrix Generator (`CppLegalizerGenerator`) [DONE]
- **File Output:** `<Target>LegalizerActionTable.h`, `<Target>LegalizerActionTable.cpp`
- **Input DSL:** `.lad` (Legalize Action Definition File), `.irdf`, `.tyf`
- **Synthesized Structures:**
  - Flat 2D/3D static lookup array:
    ```cpp
    // Indexed by [GenericOpcode - GENERIC_OPCODE_START][TypeSlotIndex][MirTypeId]
    static const LegalizeAction g_<Target>_LegalizeMatrix[GENERIC_OPCODE_COUNT][MAX_TYPE_SLOTS][MAX_TYPES] = {
        /* [ADD][Slot 0][i32] = */ LegalizeAction::Legal,
        /* [ADD][Slot 0][i64] = */ LegalizeAction::Legal,
        /* [ADD][Slot 0][i128]= */ LegalizeAction::NarrowScalar,
        /* [SDIV][Slot 0][i64]= */ LegalizeAction::Libcall,
        // ...
    };
    
    LegalizeAction GetTargetLegalizeAction(MirInstructionOpCode op, size_t slot, MirTypeId type);
    ```

---

## 2.7 Legalization Rewrite Rule Engine Generator (`CppLegalizerRuleGenerator`) [DONE]
- **File Output:** `<Target>LegalizeRules.h`, `<Target>LegalizeRules.cpp`
- **Input DSL:** `.lrd` (Legalize Rule Definition File)
- **Synthesized Structures:**
  - Concrete class `<Target>LegalizeRules : public MirExpansionRuleRegistry`:
    ```cpp
    class <Target>LegalizeRules {
    public:
        static bool tryExpand(MirBuilderContext *ctx, MirInstruction *inst);
    private:
        static bool Expand_NarrowAddi128(MirBuilderContext *ctx, MirInstruction *inst);
        static bool Expand_CustomLowerCall(MirBuilderContext *ctx, MirInstruction *inst);
    };
    ```
  - Synthesizes SSA variable unmerging, carry/borrow chain insertion, and target helper calls according to `.lrd` expansion blocks.

---

## 2.8 Multi-Variant ISel Table Generator (`CppISelTableGenerator`) [DONE]
- **File Output:** `<Target>ISelTable.h`, `<Target>ISelTable.cpp`
- **Input DSL:** `.isf` (Instruction Selection File)
- **Synthesized Structures:**
  1. **Addressing Mode Matchers:**
     ```cpp
     struct AddrModeRegImmResult { MirOperand *base; int64_t offset; };
     bool Match_AddrModeRegImm(MirOperand *addr, AddrModeRegImmResult &res);
     ```
  2. **Decision-Tree Instruction Selector (`<Target>InstructionSelector`):**
     ```cpp
     class <Target>InstructionSelector : public MirInstructionSelector {
     public:
         bool select(MirBuilderContext *ctx, MirInstruction *inst) override;
     };
     ```
     - Groups patterns by root opcode (`LOAD`, `STORE`, `ADD`, `BR_COND`).
     - Emits nested pattern tests ordered by pattern cost heuristic ($\text{cost} = \text{pattern depth} \times 10 - \text{emitted instructions}$).
     - Emits replacement sequences constructing target `MirInstruction` nodes with `MirInstructionBuilder`.

---

## 2.9 Calling Convention Descriptor Generator (`CppCallingConvGenerator`) [DONE]
- **File Output:** `<Target>CallingConventions.h`, `<Target>CallingConventions.cpp`
- **Input DSL:** `.ccdf` (Calling Convention Definition File)
- **Synthesized Structures:**
  - Calling convention factory functions:
    ```cpp
    CallingConvDesc *Create_SystemV_AMD64(std::pmr::memory_resource *alloc);
    CallingConvDesc *Create_Win64_AMD64(std::pmr::memory_resource *alloc);
    ```
  - Automatically initializes:
    - Stack alignment, shadow space size, red zone size.
    - Callee-saved and caller-saved register bitsets.
    - Argument assignment rules (GPR sequence, FPR sequence, stack fallback).
    - Return value assignment rules & indirect SRET register configurations.

---

## 2.10 Target Descriptor & Binary Descriptor Glue Generator (`CppTargetDescGenerator`) [DONE]
- **File Output:** `<Target>TargetDesc.h`, `<Target>TargetDesc.cpp`, `<Target>TargetBinaryDesc.h`, `<Target>TargetBinaryDesc.cpp`
- **Input DSL:** `.tdf`, Target configuration
- **Synthesized Structures:**
  1. **`<Target>TargetDesc : public TargetDesc`:**
     - Implements `getTypeLayout()`, `getExpansionRegistry()`, `getFrameLowerer()`, `getInstructionSelector()`, `getLegalizer()`, `getRegisterAllocator()`, `getAvailableRegisterBanks()`, `getAvailableCallingConventions()`, `getNearestLegalType()`, `getInstructionPtrReg()`, `getStackSlotSize()`.
  2. **`<Target><OS>TargetBinaryDesc : public TargetBinaryDesc`:**
     - Implements `isLittleEndian()`, `isPositionIndependent()`, `getCodeModel()`, `getObjectFormat()`, `getFunctionAlignment()`, `getLoopAlignment()`, `getSections()`.
  3. **Target Registry Entry Point:**
     ```cpp
     std::unique_ptr<TargetDesc> Create<Target>TargetDesc(std::pmr::memory_resource *alloc);
     ```

---

## 2.11 Multi-Target Driver CLI (`EzDsl-cli`) & Target Pipeline Dispatcher [DONE]
- **File:** `EzDsl/src/Driver/Main.cpp`
- **CLI Options for Target Pipeline:**
  ```bash
  ezdsl-gen --target AMD64 \
            --tdf targets/AMD64/AMD64.tdf \
            --idf targets/AMD64/AMD64.idf \
            --lad targets/AMD64/AMD64.lad \
            --lrd targets/AMD64/AMD64.lrd \
            --isf targets/AMD64/AMD64.isf \
            --ccdf targets/AMD64/AMD64.ccdf \
            --tyf types/StandardTypes.tyf \
            --irdf ir/MirInstructionSet.irdf \
            -o build/generated/AMD64/
  ```
- **Driver Pipeline Steps:**
  1. Parse all input files into a unified PMR AST context.
  2. Populate unified `SymbolTable` across all files.
  3. Execute semantic validation passes in dependency order:
     - `TypePass` $\to$ `IrInstructionPass` $\to$ `RegisterBankPass` $\to$ `TargetInstPass` $\to$ `LegalizeActionPass` $\to$ `LegalizeRulePass` $\to$ `InstSelPass` $\to$ `CallingConvPass`.
  4. If any diagnostic error occurs, log formatted errors with source line locations and abort.
  5. Invoke all 8 code generators to emit the complete target source files.

---

## 2.12 CMake Target Integration Suite (`EzDslGenBackend.cmake`) [DONE]
- **File:** `EzTriple/CMake/EzDslGenBackend.cmake`
- **CMake Function Specification:**
  ```cmake
  function(EzDslGenTarget TARGET_NAME)
      cmake_parse_arguments(ARG "" "OUTPUT_DIR" "TDF;IDF;LAD;LRD;ISF;CCDF;TYF;IRDF" ${ARGN})
      
      set(GENERATED_HEADERS
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}InstructionDefs.h
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}RegisterBanks.h
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}TypeLayout.h
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizerActionTable.h
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizeRules.h
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}ISelTable.h
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}CallingConventions.h
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}TargetDesc.h
      )
      
      set(GENERATED_SOURCES
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}InstructionDefs.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}BinaryEncoder.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}RegisterBanks.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}TypeLayout.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizerActionTable.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizeRules.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}ISelTable.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}CallingConventions.cpp
          ${ARG_OUTPUT_DIR}/${TARGET_NAME}TargetDesc.cpp
      )
      
      add_custom_command(
          OUTPUT ${GENERATED_HEADERS} ${GENERATED_SOURCES}
          COMMAND EzDsl-cli --target ${TARGET_NAME}
                            --tdf ${ARG_TDF}
                            --idf ${ARG_IDF}
                            --lad ${ARG_LAD}
                            --lrd ${ARG_LRD}
                            --isf ${ARG_ISF}
                            --ccdf ${ARG_CCDF}
                            --tyf ${ARG_TYF}
                            --irdf ${ARG_IRDF}
                            -o ${ARG_OUTPUT_DIR}
          DEPENDS EzDsl-cli ${ARG_TDF} ${ARG_IDF} ${ARG_LAD} ${ARG_LRD} ${ARG_ISF} ${ARG_CCDF} ${ARG_TYF} ${ARG_IRDF}
          COMMENT "Synthesizing EzTriple Target: ${TARGET_NAME}"
      )
      
      add_library(EzTriple_${TARGET_NAME} STATIC ${GENERATED_SOURCES} ${GENERATED_HEADERS})
      target_link_libraries(EzTriple_${TARGET_NAME} PUBLIC EzTriple EzMir EzCore)
  endfunction()
  ```

---

# 6. Phase 3: EzMir Core & Execution Engines

---

## 3.1 Function-Scoped Monotonic Arenas & Core MIR Data Structures [DONE]
- Implemented in `EzMir/include/` and `src/`:
  - `MirFunction`, `MirBlock`, `MirInstruction`, `MirOperand`, `MirOperands`, `MirRegisterBank`, `MirRegisterClass`, `MirRegisterReference`.
  - Monotonic PMR memory allocator scoping per function and per compilation unit.
  - Comprehensive instruction builder (`MirInstructionBuilder`), operand builder (`MirOperandBuilder`), and printer (`MirPrinter`).
- **Test Coverage:** `tests/EzMirTestSuite/tests/T_Block.cpp`, `T_Function.cpp`, `T_Instruction.cpp`, `T_Operand.cpp`, `T_GlobalVar.cpp`.

---

## 3.2 Core Analysis Passes (`CodeFlowAnalysisPass`, `LivenessAnalysisPass`, `NonSsaToSsaPass`) [DONE]
- Implemented in `EzMir/src/MirPasses/Passes/`:
  - `CodeFlowAnalysisPass`: Builds CFG predecessor/successor graphs, loop depth, and dominator sets (`T_CodeFlowPass.cpp`).
  - `LivenessAnalysisPass`: Computes Gen/Kill, Live-In/Live-Out sets, and virtual register live intervals (`T_LivenessAnalysis.cpp`).
  - `NonSsaToSsaPass`: Standard SSA construction converting mutable alloca variables into SSA $\phi$ nodes (`T_NonSsaToSsa.cpp`).

---

## 3.3 MIR Invariant Verifier Pass (`MirVerifierPass`) [TODO]
- **File:** `EzMir/include/MirPasses/Passes/MirVerifierPass.h`, `EzMir/src/MirPasses/Passes/MirVerifierPass.cpp`
- **Verification Rules:**
  1. **SSA Dominance Invariant:** For every instruction $I$ using virtual register $V$, the unique defining instruction $D(V)$ must strictly dominate $I$ (or dominate the corresponding incoming predecessor edge if $I$ is a $\phi$ node).
  2. **Terminator Invariant:** Every `MirBlock` must contain exactly one terminator instruction (`isTerminator() == true`) located strictly at the end of the block instruction list.
  3. **$\phi$ Invariant:** $\phi$ instructions must only reside at the entry of basic blocks before any non-$\phi$ instruction, with incoming block count matching predecessor count.
  4. **Type Consistency:** Operand types must match instruction signature constraints.

---

## 3.4 Generic Legalizer Engine (`MirLegalizerPass`) [DONE]
- **Files:** `EzTriple/include/Legalizer/MirLegalizerPass.h`, `EzTriple/src/Legalizer/MirLegalizerPass.cpp`, `EzTriple/src/Legalizer/MirLegalizer.cpp`, `EzTriple/src/Legalizer/MirFunctionSignatureLegalizerPass.cpp`, `EzTriple/src/Legalizer/Actions/`
- **Tested in:** `tests/EzTripleTestSuite/tests/T_MirLegalizer.cpp`
- **Execution Pipeline (Driven by generated `<Target>LegalizerActionTable` & `<Target>LegalizeRules`):**
  1. Iterate instructions in topological order.
  2. Query `GetTargetLegalizeAction(opcode, slot, type)`.
  3. Action Dispatch:
     - `Legal`: Continue without alteration.
     - `WidenScalar`: Insert `SEXT`/`ZEXT` from original type to nearest legal type, perform widened operation, insert `TRUNC` to original destination.
     - `NarrowScalar`: Split wide operands into low/high halves via `UNMERGE_VALUES`, lower into multi-word arithmetic with carry/borrow, combine results with `MERGE_VALUES`.
     - `Bitcast`: Insert reinterpret bitcast to legal bit-compatible type.
     - `Libcall`: Lower complex operation (e.g. `f128` operations, `i128` division) into standard ABI function calls (`__divti3`, `__udivti3`).
     - `Custom`: Invoke synthesized `<Target>LegalizeRules::tryExpand(ctx, inst)`.
  4. Integration with Function Signature Legalization:
     - Apply SRET transformations (`LegalizeCallAction.cpp`, `LegalizeReturnAction.cpp`, `MirFunctionSignatureLegalizerPass.cpp`).

---

## 3.5 Generic Instruction Selector Engine (`MirInstructionSelectorPass`) [DONE]
- **Files:** `EzTriple/include/InstructionSelector/MirInstructionSelectorPass.h`, `EzTriple/src/InstructionSelector/MirInstructionSelectorPass.cpp`, `EzTriple/include/InstructionSelector/MirInstructionSelector.h`
- **Tested in:** `tests/EzTripleTestSuite/tests/T_MirInstructionSelector.cpp`
- **Execution Pipeline:**
  1. Iterate basic blocks in reverse post-order.
  2. Delegate to synthesized `targetDesc->getInstructionSelector()->select(ctx, inst)`.
  3. Replaces generic MIR instructions with target physical/virtual instructions with register class constraints.
  4. Erase selected generic MIR nodes.

---

# 7. Phase 4: EzTriple & Backend Hardening

---

## 4.1 Target & Binary Descriptor Architecture (`TargetDesc`, `TargetBinaryDesc`) [DONE]
- Implemented in `EzTriple/include/Descriptors/TargetDesc.h` and `TargetBinaryDesc.h`.
- Provides full polymorphic interfaces for target hardware (CPU) and operating system binary environments (ABI).

---

## 4.2 ABI Lowerer Engine (`MirAbiLowerer`, `MirAbiLowererPass`) [DONE]
- Implemented in `EzTriple/src/AbiLowerer/MirAbiLowerer.cpp` and `MirAbiLowererPass.cpp`.
- Tested in `tests/EzTripleTestSuite/tests/T_MirAbiLowerer.cpp`:
  - `processCallBlock()`: Standardizes call sequences, allocates caller-saved spill tracking, inserts argument placement (`PUSH_ARG`).
  - `processReturnBlock()`: Inserts return value moves (`PUSH_RET`) and SRET data moves according to `CallingConvDesc`.
  - `processCallReturnBlock()`: Emits `POP_RET` unpacking returned registers into destination virtual registers.
  - `processFunctionArguments()`: Lowers incoming parameters (`POP_ARG`) into function entry registers or stack slots.

---

## 4.3 Frame Lowerer Engine (`MirFrameLowerer`, `MirFrameLowererPass`) [DONE]
- **Files:** `EzTriple/include/FrameLowerer/MirFrameLowerer.h`, `EzTriple/src/FrameLowerer/MirFrameLowerer.cpp`, `EzTriple/src/FrameLowerer/MirFrameLowererPass.cpp`
- **Tested in:** `tests/EzTripleTestSuite/tests/T_MirFrameLowerer.cpp`
- **Completed:**
  - Abstract stack frame object layout calculation (`calculateFrameLayout`) with 16-byte boundary alignment.
  - Abstract stack object reference rewriting into base pointer `MirMemory` operands (`lowerStackObjectReferences`).
  - Pass lifecycle execution (`MirFrameLowererPass`).

---

## 4.4 Chaitin-Briggs Register Allocator (`MirRegisterAllocator`, `MirRegisterAllocatorPass`) [DONE]
- Implemented in `EzTriple/src/RegisterAllocator/MirRegisterAllocator.cpp` and `MirRegisterAllocatorPass.cpp`:
  - Interference Graph construction from `LivenessResult`.
  - Chaitin-Briggs degree evaluation and simplification (`simplify`) with $K$-colorability heuristic.
  - Color assignment (`selectColors`) assigning physical registers to virtual nodes on the selection stack.
  - Spill cost calculation:
    $$\text{Cost}(v) = \frac{\sum_{u \in \text{uses}(v)} 10^{\text{loopDepth}(u)} + \sum_{d \in \text{defs}(v)} 10^{\text{loopDepth}(d)}}{\text{degree}(v)}$$
  - Virtual register rewriting to assigned physical colors (`rewriteColors`).
  - Spill insertion allocating stack slots and replacing operands with load/store sequences (`rewriteSpilledRegisters`).

---

# 8. Phase 5: EzCodeEmitter & Direct Object Writers

---

## 8.1 Object Emitter Core (`CodeSection`, `CodeEmitterContext`, `GenericCodeEmitter`) [DONE]
- Implemented in `EzCodeEmitter/include/` and `src/`:
  - `CodeSection`: Monotonic node-based byte buffer supporting 8/16/32/64-bit emissions, alignment padding, and label binding.
  - `CodeEmitterContext`: Manages relocations (`CodeRelocation`), symbol tables, and section maps.
  - `Helpers.cpp`: Section allocation templates for ELF (`.text`, `.rodata`, `.data`, `.bss`), COFF, and Mach-O.

---

## 8.2 ELF64 Object Writer (`ElfObjectWriter`) [TODO]
- **File:** `EzCodeEmitter/include/Writers/ElfObjectWriter.h`, `EzCodeEmitter/src/Writers/ElfObjectWriter.cpp`
- **Responsibilities:**
  - Emits standard System V ELF64 object files (`.o`).
  - Constructs `Elf64_Ehdr`, `Elf64_Shdr` table, string tables (`.strtab`, `.shstrtab`), symbol table (`.symtab`), and relocation tables (`.rela.text`, `.rela.data`).
  - Serializes `CodeSection` payload buffers to file descriptor or output stream.

---

## 8.3 PE/COFF64 Object Writer (`CoffObjectWriter`) [TODO]
- **File:** `EzCodeEmitter/include/Writers/CoffObjectWriter.h`, `EzCodeEmitter/src/Writers/CoffObjectWriter.cpp`
- **Responsibilities:**
  - Emits Microsoft Windows PE/COFF 64-bit `.obj` files.
  - Constructs `IMAGE_FILE_HEADER`, `IMAGE_SECTION_HEADER` array, relocation entries (`IMAGE_RELOCATION`), symbol table records, and `.pdata`/`.xdata` SEH unwind structures.

---

## 8.4 Mach-O 64-bit Object Writer (`MachoObjectWriter`) [TODO]
- **File:** `EzCodeEmitter/include/Writers/MachoObjectWriter.h`, `EzCodeEmitter/src/Writers/MachoObjectWriter.cpp`
- **Responsibilities:**
  - Emits Apple Mach-O 64-bit `.o` files.
  - Constructs `mach_header_64`, `LC_SEGMENT_64` commands, `section_64` descriptors, `LC_SYMTAB`, and relocation records.

---

# 9. Phase 6: End-to-End Testing & 1.0 Release Checklist

### Complete 1.0 Milestone Verification Matrix

| Phase | Milestone Item | Acceptance Criteria | Target Test Suite | Status |
|:---|:---|:---|:---|:---:|
| **Phase 1** | `.ccdf` Lexy Parser & AST | Parsing of System V & Win64 calling conventions | `T_CallingConvDefLang.cpp` | ✅ **DONE** |
| **Phase 1** | All 8 DSL Parsers & ASTs | Full syntax coverage for `.tdf`, `.idf`, `.lad`, `.lrd`, `.isf`, `.irdf`, `.tyf` | `tests/EzDslTestSuite/` (8 suites) | ✅ **DONE** |
| **Phase 1** | All 8 Sema Passes | Cross-file symbol resolution, bitfield overlap, DAG alias cycle checks | `T_Sema_*Pass.cpp` (7 suites) | ✅ **DONE** |
| **Phase 2** | Type & IR Generators | Synthesize `MirTypeTable` & `MirInstructionSet` | `T_EzDslCli_Gen*.cpp` | ✅ **DONE** |
| **Phase 2** | Target Model Generators | Synthesize registers, classes, banks, alias tables from `.tdf` | `T_EzDslCli_GenRegisterBanks.cpp` | ✅ **DONE** |
| **Phase 2** | Target Instruction Encoders | Synthesize target opcodes, descriptors, and binary encoders from `.idf` | `T_EzDslCli_GenTargetInst.cpp` | ✅ **DONE** |
| **Phase 2** | 2D Legality Action Matrix | Synthesize constant-time 2D matrix from `.lad` | `T_EzDslCli_GenLegalizer.cpp` | ✅ **DONE** |
| **Phase 2** | Rewrite Rule Engine | Synthesize expansion patterns from `.lrd` | `T_EzDslCli_GenLegalizer.cpp` | ✅ **DONE** |
| **Phase 2** | ISel Decision Tree | Synthesize AddrMode matchers & decision tree selector from `.isf` | `T_EzDslCli_GenISelTable.cpp` | ✅ **DONE** |
| **Phase 2** | Calling Convention Descs | Synthesize `CallingConvDesc` factories from `.ccdf` | `T_EzDslCli_GenCallingConv.cpp` | ✅ **DONE** |
| **Phase 2** | TargetDesc & Glue Generator | Synthesize `<Target>TargetDesc` & `<Target>TargetBinaryDesc` | `T_EzDslCli_GenTargetDesc.cpp` | ✅ **DONE** |
| **Phase 2** | Multi-Target CLI Driver | `EzDsl-cli` compiles full target bundle in one invocation | `T_EzDslCli_*.cpp` | ✅ **DONE** |
| **Phase 3** | MirVerifierPass | Strict SSA dominance & terminator verification | `T_MirVerifierPass.cpp` | ⏳ **PENDING** |
| **Phase 3** | Legalizer & ISel Engines | Generic MIR lowering to target MIR driven by generated tables | `T_MirLegalizer.cpp`, `T_MirInstructionSelector.cpp` | ✅ **DONE** |
| **Phase 4** | RegAlloc & ABI Lowering | Full Chaitin-Briggs coloring, coalescing, call/return ABI lowering | `T_MirAbiLowerer.cpp` | ✅ **DONE** |
| **Phase 4** | Frame Lowerer Engine | Frame layout calculation, stack offset rewriting, pass execution | `T_MirFrameLowerer.cpp` | ✅ **DONE** |
| **Phase 5** | Direct Object Writers | Valid ELF64, COFF64, Mach-O binary generation verified by `readelf`/`llvm-readobj` | `T_*ObjectWriter.cpp` | ⏳ **PENDING** |
| **Phase 6** | End-to-End Targets | Fully functional **x86-64 (AMD64)** and **RISC-V 64** execution | `ez-lit` Native Execution Tests | ⏳ **PENDING** |


