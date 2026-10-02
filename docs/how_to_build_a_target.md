# How to Build a Target Architecture in EzPacker

[EzPacker Documentation Index](index.md) > **How to Build a Target Architecture**

---

## 1. Architectural Philosophy & Retargetability

EzPacker is engineered for modularity and retargetability. The compiler middle-end, legalizer pipeline, register allocator, and object writers contain **zero hardcoded architecture references or `#ifdef` branches**. 

All target-dependent knowledge is completely encapsulated within self-contained directories under `EzTargets/<TargetName>/`. Adding support for a new hardware architecture (such as RISC-V, ARM64, or a custom ASIC ISA) requires authoring:
1. **Declarative DSL Specifications**: High-level descriptions of hardware registers, instructions, encodings, calling conventions, legalizer rules, and the target descriptor (`.tdesc`).
2. **Runtime C++ Components**: Subclasses implementing frame lowering, instruction selection dispatch, code emission, and relocation resolution wired together by the target descriptor.
3. **CMake Wiring & Registration**: Invoking the EzDsl CMake generators (including `EzDslGenTargetDesc`) and registering the target factory with `EzCompiler::TargetResolver`.

```
       +-----------------------------------------------------------------------+
       |                          EzTargets/<TargetName>                       |
       +-----------------------------------------------------------------------+
                     |                                           |
                     v                                           v
       +----------------------------+             +----------------------------+
       |      DSL Specifications    |             |      C++ Runtime Core      |
       |  - <target>.tdesc          |             |  - <Target>FrameLowerer    |
       |  - <target>.ezcc           |             |  - <Target>InstSelector    |
       |  - <target>_legalize.lad   |             |  - <Target>RegAllocator    |
       |  - <target>_rules.lrd      |             |  - <Target>CodeEmitter     |
       |  - <target>_instructions   |             |  - <Target>ElfBinaryDesc   |
       |  - <target>_patterns.isf   |             |  - <Target>RelocResolver   |
       +----------------------------+             +----------------------------+
                     |                                           |
                     v (EzDslCli via CMake)                      |
       +---------------------------------------------------+     |
       |               Synthesized C++ Artifacts            |     |
       |  - <Target>TargetDesc.h / .cpp  (EzDslGenTargetDesc)  <---+
       |  - <Target>RegisterInfo.h       (EzDslGenRegisterInfo)|
       |  - <Target>CallingConvDesc.h    (EzDslGenCallingConv) |
       |  - <Target>LegalizerActionTable (EzDslGenLegalizer)   |
       |  - <Target>TargetInstructions   (EzDslGenTargetInst)  |
       |  - <Target>InstructionSelector  (EzDslGenInstSelector)|
       +---------------------------------------------------+
                                     |
                                     v
                   +-----------------------------------+
                   |        Target Registration        |
                   |  TargetResolver::registerTarget   |
                   +-----------------------------------+
```

---

## 2. The 7 DSL Specifications

### 2.1 Target Descriptor: `<target>.tdesc`
Defines machine word size, stack slot size, instruction pointer register, memory displacement type, supported binary formats, register banks, register classes, sub-register mappings, CPU extensions, runtime component wiring, and libcalls. The entire `TargetDesc` C++ implementation is synthesized automatically from this manifest.

```tdesc
target RiscV64;

pointer_size: 8;
stack_slot:   8;
instruction_pointer: pc;
mem_disp_type: i64;
object_formats: [ELF];
default_calling_conv: LP64D;

register_bank GPR {
    classes { GPR64: 64, GPR32: 32 }
    sub_register { GPR64 <: GPR32 }
    registers {
        x0  enc 0  names { zero: GPR64, zero: GPR32 }
        x1  enc 1  names { ra:   GPR64, ra:   GPR32 }
        x2  enc 2  names { sp:   GPR64, sp:   GPR32 }
        x10 enc 10 names { a0:   GPR64, a0:   GPR32 }
        x11 enc 11 names { a1:   GPR64, a1:   GPR32 }
    }
}

extensions {
    rv64m { default: true; description: "Integer Multiplication and Division"; };
    rv64a { default: true; description: "Atomic Instructions"; };
    rv64f { default: false; description: "Single-Precision Floating-Point"; };
    rv64d { implies: [rv64f]; description: "Double-Precision Floating-Point"; };
}

components {
    frame_lowerer:        RiscV64FrameLowerer;
    instruction_selector: RiscV64InstructionSelector;
    register_allocator:   RiscV64RegisterAllocator;
    code_emitter:         RiscV64CodeEmitter;
    relocation_resolver:  RiscV64RelocationResolver
}

libcalls {
    __returnNothing;
    __divdi3;
    __udivdi3;
}
```
> [!NOTE]
> The `.tdesc` `libcalls` block lists target-specific or legacy libcall symbols synthesized into index-based lookup tables (`getLibcallStr(uint8_t)`). Target descriptors also integrate with `TargetLibcallRegistry` (`getLibcallRegistry()`), providing full support for compiler-rt and C runtime functions across CRT flavors (`Gnu`, `Msvc`, `Musl`, `Darwin`, `Freestanding`).

### 2.2 Calling Convention: `<target>_calling_conv.ezcc`
Defines stack frame alignment, growth direction, shadow space, red zone, callee-saved registers, parameter passing rules, and return registers.

```ezcc
calling_convention LP64D {
    stack {
        align: 16,
        growth: down,
        cleanup: caller,
        shadow_space: 0,
        red_zone: 0,
        sp: sp,
        fp: s0
    }

    preserve callee: [s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, sp]
    preserve caller: [ra, t0, t1, t2, t3, t4, t5, t6, a0, a1, a2, a3, a4, a5, a6, a7]

    classify {
        types [i8, i16, i32, i64, ptr] => integer
        types [f32, f64] => float
    }

    arguments {
        pass integer => seq([a0, a1, a2, a3, a4, a5, a6, a7]), fallback: stack(8)
        pass float   => seq([fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7]), fallback: stack(8)
    }

    returns {
        pass integer => seq([a0, a1])
        pass float   => seq([fa0, fa1])
    }
}
```

### 2.3 Legalization Actions: `<target>_legalize.lad`
Declares the target's primary legality matrix for generic MIR opcodes:

```lad
target RiscV64;

type_set GPR_SCALARS = (i32, i64);

action MOV {
    LEGAL(GPR_SCALARS, ptr, f32, f64);
    WIDENS(i1, i8, i16) >> i32;
};

action ADD {
    LEGAL(i32, i64);
    WIDENS(i1, i8, i16) >> i32;
    NARROWS(i128) >> i64;
};

action SDIV {
    LEGAL(i32, i64);
    LIBCALL(i128);
};
```

### 2.4 Legalization Rules: `<target>_rules.lrd`
Declares pattern-matching strength reduction rewrites:

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
```

### 2.5 Target Instructions: `<target>_instructions.idf`
Declares concrete machine instructions with mnemonics, operands, flags, and binary encodings:

```idf
target RiscV64;

target_inst ADD64rr(GPR64:dst OUT, GPR64:src1 IN, GPR64:src2 IN) {
    MNEMONIC("add");
    FLAGS(IsCommutative);
    ENCODING {
        form: r_type;
        opcode: [0x33];
        funct3: 0x0;
        funct7: 0x00;
        operands { dst => rd; src1 => rs1; src2 => rs2; };
    };
};

target_inst ADDI64ri(GPR64:dst OUT, GPR64:src1 IN, i64:imm IN) {
    MNEMONIC("addi");
    ENCODING {
        form: i_type;
        opcode: [0x13];
        funct3: 0x0;
        operands { dst => rd; src1 => rs1; imm => imm12_signed; };
    };
};
```

### 2.6 Instruction Selection Patterns: `<target>_patterns.isf`
Defines tree-matching rewrite patterns and addressing modes for Bottom-Up Maximal Munch:

```isf
target RiscV64;

addrmode AddrModeRegImm(GPR64:base, simm12:disp = 0) {
    variant BaseDisp { match { ADD ptr:$base, imm(i32):$disp; }; };
    variant BaseOnly { match { ptr:$base; }; };
};

pattern Select_ADD64ri [cost = 1] {
    match {
        ADD i64:$dst, i64:$src1, imm(i64):$imm;
    };
    when {
        isSimm12($imm);
    };
    select {
        ADDI64ri GPR64:$dst, GPR64:$src1, $imm;
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

## 3. Implementing the C++ Runtime Classes

### 3.1 Synthesizing the `TargetDesc` (`EzDslGenTargetDesc`)
Rather than maintaining a verbose, handwritten `TargetDesc` subclass, EzPacker generates `<Target>TargetDesc.h` and `<Target>TargetDesc.cpp` directly from `<target>.tdesc` via `EzDslGenTargetDesc` (or `EzDslCli --emit-target-desc`).

The generated descriptor inherits from `TargetDesc` (`EzTriple`) and automatically implements all required virtual interfaces:
- **Component Wiring**: Lazily instantiates and returns the runtime components declared in the `.tdesc` `components` block (`getFrameLowerer()`, `getInstructionSelector()`, `getRegisterAllocator()`, `createCodeEmitter()`, `getRelocationResolver()`).
- **Register Banks & Classes**: Initializes register banks and primary register classes (`getGprClass()`) via `initializeRegisterBanks` using metadata from the `.tdesc`.
- **Target Metadata**: Implements `getName()`, `getStackSlotSize()`, `getInstructionPtrReg()`, and `getMemOperandDisplacementType()`.
- **Binary Descriptors & Calling Conventions**: Populates `getAvailableBinaryDescriptors()` and `getAvailableCallingConventions()` matching the target's declared object formats and calling conventions.
- **Libcall Registry**: Automatically instantiates a per-target `TargetLibcallRegistry` (accessible via `getLibcallRegistry()`), initialized with compiler-rt and CRT default symbols based on the target OS/CRT flavor (`Gnu`, `Msvc`, `Musl`, `Darwin`, `Freestanding`), and provides `getLibcallStr(LibcallKind)` for type-safe runtime symbol lookups.

```cpp
// Auto-generated snippet from <Target>TargetDesc.h:
namespace EzTargets::TableGen::RiscV64 {
class RiscV64TargetDesc : public TargetDesc {
public:
    explicit RiscV64TargetDesc(MirBuilderContext *ctx);
    ~RiscV64TargetDesc() override;

    const char *getName() const override { return "riscv64"; }
    size_t getStackSlotSize() const override { return 8; }
    MirRegisterRef getInstructionPtrReg() const override;
    MirType *getMemOperandDisplacementType() override;

    MirFrameLowerer *getFrameLowerer() override;
    MirInstructionSelector *getInstructionSelector() override;
    MirRegisterAllocator *getRegisterAllocator() override;
    MirLegalizer *getLegalizer() override;
    LegalizerInfo *getLegalizerInfo() override;
    TargetRelocationResolver *getRelocationResolver() override;
    std::unique_ptr<GenericCodeEmitter> createCodeEmitter() override;

    void initialize() override;
    std::string_view getLibcallStr(uint8_t symId) override;
    std::string_view getLibcallStr(LibcallKind kind) override;
    TargetLibcallRegistry *getLibcallRegistry() override;
    const std::pmr::vector<TargetBinaryDesc *> &getAvailableBinaryDescriptors() override;
    const std::pmr::vector<CallingConvDesc *> &getAvailableCallingConventions() override;
};
}

// Backward-compatible alias into the target namespace:
namespace EzTargets::RiscV64 {
    using RiscV64TargetDesc = EzTargets::TableGen::RiscV64::RiscV64TargetDesc;
}
```

### 3.2 Subclassing `MirFrameLowerer`
Implement prologue/epilogue insertion and stack allocation lowering:
```cpp
#include "FrameLowerer/MirFrameLowerer.h"

class RiscV64FrameLowerer : public MirFrameLowerer {
public:
    bool lowerAlloc(FrameLowererCtx &ctx) override {
        // Lower static ALLOC instructions into StackFrameObject entries
        return true;
    }

    bool lowerDAlloc(FrameLowererCtx &ctx) override {
        // Lower dynamic DALLOC instructions into stack pointer decrements
        return true;
    }

    void calculateFrameLayout(FrameLowererCtx &ctx) override {
        // Compute frame size, align to 16 bytes, assign callee-save offsets
    }

    void insertPrologue(FrameLowererCtx &ctx) override {
        // Emit: addi sp, sp, -FrameSize
        // Emit: sd ra, (FrameSize - 8)(sp)
        // Emit: sd s0, (FrameSize - 16)(sp)
        // Emit: addi s0, sp, FrameSize
    }

    void insertEpilogue(FrameLowererCtx &ctx) override {
        // Emit: ld s0, (FrameSize - 16)(sp)
        // Emit: ld ra, (FrameSize - 8)(sp)
        // Emit: addi sp, sp, FrameSize
        // Emit: jalr zero, 0(ra)
    }
};
```

### 3.3 Subclassing `GenericCodeEmitter`
Implement binary instruction encoding:
```cpp
#include "GenericCodeEmitter.h"

class RiscV64CodeEmitter : public GenericCodeEmitter {
public:
    void beginFunction(CodeEmitterContext *ctx, std::string_view name) override;
    void beginFunction(CodeEmitterContext *ctx, MirFunction *func) override;
    void bindLabel(MirId labelId) override;
    void endFunction(CodeEmitterContext *ctx) override;
    void endFunction(CodeEmitterContext *ctx, MirFunction *func) override;
    void emitInst(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands) override {
        // Interpret target descriptor, encode machine word (R, I, S, B, U, J type),
        // and emit into the active section buffer via ctx->getCurrentSection()->emit32(...)
    }
};
```

---

## 4. CMake Integration

In `EzTargets/<TargetName>/CMakeLists.txt`, use the provided `EzDslGen` macros to synthesize tables at build time:

```cmake
include("${CMAKE_SOURCE_DIR}/EzTargets/CMake/EzDslGenTargetDesc.cmake")
include("${CMAKE_SOURCE_DIR}/EzTargets/CMake/EzDslGenCallingConv.cmake")
include("${CMAKE_SOURCE_DIR}/EzTargets/CMake/EzDslGenLegalizerActionTable.cmake")
include("${CMAKE_SOURCE_DIR}/EzTargets/CMake/EzDslGenTargetInstructions.cmake")
include("${CMAKE_SOURCE_DIR}/EzTargets/CMake/EzDslGenInstructionSelector.cmake")
include("${CMAKE_SOURCE_DIR}/EzTargets/CMake/EzDslGenRegisterInfo.cmake")

# 1. Target Descriptor (<Target>TargetDesc.h / .cpp)
EzDslGenTargetDesc(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64.tdesc"
    TARGET_NAME "RiscV64"
    NAMESPACE_ROOT "EzTargets::RiscV64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

# 2. Register Info & Banks
EzDslGenRegisterInfo(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64.tdesc"
    TARGET_NAME "RiscV64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

# 3. Calling Conventions
EzDslGenCallingConv(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_calling_conv.ezcc"
    TARGET_NAME "RiscV64"
    NAMESPACE_ROOT "EzTargets::RiscV64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

# 4. Legalizer Tables
EzDslGenLegalizerActionTable(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_legalize.lad"
    RULES  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_rules.lrd"
    TARGET_NAME "RiscV64"
    NAMESPACE_ROOT "EzTargets::RiscV64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

# 5. Target Instructions
EzDslGenTargetInstructions(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_instructions.idf"
    TARGET_NAME "RiscV64"
    NAMESPACE_ROOT "EzTargets::RiscV64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

# 6. Instruction Selector
EzDslGenInstructionSelector(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_patterns.isf"
    TARGET_NAME "RiscV64"
    NAMESPACE_ROOT "EzTargets::RiscV64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)
```

Add your target subdirectory to `EzTargets/CMakeLists.txt`:
```cmake
add_subdirectory(X86_64)
add_subdirectory(RiscV64) # Added new target
```

---

## 5. Target Registration Hook

In `Registration/RiscV64TargetRegistration.cpp`, register your factory function with `TargetResolver`:

```cpp
#include "TargetResolver.h"
#include "RiscV64TargetDesc.h"

namespace EzTargets::RiscV64 {

void registerTarget() {
    EzCompiler::TargetResolver::registerTarget("riscv64",
        [](const EzCompiler::TargetTriple &triple,
           MirBuilderContext *mirCtx,
           bool isPositionIndependent,
           const std::vector<std::string> &features) -> EzCompiler::ResolvedTarget
        {
            auto target = std::make_unique<RiscV64TargetDesc>(mirCtx);
            target->setPositionIndependent(isPositionIndependent);
            target->applyFeatures(features);
            target->initialize();

            EzCompiler::ResolvedTarget result;
            result.m_callingConv = target->getDefaultCallingConv();
            result.m_binaryDesc = target->getElfBinaryDesc();
            result.m_targetDesc = std::move(target);
            return result;
        });
}

} // namespace EzTargets::RiscV64

namespace {
    struct RiscV64AutoReg {
        RiscV64AutoReg() { EzTargets::RiscV64::registerTarget(); }
    };
    const RiscV64AutoReg g_autoReg;
}
```

---

## 6. Testing Your Target

Once registered, you can immediately compile MIR modules to your new target using `-target`:

```bash
EzCompiler examples/arithmetic_32bit.mir --target riscv64-unknown-linux-elf --emit-asm
```

Verify your implementation by adding unit tests under `tests/` mirroring the x86-64 test suite (`T_MirLegalizer.cpp`, `T_MirInstructionSelector.cpp`, `T_MirRegisterAllocator.cpp`).

---

## 7. Next Steps

- Study the reference x86-64 target in [EzTargets Subproject Documentation](projects/EzTargets.md).
- Browse the [Doxygen API Reference](doxygen/index.html).
- Return to the [EzPacker Landing Page](index.md).
