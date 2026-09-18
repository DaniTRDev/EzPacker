# Master Architecture & Implementation Plan: Unified End-to-End Compiler Driver CLI (`ezc`)
**Industrial-Grade Toolchain Coordinator, Target Resolver, Pass Pipeline Orchestrator & Binary Emission Engine**

---

## 1. Executive Summary & Vision

### 1.1 Context & Purpose
With the maturation of the core compiler layers—intermediate representation ([`EzMir`](file:///E:/Repos/EzPacker/EzMir)), declarative target meta-compiler ([`EzDsl`](file:///E:/Repos/EzPacker/EzDsl)), backend lowering & target transforms ([`EzTriple`](file:///E:/Repos/EzPacker/EzTriple)), and hardware machine code emission ([`EzCodeEmitter`](file:///E:/Repos/EzPacker/EzCodeEmitter))—the EzPacker suite requires a unified, industrial-grade **Compiler Driver (`ezc`)**.

The driver serves as the top-level orchestration tool. It ingests user command-line arguments, configures memory arena hierarchies, manages source tracking and diagnostic logging, resolves target triples to concrete architecture descriptors, routes intermediate representations through optimization and backend passes, and serializes machine code into native object files (`.o` / `.obj`).

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   ezc Compiler Driver Pipeline Topology                                │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

  [ Command Line: ezc main.ez -o main.o --target x86_64-unknown-linux-gnu -O2 ]
                                   │
                                   ▼
  ┌─────────────────────────────────────────────────────────────────────────┐
  │                         DriverContext & TargetResolver                  │
  │  - CLI Argument Parsing (argparse)                                      │
  │  - Target Triple Parsing: x86_64 -> X86_64TargetDesc                    │
  │  - OS/ABI Binding: Linux -> SysV Calling Convention + ELF64 Binary Desc │
  │  - PMR Memory Resource Hierarchy Setup (Session, Function, Pass Arenas) │
  └────────────────────────────────────┬────────────────────────────────────┘
                                       │
                                       ▼
  ┌─────────────────────────────────────────────────────────────────────────┐
  │                           FrontendAdapter Boundary                      │
  │  - High-Level Input (.ez): Route to EzFrontend 2.0 (Plug-in ready)      │
  │  - Intermediate Input (.mir): Direct Textual/Programmatic MIR Loading   │
  │  - Produces: MirBuilderContext with un-lowered Generic MirModule        │
  └────────────────────────────────────┬────────────────────────────────────┘
                                       │
               [ Gate: --emit-mir ]    │
                                       ▼
  ┌─────────────────────────────────────────────────────────────────────────┐
  │                            EzMir Middle-End Passes                      │
  │  - NonSsaToSsaPass: VReg SSA phi insertion & renaming                   │
  │  - CodeFlowAnalysisPass: Predecessor/Successor CFG construction         │
  │  - LivenessAnalysisPass: Live-in/out & def-use intervals                │
  └────────────────────────────────────┬────────────────────────────────────┘
                                       │
                                       ▼
  ┌─────────────────────────────────────────────────────────────────────────┐
  │                           EzTriple Backend Pipeline                     │
  │  1. MirFunctionSignatureLegalizerPass: Tokenize signature args/returns  │
  │  2. MirLegalizerPass: Table-driven 3-tier operation rewriting           │
  │     [ Gate: --emit-legalized-mir ]                                      │
  │  3. MirAbiLowererPass: Physical ABI registers, SRET, shadow space       │
  │  4. MirInstructionSelectorPass: Maximal munch + SIB folding             │
  │  5. MirRegisterAllocatorPass: Chaitin-Briggs graph coloring + spills    │
  │  6. MirFrameLowererPass: Stack layout math, prologue/epilogue emission  │
  │     [ Gate: --emit-lowered-mir ]                                        │
  └────────────────────────────────────┬────────────────────────────────────┘
                                       │
                                       ▼
  ┌─────────────────────────────────────────────────────────────────────────┐
  │                       EzCodeEmitter & Binary Packaging                  │
  │  1. X86_64CodeEmitter: Direct Intel 64 machine instruction encoding     │
  │  2. BranchRelaxer: 2-pass label resolution & short-to-near relaxation   │
  │  3. Section Population: .text, .data, .rodata, .bss                     │
  │  4. Binary Format Writer: Elf64Writer (.o) or CoffWriter (.obj)         │
  │     [ Gate: --emit-obj ]                                                │
  └────────────────────────────────────┬────────────────────────────────────┘
                                       │
                                       ▼
                      [ Output File: main.o / main.obj ]
```

### 1.2 The Decoupled Frontend Contract
Because `EzFrontend` is currently being redesigned as a next-generation frontend, `ezc` must establish a clean **`FrontendAdapter`** boundary:
1. **Source Mode**: Accepts `.ez` files through an abstract interface (`IFrontendAdapter`), allowing the new frontend compiler to be slotted in without altering driver logic.
2. **Intermediate MIR Mode**: Accepts `.mir` files or programmatic MIR module definitions, enabling complete end-to-end backend testing, fuzzing, and driver certification independently of frontend syntax changes.

---

## 2. Core Architectural Invariants

The driver must strictly enforce five non-negotiable operational invariants under all execution paths:

* **[INV-DRV-01] Multi-Tier PMR Arena Isolation**:
  * **Session Arena**: Persists across the entire process lifetime. Allocates `MirTypeTable`, `TargetDesc`, `CommandLineOptions`, and accumulated diagnostics.
  * **Function Arena**: Recreated or reset per function. Allocates `MirBlock`, `MirInstruction`, `MirOperand`, and `MirFunctionRegisterInfo`.
  * **Pass Scratch Arena**: Allocated at pass entry and completely reset at pass exit. Allocates worklists, interference graphs, and fold candidate buffers.
  * *Invariant*: Memory usage must scale strictly linearly with function size ($O(N)$), with zero inner-loop heap allocations (`malloc`/`new`).

* **[INV-DRV-02] Structured Diagnostics & Zero Exception Leakage**:
  * Raw C++ runtime exceptions must never leak out of the compiler process.
  * All syntax errors, semantic failures, pass aborts, or target unsupported operations must register with `DiagnosticCollector`.
  * The driver must render diagnostics with byte-exact line and column highlights via `DiagnosticLogger`.

* **[INV-DRV-03] Modular Phase-Gate Halting (`--emit-*`)**:
  * The compiler pipeline must support clean early exit at any milestone:
    * `--emit-mir`: Serializes generic, unlowered SSA MIR to stdout or file.
    * `--emit-legalized-mir`: Halts immediately post-`MirLegalizerPass`.
    * `--emit-lowered-mir`: Halts post-`MirFrameLowererPass` (target machine instructions with physical registers).
    * `--emit-asm`: Emits readable target assembly text.
    * `--emit-obj`: Emits full native ELF64 / PE-COFF object files (default).

* **[INV-DRV-04] Canonical Target Triple Resolution**:
  * Triples of the form `<arch>-<vendor>-<sys>-<abi>` (e.g. `x86_64-unknown-linux-gnu`, `x86_64-pc-windows-msvc`, `x86_64-elf`, `x86_64-coff`) must be canonicalized deterministically.
  * The driver resolves architecture to `TargetDesc`, OS/ABI to `CallingConvDesc`, and executable environment to `TargetBinaryDesc`.

* **[INV-DRV-05] Deterministic Output Invariant**:
  * Compilation of identical inputs with identical target and optimization flags must produce bit-for-bit identical binary output across runs and host operating systems.

---

## 3. Command-Line Interface (CLI) Specification

The CLI driver binary will be named **`ezc`** (with optional alias `ezpacker`).

### 3.1 Syntax
```bash
ezc [options] <input-file>
```

### 3.2 Flag Catalog

| Flag | Long Flag | Parameter | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| `-o` | `--output` | `<path>` | `a.out` / `a.obj` | Destination path for output file |
| `-c` | | | `true` | Compile and assemble, but do not link (emit object) |
| `-S` | | | `false` | Stop after compilation; emit assembly text |
| | `--target` | `<triple>` | Host default | Target architecture and OS triple (e.g. `x86_64-linux-gnu`, `x86_64-windows-msvc`) |
| | `--list-targets` | | | List all supported architectures and binary formats |
| | `--emit-mir` | | `false` | Dump generic SSA MIR after frontend lowering |
| | `--emit-legalized-mir` | | `false` | Dump MIR after type and operation legalization |
| | `--emit-lowered-mir` | | `false` | Dump target-lowered MIR post register allocation & frame lowering |
| | `--emit-obj` | | `true` | Emit binary object format (ELF64 / PE-COFF) |
| `-O0` | | | Default | Disable optimizations; debug code layout |
| `-O1` | | | | Basic block optimizations, constant folding |
| `-O2` | | | | Full optimization pipeline (SSA cleanup, SIB folding, coloring) |
| `-v` | `--verbose` | | `false` | Enable verbose toolchain diagnostic logging |
| | `--print-passes` | | `false` | Print pass names and execution order |
| | `--time-passes` | | `false` | Report millisecond execution time per compiler pass |
| | `--diag-level` | `error\|warn\|info\|debug` | `warn` | Minimum diagnostic severity threshold |
| | `--color` | `auto\|always\|never` | `auto` | Colorize diagnostic output |
| `-h` | `--help` | | | Display usage summary and option descriptions |
| `-V` | `--version` | | | Display compiler suite version information |

### 3.3 Exit Codes
* **`0`**: Successful compilation.
* **`1`**: User error (syntax error, file not found, bad CLI flag, semantic error).
* **`2`**: Internal Compiler Error (ICE) / invariant assertion failure.

---

## 4. Subsystem & Component Architecture

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                   EzCompiler Architectural Components                                  │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

  EzCompiler Library & Executable (EzCompiler / ezc)
  ├── DriverContext
  │   ├── Polymorphic Memory Resources (Session, Function, Pass arenas)
  │   ├── SourceManager & DiagnosticCollector
  │   └── MirTypeTable (Interned types)
  │
  ├── TargetResolver
  │   ├── Canonical Triple Parser (<arch>-<vendor>-<sys>-<abi>)
  │   ├── Target Registry (TargetDesc factory)
  │   └── Binary Desc Resolver (ELF64 vs COFF)
  │
  ├── FrontendAdapter
  │   ├── IFrontendAdapter (Abstract interface for EzFrontend 2.0)
  │   ├── MirTextLoader (Loads textual/serialized MIR)
  │   └── ProgrammaticMirBuilder (Synthetic test IR generation)
  │
  ├── CompilationPipeline
  │   ├── Middle-End Pass Manager (CFG, SSA, Liveness)
  │   ├── Target Lowering Pass Sequence (Legalizer, ABI, ISel, RegAlloc, Frame)
  │   └── Pass Timing & Inspection Hooks
  │
  └── EmissionEngine
      ├── X86_64CodeEmitter Bridge
      ├── BranchRelaxer Coordinator
      └── ObjectFormat Serializer (Elf64Writer / CoffWriter)
```

### 4.1 DriverContext
```cpp
class DriverContext
{
public:
    explicit DriverContext(const CommandLineOptions &options);
    ~DriverContext();

    std::pmr::memory_resource *getSessionAllocator();
    std::pmr::memory_resource *createFunctionArena();
    void resetFunctionArena();

    SourceManager &getSourceManager();
    DiagnosticCollector &getDiagCollector();
    DiagnosticLogger &getDiagLogger();
    MirTypeTable &getTypeTable();

    TargetDesc *getTarget();
    CallingConvDesc *getCallingConvention();
    TargetBinaryDesc *getBinaryDescriptor();

private:
    std::pmr::monotonic_buffer_resource m_sessionArena;
    std::unique_ptr<std::pmr::monotonic_buffer_resource> m_functionArena;
    SourceManager m_sourceManager;
    DiagnosticCollector m_diagCollector;
    DiagnosticLogger m_diagLogger;
    MirTypeTable m_typeTable;
    std::unique_ptr<TargetDesc> m_targetDesc;
    CommandLineOptions m_options;
};
```

### 4.2 TargetResolver & Canonical Triple Parser
The target resolver parses standard LLVM/GCC-style target triples:
```cpp
struct TargetTriple
{
    std::string m_arch;   // "x86_64", "aarch64"
    std::string m_vendor; // "unknown", "pc", "apple"
    std::string m_sys;    // "linux", "windows", "none"
    std::string m_abi;    // "gnu", "msvc", "elf"

    static TargetTriple parse(std::string_view tripleStr);
    static TargetTriple getHostTriple();
    std::string toString() const;
    bool isX86_64() const;
    bool isWindows() const;
    bool isLinux() const;
};
```
* **Binding Logic**:
  * `arch == "x86_64"` $\rightarrow$ instantiates [`X86_64TargetDesc`](file:///E:/Repos/EzPacker/EzTriple/include/Targets/X86_64/X86_64TargetDesc.h).
  * `sys == "windows" || abi == "msvc"` $\rightarrow$ selects `Win64CallingConvDesc` and [`X86_64CoffBinaryDesc`](file:///E:/Repos/EzPacker/EzTriple/include/Targets/X86_64/X86_64CoffBinaryDesc.h).
  * `sys == "linux" || abi == "gnu" || sys == "none"` $\rightarrow$ selects `SysV_AMD64CallingConvDesc` and [`X86_64ElfBinaryDesc`](file:///E:/Repos/EzPacker/EzTriple/include/Targets/X86_64/X86_64ElfBinaryDesc.h).

### 4.3 FrontendAdapter Boundary
Provides strict decoupling from the mock frontend:
```cpp
class IFrontendAdapter
{
public:
    virtual ~IFrontendAdapter() = default;
    virtual bool compileSourceToMir(DriverContext &ctx,
                                   std::string_view sourcePath,
                                   MirBuilderContext &outMirCtx) = 0;
};

class MirTextAdapter : public IFrontendAdapter
{
public:
    bool compileSourceToMir(DriverContext &ctx,
                            std::string_view sourcePath,
                            MirBuilderContext &outMirCtx) override;
};
```

### 4.4 CompilationPipeline Orchestrator
Coordinates the pass sequence cleanly and handles phase-gate early exits:
```cpp
class CompilationPipeline
{
public:
    explicit CompilationPipeline(DriverContext &ctx);

    bool runMiddleEndPasses(MirFunction *func);
    bool runBackendPasses(MirFunction *func);

private:
    DriverContext &m_ctx;
};
```

**Pass Execution Order**:
1. `CodeFlowAnalysisPass`: Initial block CFG verification.
2. `NonSsaToSsaPass`: SSA virtual register construction.
3. `LivenessAnalysisPass`: Pre-legalization liveness verification.
4. *Gate*: Check `--emit-mir` (dump & halt).
5. `MirFunctionSignatureLegalizerPass`: Signature tokenization (`PUSH_ARG`, `POP_ARG`, `PUSH_RET`, `POP_RET`).
6. `MirLegalizerPass`: Type and opcode legalization.
7. *Gate*: Check `--emit-legalized-mir` (dump & halt).
8. `MirAbiLowererPass`: Lower ABI tokens to hardware calling convention registers.
9. `MirInstructionSelectorPass`: Maximal munch tree matching with SIB folding.
10. `MirRegisterAllocatorPass`: Chaitin-Briggs graph coloring & spilling.
11. `MirFrameLowererPass`: Stack slot alignment, prologue/epilogue emission, ALLOC lowering.
12. *Gate*: Check `--emit-lowered-mir` (dump & halt).

### 4.5 EmissionEngine
Connects the lowered MIR instructions to machine byte emission and object packaging:
```cpp
class EmissionEngine
{
public:
    explicit EmissionEngine(DriverContext &ctx);

    bool emitModule(MirBuilderContext &mirCtx, std::string_view outputPath);

private:
    bool emitFunction(MirFunction *func, class GenericCodeEmitter &emitter);
    bool writeObjectFile(TargetBinaryDesc *binDesc, std::string_view outputPath);

    DriverContext &m_ctx;
};
```

**Emission Sequence**:
1. For each `MirFunction`:
   * Invoke `emitter.beginFunction(funcName)`.
   * For each `MirBlock`:
     * Invoke `emitter.bindLabel(blockId)`.
     * For each `MirInstruction`:
       * Query `MirTargetInstructionDesc`.
       * Map operands to hardware registers / memory / immediates.
       * Emit instruction bytes via `emitter.emitInst(desc, operands)`.
   * Invoke `emitter.endFunction()`.
2. Collect code sections from `TargetBinaryDesc` (`.text`, `.rodata`, `.data`, `.bss`).
3. Run `BranchRelaxer` on executable sections to finalize relative jump displacements.
4. Add symbols (global functions, static strings, relocations) to `Elf64Writer` or `CoffWriter`.
5. Write byte buffer directly to target file.

---

## 5. Implementation Roadmap (6 Phases)

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                       6-Phase Driver Implementation Roadmap                            │
└────────────────────────────────────────────────────────────────────────────────────────────────────────┘

  Phase 1: CLI Specification & Options Parser
  ├── argparse CLI argument definitions
  ├── Target triple parser & canonicalizer
  └── Basic CLI driver binary skeleton (ezc)
                         │
                         ▼
  Phase 2: DriverContext & Target Registry
  ├── Multi-tier PMR arena management (Session, Function, Pass)
  ├── SourceManager & DiagnosticCollector integration
  └── Target resolver binding (x86-64 ELF / COFF)
                         │
                         ▼
  Phase 3: FrontendAdapter & MIR Ingestion
  ├── Abstract IFrontendAdapter boundary
  ├── Programmatic MIR module loader
  └── Textual MIR parser integration / test mock loader
                         │
                         ▼
  Phase 4: Pass Pipeline Orchestration & Inspection Gates
  ├── Sequential pass runner (Middle-end + Backend)
  ├── Early exit handlers (--emit-mir, --emit-legalized-mir, --emit-lowered-mir)
  └── Pass timing and verbose diagnostic dumper
                         │
                         ▼
  Phase 5: Binary Emission Engine & Object Writing
  ├── Connect X86_64CodeEmitter to lowered MIR functions
  ├── Two-pass branch relaxation & symbol table generation
  └── Final file serialization via Elf64Writer / CoffWriter
                         │
                         ▼
  Phase 6: Quality Assurance, Integration Tests & CTest Integration
  ├── Unit tests for CLI parsing & target resolution
  ├── End-to-end compilation tests emitting valid ELF64 / PE-COFF files
  └── Disassembly verification & CTest suite expansion
```

### Phase 1: CLI Specification & Options Parser
* **Target Files**:
  * `EzCompiler/include/CommandLineOptions.h`
  * `EzCompiler/src/CommandLineOptions.cpp`
  * `EzCompiler/src/Main.cpp`
* **Deliverables**:
  1. Define `CommandLineOptions` struct matching all flags in Section 3.2.
  2. Implement `CommandLineParser` using `argparse`.
  3. Implement `TargetTriple` parser and tests for Linux, Windows, ELF, and COFF triples.
  4. Create executable target `ezc` in CMake.

### Phase 2: DriverContext & Target Registry
* **Target Files**:
  * `EzCompiler/include/DriverContext.h`, `EzCompiler/src/DriverContext.cpp`
  * `EzCompiler/include/TargetResolver.h`, `EzCompiler/src/TargetResolver.cpp`
* **Deliverables**:
  1. Implement `DriverContext` with session and function PMR arenas.
  2. Implement `TargetResolver` binding `x86_64` to `X86_64TargetDesc`.
  3. Hook up `DiagnosticCollector` and `DiagnosticLogger` with terminal color support.

### Phase 3: FrontendAdapter & MIR Ingestion
* **Target Files**:
  * `EzCompiler/include/FrontendAdapter.h`, `EzCompiler/src/FrontendAdapter.cpp`
* **Deliverables**:
  1. Define `IFrontendAdapter` interface.
  2. Implement programmatic and fixture-based MIR loader for backend validation.
  3. Verify clean error diagnostics on malformed inputs.

### Phase 4: Pass Pipeline Orchestration & Inspection Gates
* **Target Files**:
  * `EzCompiler/include/CompilationPipeline.h`, `EzCompiler/src/CompilationPipeline.cpp`
* **Deliverables**:
  1. Implement pass runner executing passes in strict topological order.
  2. Implement `--emit-mir` printer serializing generic MIR.
  3. Implement `--emit-legalized-mir` printer.
  4. Implement `--emit-lowered-mir` printer displaying target instructions with allocated hardware registers.
  5. Implement `--time-passes` stopwatch tracking pass durations.

### Phase 5: Binary Emission Engine & Object Writing
* **Target Files**:
  * `EzCompiler/include/EmissionEngine.h`, `EzCompiler/src/EmissionEngine.cpp`
* **Deliverables**:
  1. Bridge lowered `MirFunction` blocks and instructions to `X86_64CodeEmitter`.
  2. Coordinate `BranchRelaxer` label resolution.
  3. Populate sections and symbols in `Elf64Writer` (Linux) and `CoffWriter` (Windows).
  4. Write final binary files to disk and verify permissions.

### Phase 6: QA, Integration Tests & CTest Integration
* **Target Files**:
  * `tests/EzCompilerTestSuite/CMakeLists.txt`
  * `tests/EzCompilerTestSuite/tests/T_CommandLineParser.cpp`
  * `tests/EzCompilerTestSuite/tests/T_TargetResolver.cpp`
  * `tests/EzCompilerTestSuite/tests/T_EndToEndCompilation.cpp`
* **Deliverables**:
  1. 100% passing tests for CLI flags, invalid arguments, and help messages.
  2. End-to-end tests: compile synthetic functions with arithmetic, loops, memory access, and calls; assert that emitted `.o` / `.obj` files contain valid ELF/COFF headers and machine bytecode.
  3. Update CTest suite and execute `graphify update .`.

---

## 6. Directory Layout & CMake Configuration

```
EzPacker/
├── EzCompiler/
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── EzCompilerCommon.h
│   │   ├── CommandLineOptions.h
│   │   ├── DriverContext.h
│   │   ├── TargetResolver.h
│   │   ├── TargetTriple.h
│   │   ├── FrontendAdapter.h
│   │   ├── CompilationPipeline.h
│   │   └── EmissionEngine.h
│   └── src/
│       ├── CommandLineOptions.cpp
│       ├── DriverContext.cpp
│       ├── TargetResolver.cpp
│       ├── TargetTriple.cpp
│       ├── FrontendAdapter.cpp
│       ├── CompilationPipeline.cpp
│       ├── EmissionEngine.cpp
│       └── Main.cpp
├── tests/
│   └── EzCompilerTestSuite/
│       ├── CMakeLists.txt
│       ├── include/EzCompilerTestSuite.h
│       ├── src/EzCompilerTestSuite.cpp
│       └── tests/
│           ├── T_CommandLineParser.cpp
│           ├── T_TargetResolver.cpp
│           └── T_EndToEndCompilation.cpp
```

### CMake Configuration Sample
```cmake
# EzCompiler/CMakeLists.txt
set(EZCOMPILER_FILES_LIST
    "include/EzCompilerCommon.h"
    "include/CommandLineOptions.h"
    "include/DriverContext.h"
    "include/TargetResolver.h"
    "include/TargetTriple.h"
    "include/FrontendAdapter.h"
    "include/CompilationPipeline.h"
    "include/EmissionEngine.h"

    "src/CommandLineOptions.cpp"
    "src/DriverContext.cpp"
    "src/TargetResolver.cpp"
    "src/TargetTriple.cpp"
    "src/FrontendAdapter.cpp"
    "src/CompilationPipeline.cpp"
    "src/EmissionEngine.cpp"
)

# Core driver library
EzCMK_ConfigureLib(NAME "EzCompilerLib"
    FILE_LIST ${EZCOMPILER_FILES_LIST}
    PRECOMPILED_HEADER "include/EzCompilerCommon.h"
    LINKED_LIBRARIES "EzTriple;EzMir;EzCore;EzCodeEmitter;argparse"
    INCLUDED_DIRS "include;../EzCodeEmitter/include;../EzTriple/include;../EzMir/include;../EzCore/include"
)

# Main executable driver: ezc
add_executable(ezc "src/Main.cpp")
target_link_libraries(ezc PRIVATE EzCompilerLib)
set_target_properties(ezc PROPERTIES OUTPUT_NAME "ezc")
```

---

## 7. Audit Scorecards & Acceptance Gates

| Category | Verification Item | Test / Audit Method | Pass Criteria |
| :--- | :--- | :--- | :---: |
| **CLI** | Flag parsing & validation | `T_CommandLineParser` | All flags recognized; unknown flags exit with code 1 |
| **CLI** | Target triple canonicalization | `T_TargetResolver` | Correctly resolves `x86_64-linux-gnu` and `x86_64-windows-msvc` |
| **Pipeline** | Generic MIR dump (`--emit-mir`) | CLI execution on test IR | Outputs valid SSA MIR without executing backend |
| **Pipeline** | Legalized MIR dump (`--emit-legalized-mir`) | CLI execution on test IR | All types and ops legalized post-Legalizer |
| **Pipeline** | Lowered MIR dump (`--emit-lowered-mir`) | CLI execution on test IR | Machine instructions with allocated physical registers |
| **Emission** | ELF64 object writing | `T_EndToEndCompilation` | Generates valid ELF64 with `.text`, `.rodata`, `.symtab` |
| **Emission** | PE-COFF object writing | `T_EndToEndCompilation` | Generates valid PE-COFF with proper section alignments |
| **Safety** | Memory leak & arena hygiene | ASan test run | Zero memory leaks; function arenas cleanly reset |
| **Determinism**| Output reproducibility | Checksum comparison | Consecutive compilations generate byte-identical output |
| **CTest** | Full test suite pass rate | `ctest` execution | 100% tests passing across all test suites |
