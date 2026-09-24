# EzPacker: Modern, Modular Compiler Backend & Code Generation Toolchain

**EzPacker** is an ahead-of-time (AOT) compiler backend, intermediate representation, and machine code generation toolchain written in modern C++20. Designed with clean modular separation and zero-allocation hot paths, EzPacker provides an end-to-end compilation pipeline: from high-level intermediate representations down to fully linked, relocatable binary object files (**System V ELF64** and **Windows PE/COFF**).

---

## Architectural Topology & Subprojects

EzPacker is partitioned into specialized subprojects, each with dedicated responsibilities and self-contained documentation:

```
                      ┌────────────────────────────────────────────────────────┐
                      │                 EzPacker (Root Project)                │
                      │          Ecosystem Topology & End-to-End Flow          │
                      └───────────────────────────┬────────────────────────────┘
                                                  │
          ┌───────────────────┬───────────────────┼───────────────────┬───────────────────┐
          ▼                   ▼                   ▼                   ▼                   ▼
 ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
 │     EzCore      │ │      EzMir      │ │  EzCodeEmitter  │ │    EzTriple     │ │   EzCompiler    │
 │ Foundational    │ │ Multi-Tier IR,  │ │ Binary Emitter, │ │ Architecture    │ │ Driver Engine,  │
 │ Utilities, Math,│ │ SSA Engine,     │ │ Section Nodes,  │ │ Target Descs,   │ │ Pass Pipeline,  │
 │ Diagnostics, PMR│ │ Passes, Parser  │ │ ELF/COFF Writers│ │ RegAlloc, ISel  │ │ Target Resolver │
 └─────────────────┘ └─────────────────┘ └─────────────────┘ └─────────────────┘ └─────────────────┘
                                                  ▲                           ▲
                                                  │                           │
                                         ┌────────┴────────┐         ┌────────┴────────┐
                                         │      EzDsl      │         │    EzTargets    │
                                         │ Declarative DSL │         │ Machine Target  │
                                         │ Backend Suite   │         │ Backends (x86)  │
                                         └─────────────────┘         └─────────────────┘
```

### Subprojects Index

| Subproject | Directory | Primary Role & Subsystems | Documentation Guide |
|:---|:---|:---|:---|
| **[`EzCore`](file:///E:/Repos/EzPacker/EzCore)** | [`EzCore/`](file:///E:/Repos/EzPacker/EzCore/) | Foundational infrastructure: thread-safe diagnostics (`DiagnosticCollector`, `DiagnosticBuilder`, `DiagnosticScope`), arbitrary-precision math (`FlexInt`, `FlexFloat`), source tracking (`SourceManager`), fast bitvectors (`DenseBitSet`), zero-allocation intrusive lists (`IntrusiveLinkedList`), PMR arenas. | [EzCore README](file:///E:/Repos/EzPacker/EzCore/README.md) |
| **[`EzMir`](file:///E:/Repos/EzPacker/EzMir)** | [`EzMir/`](file:///E:/Repos/EzPacker/EzMir/) | Machine Intermediate Representation: multi-tier IR (HighLevel, PassInternal, TargetLow), Cytron et al. SSA & Cooper-Harvey-Kennedy dominators, CFG & liveness dataflow analysis, interned type system (`MirTypeTable`), Lexy-based `.mir` parser (`MirParser`), and textual disassembler (`MirPrinter`). | [EzMir README](file:///E:/Repos/EzPacker/EzMir/README.md) |
| **[`EzCodeEmitter`](file:///E:/Repos/EzPacker/EzCodeEmitter)** | [`EzCodeEmitter/`](file:///E:/Repos/EzPacker/EzCodeEmitter/) | Low-level machine code emission: non-linear doubly-linked section stream (`SectionNodeKind::Data`, `Label`, `Align`), late layout linearization, post-finalization binary patching, and standard object format writers (`Elf64Writer`, `CoffWriter`). | [EzCodeEmitter README](file:///E:/Repos/EzPacker/EzCodeEmitter/README.md) |
| **[`EzTriple`](file:///E:/Repos/EzPacker/EzTriple)** | [`EzTriple/`](file:///E:/Repos/EzPacker/EzTriple/) | Target architecture & lowering engine: hardware abstractions (`TargetDesc`, `TargetBinaryDesc`), ABI lowerer (`MirAbiLowererPass`), Maximal Munch tree ISel (`MirInstructionSelectorPass`), Chaitin-Briggs graph coloring register allocation (`MirRegisterAllocatorPass`), and frame layout (`MirFrameLowererPass`). | [EzTriple README](file:///E:/Repos/EzPacker/EzTriple/README.md) |
| **[`EzCompiler`](file:///E:/Repos/EzPacker/EzCompiler)** | [`EzCompiler/`](file:///E:/Repos/EzPacker/EzCompiler/) | End-to-end compiler driver: executable driver `ezc` and `EzCompilerLib`, argparse CLI parsing, session lifecycle (`DriverContext`), `TargetTriple` normalization, target resolver factory registry, pass scheduling (`CompilationPipeline`), and binary packaging (`EmissionEngine`). | [EzCompiler README](file:///E:/Repos/EzPacker/EzCompiler/README.md) |
| **[`EzDsl`](file:///E:/Repos/EzPacker/EzDsl)** | [`EzDsl/`](file:///E:/Repos/EzPacker/EzDsl/) | Declarative backend description language suite: 9 sub-languages (`.tdesc`, `.idf`, `.ezcc`/`.ccd`, `.irdf`, `.lad`, `.lrd`, `.isf`, `.tyf`), Lexy zero-copy parsers, semantic validation passes, 10 C++ production code generators, standalone `EzDslCli` compiler. | [EzDsl README](file:///E:/Repos/EzPacker/EzDsl/README.md) |
| **[`EzTargets`](file:///E:/Repos/EzPacker/EzTargets)** | [`EzTargets/`](file:///E:/Repos/EzPacker/EzTargets/) | Target machine backends: concrete target implementations including `X86_64` (register info, instruction tables, machine encoders, calling conventions). | [`EzTargets/X86_64/`](file:///E:/Repos/EzPacker/EzTargets/X86_64/) |

---

## End-to-End Compilation Workflow

```
                        Input Source / Textual MIR (.ez / .mir)
                                           │
                                           ▼
                             ┌───────────────────────────┐
                             │     MirModuleLoader       │
                             │ (Lexy-based MirParser)    │
                             └─────────────┬─────────────┘
                                           │
                                           ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                               EzMir Module                                  │
    │                    Abstract Target-Independent SSA IR                       │
    └──────────────────────────────────────┬──────────────────────────────────────┘
                                           │
                       ┌───────────────────┴───────────────────┐
                       │   CompilationPipeline (Middle-End)    │
                       │ - CodeFlowAnalysisPass (CFG)          │
                       │ - NonSsaToSsaPass (Dominance / Phis)  │
                       │ - LivenessAnalysisPass (Intervals)    │
                       └───────────────────┬───────────────────┘
                                           │
                       ┌───────────────────┴───────────────────┐
                       │   CompilationPipeline (Legalization)  │
                       │ - MirFunctionSignatureLegalizerPass   │
                       │ - MirLegalizerPass (Actions & Rules)  │
                       └───────────────────┬───────────────────┘
                                           │
                       ┌───────────────────┴───────────────────┐
                       │ CompilationPipeline (Target Lowering) │
                       │ - MirAbiLowererPass (ABI Calls/Args)  │
                       │ - MirInstructionSelectorPass (ISel)   │
                       │ - MirRegisterAllocatorPass (Coloring) │
                       │ - MirFrameLowererPass (Prolog/Epilog) │
                       └───────────────────┬───────────────────┘
                                           │
                                           ▼
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                         Target-Lowered MirModule                            │
    │                  Physical Registers & Machine Opcodes                       │
    └──────────────────────────────────────┬──────────────────────────────────────┘
                                           │
                                           ▼
                             ┌───────────────────────────┐
                             │       EmissionEngine      │
                             │   (GenericCodeEmitter)    │
                             └─────────────┬─────────────┘
                                           │
                       ┌───────────────────┴───────────────────┐
                       │     ObjectFormat Serializer           │
                       │   (Elf64Writer / CoffWriter)          │
                       └───────────────────┬───────────────────┘
                                           │
                                           ▼
                      Relocatable Object File (.o / .obj / .s)
```

---

## Build Prerequisites & Quickstart

### Prerequisites
- **C++ Compiler**: Clang 16+, GCC 13+, or MSVC 2022 (v143+) with full **C++20** support.
- **Build System**: CMake 3.25+ and Ninja (recommended).
- **External Dependencies**: Vendored or submoduled in `extern/` (`libtommath`, `libbf`, `lexy`, `argparse`, `googletest`).

### Build Commands

```powershell
# 1. Configure the project with CMake and Ninja
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 2. Compile all subprojects, libraries, and executables
cmake --build cmake-build-debug

# 3. Run the complete automated test suite (all 93 tests)
ctest --test-dir cmake-build-debug --output-on-failure
```

---

## Command-Line Usage

### Compiling with `ezc`

```bash
# Compile a MIR module into a System V ELF64 object file
./cmake-build-debug/EzCompiler/ezc input.mir -o output.o --target x86_64-linux-gnu -O2

# Compile a MIR module into a Windows PE/COFF object file
./cmake-build-debug/EzCompiler/ezc input.mir -o output.obj --target x86_64-windows-msvc

# Inspect intermediate SSA MIR after middle-end passes
./cmake-build-debug/EzCompiler/ezc input.mir --emit-mir

# Inspect target-lowered assembly text
./cmake-build-debug/EzCompiler/ezc input.mir -S -o output.s
```

### Compiling Backend Descriptions with `EzDslCli`

```bash
# Ingest target descriptor and synthesize register info
./cmake-build-debug/EzDsl/Cli/EzDslCli -i targets/x86_64/x86_64.tdesc --emit-registers -o generated/

# Ingest instruction selection patterns and generate C++ matcher
./cmake-build-debug/EzDsl/Cli/EzDslCli -i targets/x86_64/patterns.isf --emit-instruction-selector -o generated/
```

---

## Repository Layout

```
EzPacker/
├── CMakeLists.txt              # Root CMake build configuration
├── README.md                   # Global architectural overview (this file)
├── EzCore/                     # Foundational utilities, diagnostics, math, PMR
│   ├── include/                # Public headers (Diagnostics, FlexNumber, SourceManager)
│   ├── src/                    # Subsystem implementations
│   ├── tests/                  # EzCore unit test fixtures
│   └── README.md               # EzCore architectural guide
├── EzMir/                      # Machine Intermediate Representation & middle-end
│   ├── include/                # IR headers (Instruction, Operand, Block, Builder, Parser)
│   ├── src/                    # Passes, printer, parser implementations
│   └── README.md               # EzMir architectural guide
├── EzCodeEmitter/              # Machine code emission and object file writers
│   ├── include/                # CodeSection, CodeEmitterContext, IObjectWriter, Elf64/Coff
│   ├── src/                    # Serialization and writer implementations
│   └── README.md               # EzCodeEmitter architectural guide
├── EzTriple/                   # Target architecture, lowering, ISel, RegAlloc, Frame
│   ├── include/                # Descriptors, ABI lowerers, ISel matchers, graph coloring
│   ├── src/                    # Backend pass implementations
│   └── README.md               # EzTriple architectural guide
├── EzCompiler/                 # Driver executable (ezc) & CompilationPipeline
│   ├── include/                # CommandLineOptions, DriverContext, TargetResolver, Pipeline
│   ├── src/                    # Driver implementation and Main.cpp entrypoint
│   └── README.md               # EzCompiler architectural guide
├── EzDsl/                      # Declarative backend description language suite
│   ├── Lexer/                  # Lexy-based AST & grammar parsers
│   ├── Sema/                   # Semantic validation passes and symbol tables
│   ├── CodeGenerators/         # 10 C++ production table generators
│   ├── Cli/                    # EzDslCli driver executable
│   └── README.md               # EzDsl language specifications and guide
├── EzTargets/                  # Target architecture backend implementations
│   └── X86_64/                 # x86-64 target descriptor, encoders, and register banks
├── tests/                      # Centralized multi-subproject test suites
│   ├── EzCodeEmitterTestSuite/ # Code emission & object writer tests
│   ├── EzCompilerTestSuite/    # Driver & compiler component tests
│   ├── EzCompilerEndToEndTests/# 10 complete end-to-end compilation benchmarks
│   ├── EzDslLexerTestSuite/    # DSL grammar & AST tests
│   ├── EzDslSemaTestSuite/     # DSL semantic validation tests
│   ├── EzDslCodeGeneratorsTestSuite/ # C++ code generation tests
│   ├── EzDslCliTestSuite/      # EzDslCli integration tests
│   ├── EzMirTestSuite/         # IR, SSA, and pass tests
│   └── EzTripleTestSuite/      # Target lowering and register allocation tests
└── graphify-out/               # Graphify knowledge graph and architecture reports
```

---

## License

This project is licensed under the Apache License 2.0. See the `LICENSE` file for details.
