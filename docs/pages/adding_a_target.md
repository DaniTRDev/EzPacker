# Adding a New Target Architecture {#adding_a_target}

This masterclass guide walks step-by-step through porting **EzPacker** to a completely new hardware target architecture (such as **RISC-V 64**, **ARM64**, or a custom domain-specific processor) using the existing `x86_64` backend as a real-world reference.

---

## 🏗️ Retargeting Architecture & Repository Layout

EzPacker strictly decouples hardware details from target-independent optimization. Adding a new architecture consists of two tightly coupled stages:
1. **Declarative Specification (`EzDsl`)**: High-level modeling of registers, instructions, calling conventions, legalization tables, and instruction-selection tree patterns.
2. **C++ Concrete Backend (`EzTargets`)**: Inheriting from base interfaces to implement frame lowering, instruction selection fallbacks, binary encoding, and relocation resolution.

### Target Directory Layout Standards
Every architecture backend resides under `EzTargets/<Arch>/`:

```
EzTargets/
└── MyArch/
    ├── CMakeLists.txt
    ├── include/
    │   ├── MyArchCommon.h
    │   ├── MyArchTargetDesc.h
    │   ├── MyArchFrameLowerer.h
    │   ├── MyArchTargetInstructionSelector.h
    │   ├── MyArchRelocationResolver.h
    │   └── MyArchCodeEmitter.h
    ├── src/
    │   ├── MyArchTargetDesc.cpp
    │   ├── MyArchFrameLowerer.cpp
    │   ├── MyArchTargetInstructionSelector.cpp
    │   ├── MyArchRelocationResolver.cpp
    │   ├── MyArchCodeEmitter.cpp
    │   ├── MyArchElfBinaryDesc.cpp
    │   └── MyArchCoffBinaryDesc.cpp
    ├── Registration/
    │   ├── include/EzTargetsMyArchRegistration.h
    │   └── MyArchTargetRegistration.cpp
    └── targets/
        └── myarch/
            ├── myarch.tdesc               # Architecture & register bank descriptor
            ├── myarch_instructions.idf    # Target instruction mnemonics & encodings
            ├── myarch_calling_conv.ezcc   # ABI stack layout & parameter conventions
            ├── myarch_legalize.lad        # Legality actions & scalar clamps
            ├── myarch_rules.lrd           # Multi-word rewrite rules
            └── myarch_patterns.isf        # Tree pattern instruction selector rules
```

---

## 📝 Phase 1: Declarative Target Modeling (`EzDsl`)

### 1. The Target Descriptor (`myarch.tdesc`)
The `.tdesc` file defines basic machine properties, supported object file formats, and the hardware register banks:

```dsl
target myarch;

pointer_size: 8;
stack_slot: 8;
instruction_pointer: pc;
mem_disp_type: i64;
object_formats: [elf64, coff];
default_calling_conv: myarch_default_cc;

register_bank GPR {
    classes {
        class gpr64 [r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15];
        class gpr32 [r0d, r1d, r2d, r3d, r4d, r5d, r6d, r7d, r8d, r9d, r10d, r11d, r12d, r13d, r14d, r15d];
    }

    sub_register {
        gpr32 <: gpr64;
    }

    registers enc {
        r0  : 0,  r1  : 1,  r2  : 2,  r3  : 3,
        r4  : 4,  r5  : 5,  r6  : 6,  r7  : 7,
        r8  : 8,  r9  : 9,  r10 : 10, r11 : 11,
        r12 : 12, r13 : 13, r14 : 14, r15 : 15;
    }

    names {
        r0  : "x0",  r1  : "ra",  r2  : "sp",  r3  : "gp",
        r4  : "tp",  r5  : "t0",  r6  : "t1",  r7  : "t2",
        r8  : "s0",  r9  : "s1",  r10 : "a0",  r11 : "a1",
        r12 : "a2",  r13 : "a3",  r14 : "a4",  r15 : "a5";
    }

    special {
        sp: r2;
        fp: r8;
        ip: pc;
        zero: r0;
        lr: r1;
    }
}
```

---

### 2. Target Instruction Definitions (`myarch_instructions.idf`)
Define each target machine instruction with operand directions (`IN`, `OUT`, `INOUT`), behavioral flags, implicit register dependencies, and binary format templates:

```dsl
target myarch;

target_inst ADD_RR {
    MNEMONIC "add"
    FLAGS { IsCommutative }
    operands {
        OUT gpr64 $rd,
        IN  gpr64 $rs1,
        IN  gpr64 $rs2
    }
    ENCODING {
        opcode: 0x33,
        funct3: 0x0,
        funct7: 0x00
    }
}

target_inst ADDI {
    MNEMONIC "addi"
    FLAGS { SizeMatch }
    operands {
        OUT gpr64 $rd,
        IN  gpr64 $rs1,
        IN  simm12 $imm
    }
    ENCODING {
        opcode: 0x13,
        funct3: 0x0
    }
}

target_inst LD {
    MNEMONIC "ld"
    FLAGS { ReadsMemory }
    operands {
        OUT gpr64 $rd,
        IN  gpr64 $base,
        IN  simm12 $disp
    }
}

target_inst SD {
    MNEMONIC "sd"
    FLAGS { WritesMemory }
    operands {
        IN gpr64 $rs2,
        IN gpr64 $base,
        IN simm12 $disp
    }
}

target_inst JALR {
    MNEMONIC "jalr"
    FLAGS { IsCall }
    IMPLICIT_DEFS { r1 } ; Link register
    operands {
        IN gpr64 $target
    }
}
```

---

### 3. Calling Convention Definitions (`myarch_calling_conv.ezcc`)
Define the Application Binary Interface (ABI): stack layout, argument passing registers, return registers, and callee-saved preservation sets:

```dsl
calling_conv myarch_default_cc {
    stack {
        growth downward;
        align 16;
        shadow_space 0;
        red_zone 0;
        sp r2;
        fp r8;
        lr r1;
    }

    preserve {
        callee_saved [r8, r9];           ; s0, s1
        caller_saved [r5, r6, r7, r10, r11, r12, r13, r14, r15]; ; t0-t2, a0-a5
    }

    classify {
        types [i1, i8, i16, i32, i64, ptr] -> integer;
    }

    pass {
        class integer {
            registers [r10, r11, r12, r13, r14, r15]; ; a0..a5
            fallback stack {
                align 8;
                order right_to_left;
            }
        }
    }

    return {
        class integer {
            registers [r10, r11]; ; a0, a1
        }
    }
}
```

---

### 4. Legalization Actions & Rewrite Rules (`.lad` / `.lrd`)
Declare which operand types are natively supported and how unsupported types should be transformed:

```dsl
; myarch_legalize.lad
action ADD {
    LEGAL: [i32, i64];
    WIDENS: [i1, i8, i16] >> i32;
    NARROWS: [i128] >> i64;
}

CLAMP_SCALAR i32, i64;
```

```dsl
; myarch_rules.lrd
rule SplitAdd128 {
    match (ADD i128:$lhs, i128:$rhs)
    expand {
        (UNMERGE_VALUES $lhs_lo:i64, $lhs_hi:i64, $lhs)
        (UNMERGE_VALUES $rhs_lo:i64, $rhs_hi:i64, $rhs)
        (UADDO $res_lo:i64, $carry:i1, $lhs_lo, $rhs_lo)
        (UADDE $res_hi:i64, $carry_out:i1, $lhs_hi, $rhs_hi, $carry)
        (MERGE_VALUES $dst:i128, $res_lo, $res_hi)
    }
}
```

---

### 5. Instruction Selection Patterns (`myarch_patterns.isf`)
Map generic intermediate representation instructions to native target instructions using tree pattern matching:

```dsl
target myarch;

pattern SelectAddRR {
    cost 1;
    match (ADD gpr64:$lhs, gpr64:$rhs)
    select (ADD_RR $lhs, $rhs)
}

pattern SelectAddImm {
    cost 1;
    match (ADD gpr64:$lhs, simm12:$imm)
    select (ADDI $lhs, $imm)
}
```

---

## ⚙️ Phase 2: Generating Target Stubs via EzDsl

In your target's `CMakeLists.txt`, invoke the `ezdsl_target` CMake macro to compile DSL files into C++ headers:

```cmake
# EzTargets/MyArch/CMakeLists.txt
ezdsl_target(
    TARGET_NAME EzTargetsMyArch
    TDESC targets/myarch/myarch.tdesc
    IDF targets/myarch/myarch_instructions.idf
    EZCC targets/myarch/myarch_calling_conv.ezcc
    LAD targets/myarch/myarch_legalize.lad
    LRD targets/myarch/myarch_rules.lrd
    ISF targets/myarch/myarch_patterns.isf
    OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
)
```

The generator produces:
- `MyArchTargetInstructionTable.h`: Opcode enumeration and `MirTargetInstructionDesc` tables.
- `MyArchInstructionSelector.h`: Base tree-pattern instruction selector class.
- `MyArchRegisterInfo.h`: Register banks, classes, and sub-register aliases.
- `MyArchCallingConvTable.h`: Calling convention descriptors.
- `MyArchLegalizerInfo.h`: Legalization dispatch rules.

---

## 💻 Phase 3: Implementing Concrete C++ Backend Classes

### 1. Concrete Target Descriptor (`MyArchTargetDesc`)
Inherit from `TargetDesc` to bind all components together:

```cpp
// EzTargets/MyArch/include/MyArchTargetDesc.h
#pragma once
#include "Descriptors/TargetDesc.h"
#include <memory>

namespace EzTargets::MyArch
{

class MyArchTargetDesc : public TargetDesc
{
public:
    explicit MyArchTargetDesc(MirBuilderContext *ctx);
    ~MyArchTargetDesc() override;

    const char *getName() const override { return "myarch"; }
    size_t getStackSlotSize() const override { return 8; }

    void initialize() override;

    MirFrameLowerer *getFrameLowerer() override;
    MirInstructionSelector *getInstructionSelector() override;
    MirLegalizer *getLegalizer() override;
    MirRegisterAllocator *getRegisterAllocator() override;
    std::unique_ptr<GenericCodeEmitter> createCodeEmitter() override;
    TargetRelocationResolver *getRelocationResolver() override;

private:
    std::unique_ptr<MirFrameLowerer> m_frameLowerer;
    std::unique_ptr<MirInstructionSelector> m_selector;
    std::unique_ptr<TargetRelocationResolver> m_relocResolver;
};

} // namespace EzTargets::MyArch
```

---

### 2. Instruction Selector (`MyArchTargetInstructionSelector`)
Inherit from the generated base selector `MyArchInstructionSelector` and provide custom fallbacks for control flow, comparisons, and memory:

```cpp
// EzTargets/MyArch/src/MyArchTargetInstructionSelector.cpp
#include "MyArchTargetInstructionSelector.h"

namespace EzTargets::MyArch
{

bool MyArchTargetInstructionSelector::select(MirBuilderContext *ctx, MirInstruction *inst)
{
    // 1. Try generated pattern selector first
    if (MyArchInstructionSelector::select(ctx, inst))
        return true;

    // 2. Custom fallback for control flow and calls
    switch (inst->getOpCode())
    {
        case MirInstructionOpCode::BR:
            return selectBranch(ctx, inst);
        case MirInstructionOpCode::BR_COND:
            return selectCondBranch(ctx, inst);
        case MirInstructionOpCode::CALL:
            return selectCall(ctx, inst);
        default:
            return false;
    }
}

} // namespace EzTargets::MyArch
```

---

### 3. Frame Lowerer (`MyArchFrameLowerer`)
Implement function prologue, epilogue, and stack alignment:

```cpp
// EzTargets/MyArch/src/MyArchFrameLowerer.cpp
#include "MyArchFrameLowerer.h"
#include "Block/MirBlock.h"
#include "Function/MirFunction.h"

namespace EzTargets::MyArch
{

bool MyArchFrameLowerer::emitPrologue(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    MirBlock *entryBlock = func->getEntryBlock();
    size_t stackSize = ctx.m_stackFrameSize; // Computed by frame analysis

    // Align stack frame to 16 bytes
    stackSize = (stackSize + 15) & ~15;

    // 1. Decrement stack pointer: addi sp, sp, -stackSize
    // 2. Save return address (ra) and frame pointer (s0)
    // 3. Save any callee-saved registers touched by the function
    return true;
}

bool MyArchFrameLowerer::emitEpilogue(FrameLowererCtx &ctx)
{
    // 1. Restore callee-saved registers
    // 2. Restore ra and s0
    // 3. Increment stack pointer: addi sp, sp, stackSize
    // 4. Return: jalr x0, ra, 0 (ret)
    return true;
}

} // namespace EzTargets::MyArch
```

---

### 4. Code Emitter & Relocation Resolver (`MyArchCodeEmitter`)
Encode machine instructions into `DataNode` streams and patch branch offsets:

```cpp
// EzTargets/MyArch/src/MyArchRelocationResolver.cpp
#include "MyArchRelocationResolver.h"

namespace EzTargets::MyArch
{

bool MyArchRelocationResolver::resolve(
    TargetCodeRelocationType relocType,
    uint64_t sourceOffset,
    uint64_t targetOffset,
    int64_t addend,
    uint8_t *patchBuffer,
    size_t bufferSize)
{
    int64_t displacement = static_cast<int64_t>(targetOffset - sourceOffset + addend);

    switch (relocType)
    {
        case TargetCodeRelocationType::Branch32:
        {
            // Patch 32-bit branch or jump offset
            int32_t disp32 = static_cast<int32_t>(displacement);
            std::memcpy(patchBuffer, &disp32, sizeof(disp32));
            return true;
        }
        default:
            return false;
    }
}

} // namespace EzTargets::MyArch
```

---

## 🔌 Phase 4: Target Registration & Triple Binding

Register the architecture factory with `TargetResolver`:

```cpp
// EzTargets/MyArch/Registration/MyArchTargetRegistration.cpp
#include "EzTargetsMyArchRegistration.h"
#include "Compiler/TargetResolver.h"
#include "MyArchTargetDesc.h"

namespace EzTargets::MyArch
{

void registerTarget()
{
    EzCompiler::TargetResolver::registerTarget(
        "myarch",
        [](const EzCompiler::TargetTriple &triple,
           MirBuilderContext *mirCtx,
           bool isPositionIndependent,
           const std::vector<std::string> &features) -> EzCompiler::ResolvedTarget
        {
            EzCompiler::ResolvedTarget result;
            auto target = std::make_unique<MyArchTargetDesc>(mirCtx);
            target->initialize();

            result.m_targetDesc = std::move(target);
            return result;
        }
    );
}

// Automatic registration on static load
namespace {
    struct MyArchAutoInit {
        MyArchAutoInit() { registerTarget(); }
    } g_autoInit;
}

} // namespace EzTargets::MyArch
```

---

## 🧪 Phase 5: Verification & Testing

1. **Add Unit Tests**: Under `tests/EzTargetsMyArchTestSuite/`, verify register bank lookups, instruction selector pattern coverage, and frame lowerer prologues.
2. **Add End-to-End Compilation Tests**: Under `tests/EzCompilerEndToEndTests/`, verify that `ezc` compiles arithmetic, loops, and function calls into valid binary object files.
3. **Disassembly Verification**: Use `llvm-objdump -d` or `objdump` to ensure emitted opcodes match hardware specification.

---

> [!TIP]
> Inspect the full implementation of x86-64 in [`EzTargets/X86_64`](file:///E:/Repos/EzPacker/EzTargets/X86_64) as a production reference!
