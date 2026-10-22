# EzPacker: Modern C++20 Compiler Backend & Code Generation Framework

Welcome to the comprehensive documentation for **EzPacker**.

EzPacker is a modular, high-performance compiler backend, machine intermediate representation (MIR) framework, and binary code generation engine written in ISO C++20. It accepts strongly-typed, SSA-form machine intermediate representation and transforms it into native, relocatable machine code object files (**ELF64** for Linux; **PE/COFF** for Windows) targeting x86-64 (with SSE and AVX vector extensions) and native 64-bit RISC-V (**riscv64** with standard RV64IMAFD extensions).

---

## 1. Global Architecture Overview

EzPacker's end-to-end pipeline cleanly separates target-independent optimization and lowering stages from architecture-specific definitions:

```mermaid
flowchart TD
    subgraph Frontend["1. Frontend & Input"]
        InputMIR["Source MIR (.mir)"] --> Parser["MirLexer & MirParser"]
        InputC["C Source (.c, .i)"] --> CFrontend["Standalone C11 Frontend\n(ChibiccWrapper, CFrontendTypeMapper,\nCFrontendFunctionLowerer, CFrontendAdapter)"]
        Parser --> InMemMIR["In-Memory SSA MIR\n(MirFunction, MirBlock, MirInstruction)"]
        CFrontend --> InMemMIR
    end

    subgraph MiddleEnd["2. Middle-End Analysis & Optimization"]
        InMemMIR --> CFGPass["CodeFlowAnalysisPass\n(CFG, Dominance Tree)"]
        CFGPass --> SSAPass["NonSsaToSsaPass\n(Phi Node Placement, Renaming)"]
        SSAPass --> LivenessPass["LivenessAnalysisPass\n(Bit-Vector Dataflow, Live Intervals)"]
        LivenessPass --> PeepholePass["MirPeepholePass (-O1/-O2/-Os)\n(Algebraic Identities, Self-Moves, Dead Code, Fall-Through)"]
    end

    subgraph Lowering["3. Target Lowering Pipeline (EzTriple)"]
        PeepholePass --> Legalizer["MirLegalizerPass\n(3-Tier Legality Matrix: Widen, Narrow, Lower)"]
        Legalizer --> ABILower["MirAbiLowererPass\n(Token Lowering, Parameter & Return Registers)"]
        ABILower --> ISel["MirInstructionSelectorPass\n(Bottom-Up Maximal Munch, Load-Folding)"]
        ISel --> RegAlloc["MirRegisterAllocatorPass\n(Chaitin-Briggs Coloring, Conservative Coalescing, Copy Removal)"]
        RegAlloc --> FrameLower["MirFrameLowererPass\n(Prologue/Epilogue Insertion, Stack Layout)"]
        FrameLower --> TargetPeephole["MirTargetPeepholePass (-O1/-O2/-Os)\n(Machine Move Removal, Spill Forwarding, Dead Stores)"]
    end

    subgraph Backend["4. Machine Code Emission (EzCodeEmitter)"]
        TargetPeephole --> CodeEmitter["GenericCodeEmitter\n(X86_64InstructionEncoder, BranchRelaxer)"]
        CodeEmitter --> Sections["CodeSection (.text, .data, .rodata) &\nRelocations (ObjectRelocEntry)"]
        Sections --> ObjectWriter{"IObjectWriter\n(Format Dispatch)"}
        ObjectWriter -->|Linux / BSD| ElfWriter["Elf64Writer\n(System V ELF64 .o)"]
        ObjectWriter -->|Windows| CoffWriter["CoffWriter\n(Microsoft PE/COFF .obj)"]
    end

    subgraph Linking["5. Linking & Runtime (EzLinker / ez-ld)"]
        ElfWriter --> Linker["EzLinkerDriver\n(Toolchain Detection & Command Synthesis)"]
        CoffWriter --> Linker
        Linker --> Runtime["Injected Runtimes:\n- Freestanding EzStd.mir (Standalone Standard Library)\n- Compiler Legalizer Helpers (compiler-rt / libgcc)\n- Freestanding EzExceptionRuntime (Custom SjLj & RTTI)"]
        Runtime --> SystemLinker{"System Linker\n(lld, link.exe, ld, gcc)"}
        SystemLinker --> FinalBin["Native Executable (.exe / ELF)\nor Shared Library (.dll / .so)"]
    end
```

### The Declarative DSL Architecture (`EzDsl`)
Instead of hand-coding thousands of lines of fragile C++ tables for hardware registers, instruction encodings, and calling conventions, EzPacker uses a suite of domain-specific languages:

```mermaid
flowchart LR
    subgraph DSLInputs["Declarative DSL Sources"]
        TDesc[".tdesc\n(Target Descriptors)"]
        EZCC[".ezcc\n(Calling Conventions)"]
        LAD[".lad\n(Legalization Actions)"]
        LRD[".lrd\n(Rewrite Rules)"]
        IDF[".idf\n(Instructions & Encodings)"]
        ISF[".isf\n(Pattern Matching Trees)"]
    end

    subgraph Toolchain["EzDsl Meta-Compiler Toolchain"]
        Lexer["EzDsl::Lexer\n(lexy combinators)"]
        Sema["EzDsl::Sema\n(Type checking & dialects)"]
        CodeGen["EzDsl::CodeGenerators\n(C++ synthesizers)"]
    end

    subgraph Runtime["Synthesized C++ Artifacts"]
        Headers["Runtime Lookup Tables\n& Matcher Classes"]
    end

    DSLInputs --> Lexer --> Sema --> CodeGen --> Headers
    Headers --> Lowering
```

---

## 2. Core Capabilities & Design Highlights

- **Pure C++20 Design**: Leverages concepts, `<format>`, ranges, and polymorphic memory resources (`std::pmr`) for memory efficiency and zero heap fragmentation during pass execution.
- **Modular C11 Frontend (`chibicc`) & ISO C11 StdLib Support (`EzLibStdC`)**: Direct ingestion of C source files (`.c`) and preprocessed files (`.i`) using a clean, modular frontend (`ChibiccWrapper`, `CFrontendTypeMapper`, `CFrontendFunctionLowerer`, `CFrontendAdapter`). Supports both freestanding (`-ffreestanding`, default) and hosted (`-fhosted`) environments, bundling all 9 freestanding headers and all 28 ISO C11 standard library headers (`include/c11std`), lowering directly to SSA MIR with full GNU attribute support and cross-platform standard C runtime linking (`EzLibStdC`).
- **Multi-Stage Optimization Pipeline**: Generic SSA peephole optimizer (`MirPeepholePass`), conservative copy/two-address register coalescing (George & Briggs criteria) with copy affinity coloring, redundant copy elimination, and post-frame machine peephole optimizer (`MirTargetPeepholePass`).
- **Table-Driven 3-Tier Legalizer & Libcalls**: Dense primary matrix indexed by opcode and compact type provides $O(1)$ legality dispatch, combined with multi-slot signature matchers, compiler-rt runtime libcall mappings (`__multi3`, `__divti3`), and declarative algebraic strength reduction rules.
- **Bottom-Up Maximal Munch Instruction Selection**: Tree-matching pattern matching that automatically synthesizes SIB addressing modes, performs opportunistic memory load-folding (`ADD64rm`), and prioritizes AVX over SSE via cost-ordered matching.
- **Production Chaitin-Briggs Graph-Coloring Register Allocator**: Full interference graph construction, loop-depth spill cost estimation, optimistic simplification, register spilling, and conservative coalescing across GPR and FPR/VR256 banks.
- **Dual Calling Convention & Binary Format Support**: Built-in support for **System V AMD64**, **Microsoft Win64**, and RISC-V **LP64D** calling conventions, with native emitters for **ELF64** (`.o`) and **PE/COFF** (`.obj`).
- **Complete Vector Extension & VEX Support**: Implication-aware CPU feature tracking supporting SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, and AVX2 with dedicated 256-bit `VR256` YMM registers and table-driven 2-byte (`0xC5`) / 3-byte (`0xC4`) VEX prefix encoding.
- **Modular Target Architecture**: `EzTargets` organized into clean architectural subsystems (`InstructionSelector`, `FrameLowerer`, `RegisterAllocator`, `Lowering`, `Descriptors`, `Relocation`, `CodeEmitter`, `BranchRelaxation`, `Encoding`) eliminating legacy fallback code.

---

## 3. Documentation Navigation

### 📖 Essential Guides
- **[First Steps & Quickstart](first_steps.md)**: Write your first MIR module, compile to native object code, inspect intermediate pipeline states, and link with host C/C++ toolchains.
- **[Comprehensive Build Guide](build_guide.md)**: Toolchain requirements, CMake configuration options, step-by-step compilation on Windows and Linux, running test suites, web IDE setup, and generating docs.
- **[Examples & Use Cases](examples.md)**: In-depth technical walkthrough of bundled MIR and C11 modules (arithmetic, control flow, ABI lowering, crypto hashing, load-folding, recursion, epilogues, exceptions, float conversions, vector SIMD/AVX, and YARPGen C kernels).
- **[How to Build a Target Architecture](how_to_build_a_target.md)**: Complete architectural tutorial on adding a new CPU architecture backend to EzPacker from first principles.
- **[YARPGen Differential Testing](yarpgen_differential_testing.md)**: Dual-mode testing infrastructure (bulk multi-file compilation + 3,600+ LOC single-file mega stress test) comparing EzPacker against GCC/Clang with automated reproducer triage.

### 🌐 Interactive Web Compiler Explorer (`web/`)
EzPacker includes a full-stack, local-first web application featuring:
- **Live Monaco Editor**: Syntax highlighting for both C11 source and EzPacker SSA MIR with auto-detection.
- **Multi-Stage Pipeline Inspection**: Side-by-side inspection of Generic MIR, Legalized MIR, Lowered Machine MIR, and Native Assembly.
- **Interactive CFG & Dataflow Viewers**: Rendered control flow graphs and live-interval liveness matrices for all compilation passes.
- **Object Binary Dissector**: In-depth PE/COFF and ELF64 section, symbol, and relocation table inspection.
- **Doxygen Documentation Bridge**: Interactive hovering over symbols to reveal docstrings, parameters, return types, and definitions extracted from Doxygen XML.

To launch the web interface:
```bash
# Start the full-stack server (serves the pre-built web client at http://localhost:3001)
npm start --prefix web/server
```

### 🏛️ Subproject Overviews
Every non-vendored subproject in the repository is documented in detail:

| Subproject | Description | Documentation |
| :--- | :--- | :--- |
| **`EzCore`** | Foundational utilities: PMR allocators, diagnostic engine, `FlexInt`/`FlexFloat`, `DenseBitSet`, `IntrusiveLinkedList`, `SourceManager`. | [EzCore Guide](projects/EzCore.md) |
| **`EzMir`** | Machine Intermediate Representation: `MirFunction`, `MirBlock`, `MirInstruction`, `MirOperand`, `MirType`, SSA pass framework, parser, and printer. | [EzMir Guide](projects/EzMir.md) |
| **`EzDsl`** | Meta-compiler toolkit: Lexer, Sema, CodeGenerators, and `EzDslCli` driver for all 10 DSL dialects. | [EzDsl Guide](projects/EzDsl.md) |
| **`EzCodeEmitter`** | Binary code emission: section management, relocations, `Elf64Writer` (ELF64), and `CoffWriter` (PE/COFF). | [EzCodeEmitter Guide](projects/EzCodeEmitter.md) |
| **`EzTriple`** | Target-independent backend lowering: Legalizer, ABI lowerer, instruction selector, register allocator, and frame lowerer. | [EzTriple Guide](projects/EzTriple.md) |
| **`EzCompiler`** | Driver application: command-line parsing, `DriverContext`, target resolver, compilation pipeline, and emission engine (`ezc`). | [EzCompiler Guide](projects/EzCompiler.md) |
| **`EzTargets`** | Architecture backends: modular x86-64 and RISC-V 64-bit targets (synthesized `X86_64TargetDesc` and `RiscV64TargetDesc`, `VR256` registers, VEX prefix encoder, RV64IMAFD table-driven encoder, compiler-rt libcalls, `X86_64FrameLowerer`, `RiscV64FrameLowerer`, `BranchRelaxer`). | [EzTargets Guide](projects/EzTargets.md) |
| **`EzLinker`** | System linker driver and freestanding runtime: automated toolchain detection (`lld`, `link.exe`), decoupled C stdlib, freestanding `EzStd.mir` and custom assembly `EzExceptionRuntime` (`ez-ld`). | [EzLinker Guide](projects/EzLinker.md) |
| **`web`** | Compiler Explorer & Web IDE: React/Vite client and Node.js bridge for live interactive compilation, CFG inspection, and Doxygen symbol exploration. | [Web Explorer](file:///E:/Repos/EzPacker/web/) |

### 🔍 Complete API Reference
- **[Generated Doxygen API Documentation](doxygen/index.html)**: Interactive class hierarchies, inheritance diagrams, member documentation, and source code cross-references. Generate locally on demand via:
  ```bash
  cmake --build <build-dir> --target doxygen
  # or
  cmake --build <build-dir> --target docs
  ```

---

## 4. Quick Taste of EzPacker

```mir
// compute.mir
fn @dot3(i32 %x1, i32 %y1, i32 %x2, i32 %y2) -> i32 {
entry:
    %p1 = IMUL i32 %x1, %y1;
    %p2 = IMUL i32 %x2, %y2;
    %dot = ADD i32 %p1, %p2;
    RET i32 %dot;
}
```

Compile directly to an assembly listing:
```bash
EzCompiler compute.mir --emit-asm
```

Generated x86-64 Assembly (Linux System V AMD64):
```nasm
dot3:
    push    rbp
    mov     rbp, rsp
    imul    edi, esi
    imul    edx, ecx
    add     edi, edx
    mov     eax, edi
    mov     rsp, rbp
    pop     rbp
    ret
```

### Standalone Freestanding C11 (Microcontrollers & Kernels)

EzPacker's embedded modular C frontend compiles pure standalone C11 with **zero dependencies on hosted libc**:

```c
// gpio_driver.c (Freestanding Baremetal MMIO)
#include <stdint.h>
#define GPIO_ODR (*(volatile uint32_t *)0x40020014UL)

void toggle_pin(uint8_t pin) {
    GPIO_ODR ^= (1UL << pin);
}
```

```bash
# Compile directly to native object code
ezc -O2 gpio_driver.c -o gpio_driver.o
```
