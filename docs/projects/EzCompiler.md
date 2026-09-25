# EzCompiler Subproject Documentation

[EzPacker Documentation Index](../index.md) > **EzCompiler**

---

## 1. Overview & Architectural Role

`EzCompiler` is the top-level driver executable and compilation orchestrator of EzPacker. It coordinates front-end source loading, middle-end SSA optimization passes, EzTriple target lowering, machine code generation, and binary object serialization into a cohesive command-line compiler: `EzCompiler`.

```
+-----------------------------------------------------------------------------------+
|                               EzCompiler Architecture                             |
|                                                                                   |
|  Command-Line Arguments                                                           |
|        |                                                                          |
|        v                                                                          |
|  CommandLineParser ---> CommandLineOptions                                        |
|                               |                                                   |
|                               v                                                   |
|  DriverContext (PMR Arenas, DiagnosticCollector, SourceManager, TargetResolver)   |
|        |                                                                          |
|        v                                                                          |
|  FrontendAdapter (Reads .mir, invokes MirLexer/MirParser)                         |
|        |                                                                          |
|        v                                                                          |
|  CompilationPipeline                                                              |
|   +-- runMiddleEndPasses    (CFG Analysis, SSA Construction, Liveness)           |
|   |    [--emit-mir Gate]                                                          |
|   +-- runLegalizationPasses (Signature Legalizer, Operation Legalizer)            |
|   |    [--emit-legalized-mir Gate]                                                |
|   +-- runTargetLoweringPasses                                                     |
|        |-- MirAbiLowererPass                                                      |
|        |-- MirInstructionSelectorPass                                             |
|        |-- MirRegisterAllocatorPass                                               |
|        |-- MirFrameLowererPass                                                    |
|        [--emit-lowered-mir Gate]                                                  |
|        [--emit-asm Gate]                                                          |
|        |                                                                          |
|        v                                                                          |
|  EmissionEngine                                                                   |
|   +-- GenericCodeEmitter (Encodes instructions, relaxes branches)                 |
|   +-- IObjectWriter (Elf64Writer / CoffWriter)                                    |
|        |                                                                          |
|        v                                                                          |
|  Output File (.o / .obj / .s)                                                     |
+-----------------------------------------------------------------------------------+
```

---

## 2. Core Components

### 2.1 `CommandLineOptions` & `CommandLineParser` (`include/CommandLineOptions.h`)
Parses and validates command-line flags using `p-ranav/argparse`.

#### Key Configuration Fields:
- **`inputFilePath`**: Path to the source `.mir` module.
- **`outputFilePath`**: Destination path for emitted object or assembly files.
- **`target`**: Target triple (`TargetTriple`), defaulting to host architecture.
- **`emissionStage`**: Compilation stopping gate:
  - `EmissionStage::Object`: Complete compilation emitting native `.o` or `.obj` (default).
  - `EmissionStage::Assembly`: Halts after lowering and dumps textual assembly (`.s`).
  - `EmissionStage::GenericMir`: Halts after middle-end passes and dumps generic SSA MIR.
  - `EmissionStage::LegalizedMir`: Halts after legalization and dumps legalized MIR.
  - `EmissionStage::LoweredMir`: Halts after frame lowering and dumps machine-lowered MIR.
- **`optLevel`**: Optimization preset (`O0`, `O1`, `O2`, `Os`).
- **`printPasses`**: Prints the banner for each pass as it runs.
- **`timePasses`**: Measures and reports wall-clock execution time per pass.
- **`isPositionIndependent`**: Generates Position-Independent Code (`-fPIC`).
- **`targetFeatures`**: List of feature modifiers (e.g. `+avx`, `-sse4_2`).

---

### 2.2 `DriverContext` (`include/DriverContext.h`)
The central execution state container for a compilation invocation:
- Owns the root `std::pmr::memory_resource` for the entire compiler run.
- Owns `DiagnosticCollector` and attaches `DiagnosticLogger` configured with requested verbosity.
- Owns `SourceManager` for buffer management and coordinate translation.
- Resolves the requested target via `TargetResolver` and holds the active `ResolvedTarget` (containing `TargetDesc`, `TargetBinaryDesc`, and `CallingConvDesc`).

---

### 2.3 `TargetTriple` (`include/TargetTriple.h`)
```text
<architecture>-<vendor>-<operating_system>-<environment>
```
- **`getHostTriple()`**: Detects the host environment at compile time.
- **Object Format Predicates**: `isElf()`, `isCoff()`, `isMachO()`.
- **Architecture Predicates**: `isX86_64()`.
- **Operating System Predicates**: `isWindows()`, `isLinux()`, `isDarwin()`.

---

### 2.4 `TargetResolver` (`include/TargetResolver.h`)
A target factory registry allowing architecture libraries (such as `EzTargetsX86_64`) to register their target instantiation callbacks dynamically without introducing circular CMake dependencies.

```cpp
// Target registration signature
using TargetFactory = std::function<ResolvedTarget(
    const TargetTriple &triple,
    MirBuilderContext *mirCtx,
    bool isPositionIndependent,
    const std::vector<std::string> &features
)>;

TargetResolver::registerTarget("x86_64", x86_64Factory);
```

---

### 2.5 `CompilationPipeline` (`include/CompilationPipeline.h`)
The pass orchestrator driving execution across all functions in the module:
1. **Middle-End Stage**:
   - `CodeFlowAnalysisPass`: CFG and dominator tree construction.
   - `NonSsaToSsaPass`: Standard SSA conversion.
   - `LivenessAnalysisPass`: Virtual register live range analysis.
   - *Inspection Gate*: If `--emit-mir` is active, dumps generic SSA MIR and cleanly terminates.
2. **Legalization Stage**:
   - `MirFunctionSignatureLegalizerPass`: Legalizes function argument and return types.
   - `MirLegalizerPass`: Rewrites illegal opcodes and types into legal operations using target `LegalizerInfo`.
   - *Inspection Gate*: If `--emit-legalized-mir` is active, dumps legalized MIR and terminates.
3. **Target Lowering Stage**:
   - `MirAbiLowererPass`: Lowers calling conventions, placing parameters in registers or stack slots.
   - `MirInstructionSelectorPass`: Selects hardware instructions via Bottom-Up Maximal Munch.
   - `MirRegisterAllocatorPass`: Colors virtual registers with hardware physical registers.
   - `MirFrameLowererPass`: Computes stack layout and inserts function prologues and epilogues.
   - *Inspection Gate*: If `--emit-lowered-mir` is active, dumps machine MIR and terminates.
   - *Inspection Gate*: If `--emit-asm` is active, dumps textual assembly and terminates.

---

### 2.6 `EmissionEngine` (`include/EmissionEngine.h`)
The final stage of binary generation:
- Instantiates the target's `GenericCodeEmitter` via `TargetDesc::createCodeEmitter()`.
- Emits each function into the `.text` section of `CodeEmitterContext`.
- Binds labels and resolves internal relative branches using `BranchRelaxer`.
- Instantiates the appropriate `IObjectWriter` (`Elf64Writer` for ELF; `CoffWriter` for COFF).
- Transfers symbols and relocations to the writer and serializes the binary file to disk.

---

## 3. CLI Command-Line Reference

```bash
EzCompiler [options] <input-file>
```

| Flag | Description | Default |
| :--- | :--- | :--- |
| `<input-file>` | Path to the source `.mir` module. | *Required* |
| `-o, --output <path>` | Destination output file path. | `<input>.o` or `<input>.obj` |
| `-target <triple>` | Target triple (e.g. `x86_64-unknown-linux-gnu`, `x86_64-pc-windows-coff`). | Host triple |
| `--emit-mir` | Halts after middle-end passes and dumps generic SSA MIR. | Off |
| `--emit-legalized-mir`| Halts after legalization and dumps legalized MIR. | Off |
| `--emit-lowered-mir` | Halts after frame lowering and dumps lowered machine MIR. | Off |
| `--emit-asm, -S` | Halts after lowering and emits textual assembly. | Off |
| `-O0, -O1, -O2, -Os` | Sets the compiler optimization level. | `-O0` |
| `--print-passes` | Prints pass execution banners for debugging. | Off |
| `--time-passes` | Measures and reports elapsed time per pass. | Off |
| `-v, --verbose` | Enables verbose diagnostic logging. | Off |
| `-fPIC` | Generates Position-Independent Code. | Off |
| `-mattr=<+feat,-feat>`| Comma-separated target feature modifiers (e.g. `+avx,+sse4_2`). | Host defaults |
| `--diag-out <path>` | Emits diagnostics log to a specified file. | `stderr` |

---

## 4. API Reference & Further Reading

- Generated Doxygen API documentation: [Doxygen Documentation Index](../doxygen/index.html)
- Next subproject: [EzTargets Subproject Documentation](EzTargets.md)
- Return to [EzPacker Landing Page](../index.md)
