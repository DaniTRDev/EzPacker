# EzCompiler Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzCompiler.md) > **EzCompiler** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzCompiler` is the top-level driver and end-to-end compiler orchestration engine of EzPacker. It integrates all other subprojects (`EzCore`, `EzMir`, `EzTriple`, `EzCodeEmitter`, `EzTargets`) into a unified compilation pipeline and provides the `ezc` driver command-line executable.

```
                    Command Line Invocations (ezc)
                                  |
                                  v
                    +---------------------------+
                    |    CommandLineParser      |
                    +---------------------------+
                                  |
                                  v
                    +---------------------------+
                    |       DriverContext       |
                    | - Memory Session Arena    |
                    | - TargetResolver & Triple |
                    | - Diagnostics & SourceMgr |
                    | - MirBuilderContext       |
                    +---------------------------+
                                  |
                                  v
                    +---------------------------+
                    |     IFrontendAdapter      |
                    | (MirModuleLoader / Source)|
                    +---------------------------+
                                  |
                                  v
                    +---------------------------+
                    |    CompilationPipeline    |
                    | 1. Middle-End Passes      |
                    | 2. Legalization Passes    |
                    | 3. Target Lowering Passes |
                    +---------------------------+
                                  |
                                  v
                    +---------------------------+
                    |      EmissionEngine       |
                    | (Object Files / Assembly) |
                    +---------------------------+
```

---

## 2. Driver Context & Lifecycle (`DriverContext.h`)

`DriverContext` owns the lifetime of all compiler resources throughout a compilation invocation. It guarantees that memory allocated across passes shares a single root session arena while isolating pass-local allocations.

```cpp
namespace EzCompiler
{

class DriverContext
{
public:
    explicit DriverContext(const CommandLineOptions &options);
    ~DriverContext() = default;

    DriverContext(const DriverContext &) = delete;
    DriverContext &operator=(const DriverContext &) = delete;

    // Initializes session arena, diagnostics, type table, builder context,
    // and resolves target descriptors. Returns false on failure.
    bool initialize();

    std::pmr::memory_resource *getSessionAllocator();
    const CommandLineOptions &getOptions() const;

    SourceManager *getSourceManager();
    DiagnosticCollector *getDiagCollector();
    DiagnosticLogger *getDiagLogger();
    MirTypeTable *getTypeTable();
    MirBuilderContext *getBuilderContext();

    TargetDesc *getTargetDesc();
    CallingConvDesc *getCallingConv();
    TargetBinaryDesc *getBinaryDesc();
};

}
```

---

## 3. Target Triples & Dynamic Target Resolution

### 3.1 `TargetTriple` (`TargetTriple.h`)

Canonical representation of the compilation target in `<arch>-<vendor>-<sys>-<abi>` form:
- `static TargetTriple parse(std::string_view tripleStr)`: Parses 1-, 2-, 3-, or 4-part triples (e.g., `"x86_64-linux-gnu"`, `"x86_64-pc-windows-msvc"`, `"x86_64"`).
- `static TargetTriple getHostTriple()`: Detects host operating system and CPU architecture at runtime.
- Predicate queries:
  - `bool isX86_64() const`
  - `bool isWindows() const`
  - `bool isLinux() const`
  - `bool isElf() const`
  - `bool isCoff() const`

### 3.2 `TargetResolver` (`TargetResolver.h`)

Decouples driver orchestration from concrete architecture implementations:

```cpp
struct ResolvedTarget
{
    std::unique_ptr<TargetDesc> m_targetDesc;
    CallingConvDesc *m_callingConv{ nullptr };
    TargetBinaryDesc *m_binaryDesc{ nullptr };
};

using TargetFactory = std::function<ResolvedTarget(const TargetTriple &,
                                                   MirBuilderContext *,
                                                   bool isPositionIndependent,
                                                   const std::vector<std::string> &features)>;

class TargetResolver
{
public:
    static void registerTarget(std::string_view arch, TargetFactory factory);
    static ResolvedTarget resolve(const TargetTriple &triple,
                                  MirBuilderContext *mirCtx,
                                  bool isPositionIndependent,
                                  const std::vector<std::string> &features = {});
};
```

---

## 4. The Compilation Pipeline (`CompilationPipeline.h`)

`CompilationPipeline` orchestrates pass execution across all functions in the module, respecting early emission gates (`--emit-mir`, `--emit-legalized-mir`, `--emit-lowered-mir`, `--emit-asm`):

```cpp
class CompilationPipeline
{
public:
    explicit CompilationPipeline(DriverContext &ctx);

    bool runPipeline();
    std::string dumpCurrentMir() const;
    std::string dumpAssembly() const;

private:
    bool runMiddleEndPasses(MirFunction *func, MirPassManager &passManager);
    bool runLegalizationPasses(MirFunction *func, MirPassManager &passManager);
    bool runTargetLoweringPasses(MirFunction *func, MirPassManager &passManager);
};
```

### Pass Sequence:
1. **Middle-End Stage**:
   - `CodeFlowAnalysisPass`: CFG construction, loop analysis, dominator tree computation.
   - `NonSsaToSsaPass`: Cytron SSA construction with `PHI` node placement.
   - `LivenessAnalysisPass`: Backwards bit-vector analysis, live intervals calculation.
   - `MirPeepholePass` *(enabled when `optLevel != OptimizationLevel::O0`)*: Generic SSA algebraic identities, self/reciprocal move elimination, dead code elimination after terminators, and fall-through jump removal.
   - `LivenessAnalysisPass` *(refreshed when `optLevel != OptimizationLevel::O0`)*: Re-evaluates register intervals following peephole transformations.
2. **Legalization Stage**:
   - `MirFunctionSignatureLegalizerPass`: Legalizes formal parameters and returns against target register and stack conventions.
   - `MirLegalizerPass`: Table-driven rewrite of illegal opcodes and types (WidenScalar, NarrowScalar, Libcall, Custom).
3. **Target Lowering Stage**:
   - `MirAbiLowererPass`: ABI calling convention parameter and return token lowering.
   - `MirInstructionSelectorPass`: Bottom-Up Maximal Munch pattern matching and load folding.
   - `MirRegisterAllocatorPass`: Chaitin-Briggs graph coloring, spilling, and register rewriting. Runs with conservative coalescing (`coalesce()`), copy affinity biasing, and redundant copy removal enabled when `optLevel != OptimizationLevel::O0`.
   - `MirFrameLowererPass`: Prologue/epilogue insertion and abstract stack offset resolution.
   - `MirTargetPeepholePass` *(enabled when `optLevel != OptimizationLevel::O0`)*: Target machine-level peephole optimization (machine move/jump elimination, spill/reload forwarding, zero-identity ALU simplification, dead store elimination).

---

## 5. Machine Code Emission Engine (`EmissionEngine.h`)

`EmissionEngine` bridges lowered machine instructions into final object files on disk:

```cpp
class EmissionEngine
{
public:
    explicit EmissionEngine(DriverContext &ctx);
    bool emitModule(MirBuilderContext &mirCtx, std::string_view outputPath);
};
```

1. Obtains the target's `GenericCodeEmitter` via `TargetDesc::createCodeEmitter()`.
2. Emits instruction machine bytes into `CodeSection` node buffers.
3. Invokes `CodeSection::finalize()` to calculate label offsets and resolve alignment directives.
4. Selects the appropriate `IObjectWriter` (`Elf64Writer` or `CoffWriter`) based on `TargetBinaryDesc::getObjectFormat()`.
5. Emits symbol tables, relocations, and writes binary bytes to the destination file.

---

## 6. Command-Line Options (`ezc`)

### Exact CLI Options (`CommandLineOptions.h`)

```text
Usage: ezc [options] <input-file>

Positional Arguments:
  <input-file>                   Source file or textual .mir file to compile

Output Options:
  -o, --output <file>            Destination binary object or assembly file
  --diag-out <file>              Destination path for compiler diagnostic log

Target Configuration:
  --target <triple>              Target triple [default: host triple]
                                 (e.g. x86_64-linux-gnu, x86_64-pc-windows-msvc)
  -fPIC, --pic                   Generate position-independent code
  -mattr <features>              Target feature list (e.g. +avx, -sse)

Pipeline Stopping Gates:
  --emit-obj                     Emit native binary object (.o / .obj) [default]
  -S, --emit-asm                 Emit human-readable assembly text (.s)
  --emit-mir                     Stop and dump generic SSA MIR
  --emit-legalized-mir           Stop and dump legalized MIR
  --emit-lowered-mir             Stop and dump target-lowered machine MIR

Optimization Level:
  -O0                            No optimization
  -O1                            Basic optimizations
  -O2                            Full optimization pipeline
  -Os                            Size-focused optimization

Diagnostic and Timing Toggles:
  -v, --verbose                  Enable verbose compiler diagnostics
  --print-passes                 Print pass names as they execute
  --time-passes                  Report per-pass execution duration
  --diag-threshold <level>       Minimum reported severity (debug, trace, warning, error)
  --version                      Print compiler version banner
  -h, --help                     Display usage information
```

---

## 7. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| Options | `EzCompiler/include/CommandLineOptions.h` | `CommandLineOptions`, `CommandLineParser`, `EmissionStage`, `OptimizationLevel` |
| Driver Context | `EzCompiler/include/DriverContext.h` | `DriverContext` |
| Target Triple | `EzCompiler/include/TargetTriple.h` | `TargetTriple` |
| Target Resolver | `EzCompiler/include/TargetResolver.h` | `TargetResolver`, `ResolvedTarget`, `TargetFactory` |
| Pipeline | `EzCompiler/include/CompilationPipeline.h` | `CompilationPipeline` |
| Emission Engine | `EzCompiler/include/EmissionEngine.h` | `EmissionEngine` |
| Frontend | `EzCompiler/include/FrontendAdapter.h` | `IFrontendAdapter`, `MirModuleLoader` |
