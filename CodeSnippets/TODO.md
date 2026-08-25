# EzPacker Backend 1.0 Complete Engineering Specification & Step-by-Step Implementation Guide

This document contains the **in-depth implementation details, architectural trade-offs, semantic analysis algorithms, C++ code generation specifications, and concrete test plans** to deliver a **stable 1.0 release** of the EzPacker compiler backend.

---

# Table of Contents
1. [Master Architecture Pipeline](#1-master-architecture-pipeline)
2. [Architectural Decisions & Trade-Offs (Pros & Cons)](#2-architectural-decisions--trade-offs-pros--cons)
3. [Phase 1: EzDSL Semantic Analysis & Calling Convention DSL](#3-phase-1-ezdsl-semantic-analysis--calling-convention-dsl)
   - ~~[1.1 Calling Convention DSL (`.ccdf`) Parser & AST](#11-calling-convention-dsl-ccdf-parser--ast)~~
   - [1.2 Unified Symbol Table & Cross-Language Resolution](#12-unified-symbol-table--cross-language-resolution)
   - [1.3 Target Definition Semantic Pass (`TargetDefPass`)](#13-target-definition-semantic-pass-targetdefpass)
   - [1.4 Target Instruction Semantic Pass (`InstructionDefPass`)](#14-target-instruction-semantic-pass-instructiondefpass)
   - [1.5 Legalization Action Semantic Pass (`LegalizeActionPass`)](#15-legalization-action-semantic-pass-legalizeactionpass)
   - [1.6 Legalization Rewrite Rule Semantic Pass (`LegalizeRulePass`)](#16-legalization-rewrite-rule-semantic-pass-legalizerulepass)
   - [1.7 Instruction Selection Semantic Pass (`InstructionSelPass`)](#17-instruction-selection-semantic-pass-instructionselpass)
   - [1.8 Calling Convention Semantic Pass (`CallingConvPass`)](#18-calling-convention-semantic-pass-callingconvpass)
4. [Phase 2: EzDSL C++ Code Generators](#4-phase-2-ezdsl-c-code-generators)
   - [2.1 Target Instruction & Format Binary Encoder (`CppTargetInstGenerator`)](#21-target-instruction--format-binary-encoder-cpptargetinstgenerator)
   - [2.2 2D Legality Action Matrix Generator (`CppLegalizerGenerator`)](#22-2d-legality-action-matrix-generator-cpplegalizergenerator)
   - [2.3 Legalization Rewrite Rule Engine Generator (`CppLegalizerRuleGenerator`)](#23-legalization-rewrite-rule-engine-generator-cpplegalizerrulegenerator)
   - [2.4 Multi-Variant ISel Table Generator (`CppISelTableGenerator`)](#24-multi-variant-isel-table-generator-cppiseltablegenerator)
   - [2.5 Calling Convention Generator (`CppCallingConvGenerator`)](#25-calling-convention-generator-cppcallingconvgenerator)
   - [2.6 Driver CLI & CMake Integration (`EzDsl-cli`)](#26-driver-cli--cmake-integration-ezdsl-cli)
5. [Phase 3: EzMir Core & Execution Engines](#5-phase-3-ezmir-core--execution-engines)
   - [3.1 Function-Scoped Monotonic Arenas (`MirFunction`)](#31-function-scoped-monotonic-arenas-mirfunction)
   - [3.2 MIR Invariant Verifier Pass (`MirVerifierPass`)](#32-mir-invariant-verifier-pass-mirverifierpass)
   - [3.3 Generic Legalizer Engine (`MirLegalizerPass`)](#33-generic-legalizer-engine-mirlegalizerpass)
   - [3.4 Generic Instruction Selector Engine (`MirInstructionSelectorPass`)](#34-generic-instruction-selector-engine-mirinstructionselectorpass)
6. [Phase 4: EzTriple & Backend Hardening](#6-phase-4-eztriple--backend-hardening)
   - [4.1 Chaitin-Briggs Register Coalescing & Allocation Hardening](#41-chaitin-briggs-register-coalescing--allocation-hardening)
   - [4.2 Dynamic Alloca & ABI Red Zone Handling (`MirFrameLowerer`)](#42-dynamic-alloca--abi-red-zone-handling-mirframelowerer)
7. [Phase 5: EzCodeEmitter & Direct Object Writers](#7-phase-5-ezcodeemitter--direct-object-writers)
   - [5.1 ELF64 Object Writer (`ElfObjectWriter`)](#51-elf64-object-writer-elfobjectwriter)
   - [5.2 PE/COFF64 Object Writer (`CoffObjectWriter`)](#52-pecoff64-object-writer-coffobjectwriter)
   - [5.3 Mach-O 64-bit Object Writer (`MachoObjectWriter`)](#53-mach-o-64-bit-object-writer-machoobjectwriter)
8. [Phase 6: End-to-End Testing & 1.0 Release Checklist](#8-phase-6-end-to-end-testing--10-release-checklist)

---

# 1. Master Architecture Pipeline

```
 ┌───────────────────────────────────────────────────────────────────────────────────────────┐
 │                                      EzDSL Frontend                                       │
 │   .tdf (Targets)   │   .idf (Insts)    │   .lad (Legality)  │   .lrd (Rules)  │  .isf    │
 └─────────────────────────────────────────────┬─────────────────────────────────────────────┘
                                               │ Multi-Pass Sema Validation & Symbol Interning
                                               ▼
 ┌───────────────────────────────────────────────────────────────────────────────────────────┐
 │                                   EzDSL Code Generators                                   │
 │  - CppTargetInstGenerator           - CppLegalizerGenerator (Action Table & Rewriter)     │
 │  - CppISelTableGenerator            - CppCallingConvGenerator                             │
 └─────────────────────────────────────────────┬─────────────────────────────────────────────┘
                                               │ C++ Generated Tables, Matchers & Encoders
                                               ▼
 ┌───────────────────────────────────────────────────────────────────────────────────────────┐
 │                              EzMir & EzTriple 1.0 Pipeline                                │
 │                                                                                           │
 │  Generic MIR (SSA Form)                                                                   │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [MirVerifierPass] ─────────── Verify SSA Invariants, Dominance & Terminators             │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [MirLegalizerPass] ────────── Driven by Generated .lad Action Tables & .lrd Rewriters    │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [MirInstructionSelectorPass]  Driven by Generated .isf Matchers & AddrModes              │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [MirAbiLowererPass] ───────── Driven by Generated Calling Convention Descriptors         │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [LivenessAnalysisPass] ────── Def-Use Chains, Live-In & Live-Out Sets                    │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [MirRegisterAllocatorPass] ── Chaitin-Briggs Graph Coloring, Coalescing & Spilling       │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [MirFrameLowererPass] ─────── Prologue/Epilogue Insertion & Offset Resolution            │
 │         │                                                                                 │
 │         ▼                                                                                 │
 │  [EzCodeEmitter] ───────────── Node-based Section Finalization & Direct Object Writers    │
 └───────────────────────────────────────────────────────────────────────────────────────────┘
```

---

# 2. Architectural Decisions & Trade-Offs (Pros & Cons)

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

# 3. Phase 1: EzDSL Semantic Analysis & Calling Convention DSL

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

---

# 4. Phase 2: EzDSL C++ Code Generators

---

## 2.1 Target Instruction & Format Binary Encoder (`CppTargetInstGenerator`)

### Generator Logic & Output Format
```cpp
// Emits <Target>InstructionDefs.h
void CppTargetInstGenerator::emitHeader(std::ostream &os, SymbolTable *table) {
    os << "enum class TargetOpCode : uint16_t {\n";
    for (auto *sym : table->getSymbolsByType(SymbolType::TargetInstruction)) {
        os << "    " << sym->getName() << ",\n";
    }
    os << "    TARGET_OPCODE_COUNT\n};\n";
}

// Emits <Target>BinaryEncoder.cpp
void CppTargetInstGenerator::emitEncoders(std::ostream &os, SymbolTable *table) {
    for (auto *sym : table->getSymbolsByType(SymbolType::TargetInstruction)) {
        auto *instSym = sym->getIf<TargetInstructionSymbol>();
        auto *fmtSym = table->getSymById(instSym->m_formatId)->getIf<InstructionFormatSymbol>();
        
        os << "void Encode_" << sym->getName() << "(CodeSection *sec, const MirInstruction *inst) {\n";
        os << "    uint" << fmtSym->m_bitWidth << "_t binaryWord = 0;\n";
        
        // Emit bitfield assignments
        for (const auto &field : fmtSym->m_fields) {
            // Synthesize bit shift: binaryWord |= (val & mask) << field.m_startBit;
            os << "    // Field: " << field.m_name << " [" << field.m_startBit << ":" << field.m_endBit << "]\n";
        }
        os << "    sec->emit" << fmtSym->m_bitWidth << "(binaryWord);\n";
        os << "}\n";
    }
}
```

---

## 2.2 2D Legality Action Matrix Generator (`CppLegalizerGenerator`)

### Generator Logic & Output Format
```cpp
// Emits <Target>LegalizerActionTable.cpp
void CppLegalizerGenerator::emitSource(std::ostream &os, SymbolTable *table) {
    os << "static const LegalizeAction g_LegalizeMatrix[OPCODE_COUNT][MAX_TYPE_SLOTS][MAX_TYPES] = {\n";
    for (size_t op = 0; op < OPCODE_COUNT; ++op) {
        os << "  { // Opcode: " << g_OpcodeNames[op] << "\n";
        for (size_t slot = 0; slot < MAX_TYPE_SLOTS; ++slot) {
            os << "    { ";
            for (size_t t = 0; t < MAX_TYPES; ++t) {
                LegalizeAction action = ResolveAction(op, slot, t);
                os << "LegalizeAction::" << ToString(action) << ", ";
            }
            os << "},\n";
        }
        os << "  },\n";
    }
    os << "};\n";
}
```

---

## 2.3 Legalization Rewrite Rule Engine Generator (`CppLegalizerRuleGenerator`)

### Generated Pattern Rewriter
```cpp
bool TargetLegalizeRules::tryExpand(MirBuilderContext *ctx, MirInstruction *inst) {
    switch (inst->getOpcode()) {
        case MirInstructionOpCode::ADD:
            if (inst->getType()->getTotalSizeInBits() == 64 && TargetHooks::isSubtarget32Bit(ctx)) {
                return Expand_NarrowAddi64(ctx, inst);
            }
            break;
        // ...
    }
    return false;
}
```

---

## 2.4 Multi-Variant ISel Table Generator (`CppISelTableGenerator`)

### Generated Decision Tree Selector
```cpp
bool TargetInstructionSelector::select(MirBuilderContext *ctx, MirInstruction *inst) {
    switch (inst->getOpcode()) {
        case MirInstructionOpCode::LOAD: {
            MirOperand *dst = inst->getOperand(0);
            MirOperand *addr = inst->getOperand(1);
            
            // Check Pattern: Select_LW
            AddrModeRegImm12Result am;
            if (Match_AddrModeRegImm12(addr, am)) {
                MirInstructionBuilder b(ctx, inst->getOwner(), InsertionType::InsertBefore, inst);
                b.buildTarget(TargetOpCode::LW, { dst, am.base, am.offset });
                inst->eraseFromParent();
                return true;
            }
            break;
        }
    }
    return false;
}
```

---

## 2.5 Calling Convention Generator (`CppCallingConvGenerator`)

### Generated ABI Initializer
```cpp
CallingConvDesc *Create_SystemV_AMD64(std::pmr::memory_resource *alloc) {
    auto *cc = new (alloc->allocate(sizeof(CallingConvDesc))) CallingConvDesc("SystemV_AMD64", alloc);
    cc->setStackAlignment(16);
    cc->setShadowSpaceSize(0);
    cc->setRedZoneSize(128);
    // Populate callee-saved: RBX, RSP, RBP, R12-R15
    // Populate caller-saved: RAX, RCX, RDX, RSI, RDI, R8-R11
    // Populate argument rules & SRET rules
    return cc;
}
```

---

## 2.6 Driver CLI & CMake Integration (`EzDsl-cli`)

### CMake Packaging (`EzMir/CMake/EzDslGenBackend.cmake`)
```cmake
function(EzDslGenBackend)
    cmake_parse_arguments(ARG "" "TARGET;OUTPUT_DIR" "TDF_FILES;IDF_FILES;LAD_FILES;LRD_FILES;ISF_FILES;CCDF_FILES" ${ARGN})
    
    add_custom_command(
        OUTPUT 
            ${ARG_OUTPUT_DIR}/TargetInstructionDefs.h
            ${ARG_OUTPUT_DIR}/TargetInstructionTable.cpp
            ${ARG_OUTPUT_DIR}/TargetLegalizerActionTable.cpp
            ${ARG_OUTPUT_DIR}/TargetISelTable.cpp
            ${ARG_OUTPUT_DIR}/TargetCallingConventions.cpp
        COMMAND EzDsl-cli 
            --target-def ${ARG_TDF_FILES}
            --inst-def ${ARG_IDF_FILES}
            --legalize-actions ${ARG_LAD_FILES}
            --legalize-rules ${ARG_LRD_FILES}
            --isel-patterns ${ARG_ISF_FILES}
            --calling-conv ${ARG_CCDF_FILES}
            -o ${ARG_OUTPUT_DIR}
        DEPENDS EzDsl-cli ${ARG_TDF_FILES} ${ARG_IDF_FILES} ${ARG_LAD_FILES} ${ARG_LRD_FILES} ${ARG_ISF_FILES} ${ARG_CCDF_FILES}
    )
endfunction()
```

---

# 5. Phase 3: EzMir Core & Execution Engines

---

## 3.1 Function-Scoped Monotonic Arenas (`MirFunction`)

### Implementation
```cpp
class MirFunction {
public:
    MirFunction(std::string_view name, MirType *funcType, std::pmr::memory_resource *parentAlloc) :
        m_name(name), m_funcType(funcType), m_arena(parentAlloc) {}
    
    std::pmr::memory_resource *getAllocator() { return &m_arena; }
    
private:
    std::pmr::monotonic_buffer_resource m_arena;
};
```

---

## 3.2 MIR Invariant Verifier Pass (`MirVerifierPass`)

### Validation Rules
1. **Dominance**: For every instruction $I$ reading virtual register $V$, the unique defining instruction $D(V)$ must dominate $I$.
2. **Terminators**: Basic block must have exactly one terminator (`isTerminator` flag) at the end.
3. **$\phi$ Invariants**: $\phi$ instructions must only reside at block headers, with operand count matching predecessor count.

---

## 3.3 Generic Legalizer Engine (`MirLegalizerPass`)

### Execution Pipeline
1. Scan instructions for illegal type combinations using `TargetLegalizerActionTable`.
2. Apply `WidenScalar`: insert `SEXT`/`ZEXT`, widen operation, insert `TRUNC`.
3. Apply `NarrowScalar`: insert `UNMERGE_VALUES`, lower scalar halves with carry operations, insert `MERGE_VALUES`.
4. Apply `Libcall`: lower to `PUSH_ARG` + runtime function `CALL`.
5. Apply `Custom`: invoke `TargetLegalizeRules::tryExpand()`.

---

## 3.4 Generic Instruction Selector Engine (`MirInstructionSelectorPass`)

### Execution Pipeline
1. Topological reverse traversal over basic blocks.
2. Evaluate pattern matchers with highest cost reduction first (`cost(N)`).
3. Replace generic instructions with target instruction nodes with physical register class constraints.

---

# 6. Phase 4: EzTriple & Backend Hardening

---

## 4.1 Chaitin-Briggs Register Coalescing & Allocation Hardening

### Register Coalescing Algorithm (Briggs Criterion)
- For every copy `MOV %dst, %src`:
  - Combine nodes $U$ and $V$ into $UV$ if the merged node has $< K$ neighbors of significant degree ($\ge K$).
  - Eliminate redundant `MOV` instruction.

### Loop-Depth Weighted Spilling
$$\text{Cost}(v) = \frac{\sum_{u \in \text{uses}(v)} 10^{\text{loopDepth}(u)} + \sum_{d \in \text{defs}(v)} 10^{\text{loopDepth}(d)}}{\text{degree}(v)}$$

---

## 4.2 Dynamic Alloca & ABI Red Zone Handling (`MirFrameLowerer`)

### `lowerDAlloc` Implementation
```cpp
bool MirFrameLowerer::lowerDAlloc(FrameLowererCtx &ctx) {
    // 1. Force Frame Pointer usage for function
    ctx.m_targetFunc->setRequiresFramePointer(true);
    // 2. Emit SP alignment & stack subtraction
    // sub rsp, allocSize
    // and rsp, -alignment
    // mov %dest, rsp
    return true;
}
```

---

# 7. Phase 5: EzCodeEmitter & Direct Object Writers

---

## 7.1 ELF64 Object Writer (`ElfObjectWriter`)
- Generates standard System V AMD64 ELF `.o` files.
- Writes `Elf64_Ehdr`, `.text`, `.rodata`, `.data`, `.bss`, `.symtab`, `.strtab`, `.rela.text`.

## 7.2 PE/COFF64 Object Writer (`CoffObjectWriter`)
- Generates Microsoft Windows PE/COFF `.obj` files.
- Writes `IMAGE_FILE_HEADER`, `.text`, `.rdata`, `.data`, `.pdata` (x64 SEH unwind info), symbol table, relocation records.

## 7.3 Mach-O 64-bit Object Writer (`MachoObjectWriter`)
- Generates Apple Mach-O `.o` files.
- Writes `mach_header_64`, `LC_SEGMENT_64`, `__TEXT/__text`, `__DATA/__data`, `LC_SYMTAB`.

---

# 8. Phase 6: End-to-End Testing & 1.0 Release Checklist

### Complete 1.0 Milestone Verification Matrix

| Phase | Milestone Item | Acceptance Criteria | Target Test Suite |
|:---|:---|:---|:---|
| **Phase 1** | `.ccdf` Lexy Parser & AST | Full parsing of System V & Win64 calling conventions | `T_CallingConvDefLang.cpp` |
| **Phase 1** | Sema Passes | Cross-file symbol resolution, bitfield overlap, DAG alias cycle checks | `T_Sema_*Pass.cpp` (6 test suites) |
| **Phase 2** | Code Generators | Target instruction encoders, 2D legality matrices, ISel decision trees | `T_Gen_*` (5 test suites) |
| **Phase 3** | MirVerifierPass | Strict SSA dominance & terminator verification | `T_MirVerifierPass.cpp` |
| **Phase 3** | Legalizer & ISel Engines | End-to-end SSA lowering from generic MIR to target MIR | `T_MirLegalizer_Integration.cpp`, `T_MirISel_Integration.cpp` |
| **Phase 4** | RegAlloc & Frame | Chaitin-Briggs coloring, coalescing, loop-depth spill, `alloca` | `T_MirRegisterAllocator_*` |
| **Phase 5** | Direct Object Writers | Valid ELF, COFF, and Mach-O binary generation parsed by `readelf`/`llvm-readobj` | `T_*ObjectWriter.cpp` |
| **Phase 6** | End-to-End Targets | Fully functional **x86-64** and **RISC-V 64** backend execution | `ez-lit` & Native Execution Tests |
