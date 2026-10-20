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
    bool runInputVerification(MirFunction *func, MirPassManager &passManager);
    bool runMiddleEndPasses(MirFunction *func, MirPassManager &passManager);
    bool runLegalizationPasses(MirFunction *func, MirPassManager &passManager);
    bool runTargetLoweringPasses(MirFunction *func, MirPassManager &passManager);
};
```

### Pass Sequence:
0. **Input Verification Stage** (Pre-Middle-End):
   - `MirVerifierPass`: Validates input MIR structural invariants, expected operand kinds, DEF register constraints, and opcode flags (`SizeMatch`, `DestLarger`, `DestSmaller`, `TreatAsSigned`) on each function before middle-end transformations begin.
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

RTTI & Exception Configuration:
  --rtti                         Enable generation of runtime type info and source references [default]
  --no-rtti, -fno-rtti           Disable generation of runtime type info and source references

> [!NOTE]
> When `--rtti` is enabled (default), user RTTI descriptors are emitted and bound to exceptions. When `--no-rtti` is passed, user RTTI generation is suppressed, and exceptions automatically attach `__ez_default_rtti` and default payload at runtime, maintaining safe execution across calling conventions.


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

C Preprocessor Options:
  -I, --include-directory <dir>  Add directory to C header search path (may be repeated)
  -D, --define-macro <name[=val]> Predefine macro name with optional value (may be repeated)

Diagnostic and Timing Toggles:
  -v, --verbose                  Enable verbose compiler diagnostics
  --print-passes                 Print pass names as they execute
  --time-passes                  Report per-pass execution duration
  --diag-threshold <level>       Minimum reported severity (debug, trace, warning, error)
  --dump-pipeline-json <path>    Emit structured pass telemetry and MIR snapshots as JSON
  --version                      Print compiler version banner
  -h, --help                     Display usage information
```

---

## 7. Native C Frontend & chibicc Integration (`CFrontendAdapter`)

EzPacker embeds Rui Ueyama's C11 reference C compiler (`chibicc`) as a native, in-process C frontend library (`chibicc_frontend`). This allows `ezc` to directly ingest C source files (`.c`) and preprocessed C files (`.i`), parse them into an Abstract Syntax Tree (AST), and lower them into generic SSA MIR for downstream optimization and native code emission.

### Modular Architecture

The C frontend is decoupled into dedicated, single-responsibility components:

```
                  C Source (.c / .i)
                          |
                          v
             +--------------------------+
             |      ChibiccWrapper      | <--- [-I include_dirs, -D macro_defines]
             |  - Tokenize & Preprocess |
             |  - Standalone Macros     |
             |  - Parse C11 AST (Obj)   |
             |  - Error Trapping (SjLj) |
             +--------------------------+
                          |
                          v AST (Obj*)
             +--------------------------+
             |    CFrontendAdapter      | <--- High-level coordinator
             |  - Global Var Lowering   |
             |  - Function Dispatch     |
             +--------------------------+
                     /          \
                    v            v
  +--------------------+      +------------------------------+
  | CFrontendTypeMapper|      |  CFrontendFunctionLowerer    |
  | - Integral Types   |      |  - Param / Local Allocation  |
  | - Pointers/Arrays  |      |  - Address Lowering (lowerAddr)|
  | - Structs/Unions   |      |  - CFG & Branch Construction |
  | - Function Types   |      |  - Expression / Stmt Lowering|
  +--------------------+      +------------------------------+
                                             |
                                             v
                                    MirModule (SSA MIR)
                                             |
                                             v
                                    CompilationPipeline
```

### Key Components

1. **`ChibiccWrapper` (`EzCompiler/include/CFrontend/ChibiccWrapper.h`)**:
   - Encapsulates chibicc's global tokenizer, preprocessor, and parser states in a clean, re-entrant C++ interface.
   - Manages include search paths (`addIncludePath`) and command-line macro definitions (`addMacroDefine`).
   - Implements thread-safe / re-entrant error trapping via `setjmp` / `longjmp`. Syntax errors, missing includes, or unclosed macros are captured and converted into EzPacker `DiagnosticCollector` messages rather than abruptly calling `exit(1)`.
   - Cleans and isolates preprocessor caches, include guards, and macro tables between compilation units via `chibicc_reset_preprocessor()`.

2. **`CFrontendTypeMapper` (`EzCompiler/include/CFrontend/CFrontendTypeMapper.h`)**:
   - Standalone type conversion engine that maps chibicc `Type` structures to EzPacker `MirType` instances within `MirTypeTable`.
   - Maps C primitives (`void`, `_Bool`, `char`, `short`, `int`, `long`, `float`, `double`, `long double`), pointers, arrays, structs, unions, and function types.

3. **`CFrontendFunctionLowerer` (`EzCompiler/include/CFrontend/CFrontendFunctionLowerer.h`)**:
   - Specialized function-level AST lowerer translating chibicc AST nodes into basic blocks (`MirBlock`) and instructions (`MirInstruction`).
   - Analyzes address-taken variables (`collectAddressTaken`) and allocates parameters/locals to virtual registers or static stack slots (`StackFrameObject`) accordingly.
   - Computes lvalue memory addresses uniformly via `lowerAddr()`, supporting direct variables, dereferences (`*ptr`), struct member accesses (`struct.member` and `ptr->member`), and array accesses.
   - Constructs structured control flow (`if-else`, loops, `break`, `continue`, `return`) and arithmetic, logical, comparison, and casting expressions.

4. **`CFrontendAdapter` (`EzCompiler/include/CFrontend/CFrontendAdapter.h`)**:
   - Implements `IFrontendAdapter` to interface with `MirModuleLoader`.
   - Orchestrates the AST translation pass (`lowerAstToMir`), creating global variables (`MirGlobalVar`) with proper linkage (`Internal` for static, `External` for extern) and delegating each function definition to `CFrontendFunctionLowerer`.

5. **Standalone / Freestanding Software Orientation**:
   - The C frontend is strictly oriented for standalone/freestanding systems programming (operating system kernels, bootloaders, embedded firmware, and freestanding runtime software).
   - Standard conformance: `__STDC_HOSTED__` is set to `0`. The preprocessor automatically injects `__STDC_FREESTANDING__ = 1` and `__EZPACKER_STANDALONE__ = 1`.
   - All hosted standard library assumptions (e.g. `stdio.h`, `stdlib.h`, and hosted target macros like `linux` or `unix`) are eliminated.
   - Command-line flags `-ffreestanding` and `-nostdlib` are supported by `CommandLineParser` and forwarded to the driver.

6. **Bundled ISO C11 Freestanding Headers**:
   - In conformance with ISO C11 §4.p6 ("Freestanding implementations"), EzPacker provides all 9 mandated freestanding C11 headers:
     - `<stdint.h>`: Fixed-width integer types (`int8_t` through `uint64_t`), least, fast, pointer-holding (`intptr_t`, `uintptr_t`), and maximum-width types with corresponding limits and literal macros.
     - `<stddef.h>`: Fundamental standard definitions (`size_t`, `ptrdiff_t`, `wchar_t`, `max_align_t`, `NULL`, and `offsetof`).
     - `<stdbool.h>`: Boolean type macro (`bool` -> `_Bool`), `true`, `false`, and `__bool_true_false_are_defined`.
     - `<limits.h>`: Basic integral type ranges and sizes (`CHAR_BIT`, `INT_MIN/MAX`, `UINT_MAX`, `LONG_MIN/MAX`, `ULLONG_MAX`).
     - `<stdalign.h>`: Alignment specifier macros (`alignas`, `alignof`, `__alignas_is_defined`, `__alignof_is_defined`).
     - `<stdnoreturn.h>`: Function return convenience macro (`noreturn` -> `_Noreturn`).
     - `<iso646.h>`: Alternative operator spellings (`and`, `or`, `not`, `xor`, `bitand`, `bitor`, `compl`, etc.).
     - `<float.h>`: IEEE 754 floating-point characteristics, radix, limits, mantissa digits, and epsilon definitions.
     - `<stdarg.h>`: Variadic argument macros (`va_list`, `va_start`, `va_arg`, `va_copy`, `va_end`) matching chibicc calling convention ABI.

7. **Dual-Resolution Architecture (Disk Auto-Discovery + In-Memory Fallback)**:
   - **Disk Auto-Discovery**:
     - `CFrontendAdapter::getBundledFreestandingIncludeDir()` and `ChibiccWrapper::getBundledFreestandingIncludeDir()` automatically locate the headers directory on disk by querying (1) the `EZPACKER_FREESTANDING_DIR` environment variable, (2) the compiled-in definition `EZPACKER_FREESTANDING_INCLUDE_DIR`, (3) executable-relative paths (`include/freestanding`), and (4) working directory fallbacks.
     - The discovered path is injected into chibicc's preprocessor search path list.
   - **In-Memory Fallback**:
     - If the headers directory is not present on disk (such as when relocating binaries without copying header assets), chibicc's preprocessor (`preprocess.c` and `tokenize.c`) falls back to virtual paths `<builtin>/<header.h>`.
     - Content is retrieved instantly from embedded in-memory string literals in `builtin_headers.h`, guaranteeing 100% resilient zero-setup compilation anywhere.

8. **CLI Driver Ingestion (`FrontendAdapter.cpp`)**:
   - `FrontendAdapter` inspects the input file extension. When `.c` or `.i` is detected, it delegates parsing to `CFrontendAdapter`, passing all configured `-I` include directories and `-D` macro definitions, and automatically resolving bundled freestanding headers.

9. **Complete 48 AST Node Lowering (100% chibicc Coverage)**:
   The C frontend provides full semantic lowering for all 48 AST node types defined by chibicc:
   - **Control Flow**:
     - `ND_SWITCH`, `ND_CASE` (including GNU extension range cases `case low ... high:`).
     - `ND_GOTO`, `ND_LABEL`, computed indirect gotos via `ND_GOTO_EXPR` (`goto *ptr;`), and address-of-label via `ND_LABEL_VAL` (`&&label`).
     - Loops (`ND_FOR`, `ND_DO`), branches (`ND_IF`), blocks (`ND_BLOCK`), returns (`ND_RETURN`), and statement expressions (`ND_STMT_EXPR`).
   - **Dynamic Stack Allocation & Memory Zeroing**:
     - `ND_VLA_PTR`: Variable-length arrays and dynamic allocations lowered to `MirInstructionOpCode::DALLOC`.
     - `ND_MEMZERO`: Lowers variable zero-initialization using scalar stores for small types or runtime `@memset` calls for aggregated types.
   - **Bitfield Access Lowering**:
     - Bitfield Read (`ND_MEMBER`): Emits container scalar load, left-shifts to place bits at MSB, followed by signed arithmetic shift right (for signed fields) or logical shift right (for unsigned fields).
     - Bitfield Write (`ND_ASSIGN`): Emits container scalar load, clears target bit slice using a bitwise AND mask, shifts and masks the incoming value, merges via bitwise OR, stores back to the container, and sign/zero-extends the expression result.
   - **Function Calls & Symbol Resolution**:
     - Two-pass module symbol resolution in `CFrontendAdapter`: Declares all function symbols upfront in `MirBuilderContext` before lowering function bodies. This guarantees mutual recursion, function pointers, and chibicc's reverse AST function ordering resolve seamlessly without forward-reference errors.
     - Direct and Indirect Calls (`ND_FUNCALL`): Evaluates callee expressions; emits direct calls for function symbols and dynamic register-indirect calls `CALL(dest, calleeReg, args...)` for function pointers.
   - **Type Conversions & Casting (`ND_CAST`)**:
     - Complete casting semantics: integer extensions (`SEXT`, `ZEXT`), truncations (`TRUNC`), signed integer to floating-point (`SITOFP`), floating-point to signed integer (`FPTOSI`), float widenings (`FPEXT`), float truncations (`FPTRUNC`), float-to-bool testing (`FCMP` against 0.0), and compile-time numeric constant folding.
   - **Target DSL Integrated Inline Assembly (`ND_ASM`)**:
     - First-class inline assembly lowering querying the Target DSL instruction tables (`TargetDesc::findInstructionDesc(mnemonic)`).
     - Comprehensive support for SYSTEM instructions across both `x86_64` (e.g. `CPUID`, `INVD`, `WBINVD`, `INVLPG`, `HLT`, `RSM`, `RDMSR`, `WRMSR`, `RDTSC`, `RDPMC`, `SYSENTER`, `SYSEXIT`, `SYSCALL`, `SYSRET`, `SWAPGS`, `XGETBV`, `XSETBV`, `CLAC`, `STAC`, `RDRAND`, `RDSEED`) and `riscv64` (e.g. `ECALL`, `EBREAK`, `MRET`, `SRET`, `WFI`, `FENCE.I`, `SFENCE.VMA`, `CSRRW`, `CSRRS`, `CSRRC`, `CSRRWI`, `CSRRSI`, `CSRRCI`).
     - Strict diagnostic verification: Emits compiler errors via `m_driverCtx.getDiagCollector()->error(...)` with source locations whenever unrecognized mnemonics or unsupported assembly instructions are encountered.
   - **Atomic Primitives**:
     - `ND_CAS`: Atomic Compare-And-Swap.
     - `ND_EXCH`: Atomic Exchange.

---


---

## 8. Native Pipeline Telemetry Engine (PipelineTelemetry)

EzCompiler includes a high-performance native pipeline telemetry recorder (PipelineTelemetry.h / PipelineTelemetry.cpp) designed for automated verification, compiler developer debugging, and web workbench integration.

### Core Capabilities
- **Per-Pass Telemetry Records**:
  - 
ame: Name of the executed pass (e.g., MirVerifierPass, NonSsaToSsaPass, MirPeepholePass, MirLegalizerPass, MirInstructionSelectorPass, MirRegisterAllocatorPass, MirFrameLowererPass).
  - stage: Compilation stage (Verification, MiddleEnd, Legalization, TargetLowering).
  - unction: Target function name for function-level passes.
  - 	ype: FunctionPass or ModulePass.
  - durationMs: High-resolution pass execution duration in milliseconds.
  - modifiedMir: Boolean mutation flag indicating whether the pass modified instructions, operands, or basic blocks.
  - instructionCount & lockCount: Function metrics post-pass.
  - mirDump: Complete formatted MIR text snapshot immediately following the pass.
- **Deep Analysis Topologies**:
  - locks: Basic block definitions with numeric IDs, label names, entry flags, predecessors, successors, and instruction listings.
  - liveness: Register definition (def), use (use), liveIn, and liveOut sets per block.
  - rameLayout: Total stack frame size, callee-saved registers, and stack slots.
- **Diagnostics Trapping**:
  - Structured capture of all frontend, parser, and pass diagnostics (error, warning, 	race, debug) with source locations.
  - Telemetry JSON is written deterministically even when compilation fails, providing compiler engineers with complete diagnostic and pass history up to the point of failure.

### Command-Line Usage
`ash
ezc input.mir -o output.o --dump-pipeline-json pipeline.json
`

## 9. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| Options | `EzCompiler/include/CommandLineOptions.h` | `CommandLineOptions`, `CommandLineParser`, `EmissionStage`, `OptimizationLevel` |
| Driver Context | `EzCompiler/include/DriverContext.h` | `DriverContext` |
| Target Triple | `EzCompiler/include/TargetTriple.h` | `TargetTriple` |
| Target Resolver | `EzCompiler/include/TargetResolver.h` | `TargetResolver`, `ResolvedTarget`, `TargetFactory` |
| Pipeline | `EzCompiler/include/CompilationPipeline.h` | `CompilationPipeline` |
| Emission Engine | `EzCompiler/include/EmissionEngine.h` | `EmissionEngine` |
| Frontend | `EzCompiler/include/FrontendAdapter.h` | `IFrontendAdapter`, `MirModuleLoader` |
| C Frontend Wrapper | `EzCompiler/include/CFrontend/ChibiccWrapper.h` | `ChibiccWrapper` |
| C Frontend Type Mapper | `EzCompiler/include/CFrontend/CFrontendTypeMapper.h` | `mapType`, `CFrontendTypeMapper` |
| C Frontend Lowerer | `EzCompiler/include/CFrontend/CFrontendFunctionLowerer.h` | `CFrontendFunctionLowerer` |
| C Frontend Adapter | `EzCompiler/include/CFrontend/CFrontendAdapter.h` | `CFrontendAdapter` |
| Pipeline Telemetry | EzCompiler/include/PipelineTelemetry.h | PipelineTelemetry, PassRecord, FunctionTelemetry |
