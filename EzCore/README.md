# EzCore: Foundational Infrastructure, Diagnostics & Arbitrary-Precision Math

[`EzCore`](file:///E:/Repos/EzPacker/EzCore) is the foundational utility and infrastructure library of the **EzPacker** compiler toolchain. It provides the low-level, high-performance, and modular subsystems underpinning all compiler layers—from lexical analysis and semantic parsing to Machine Intermediate Representation ([`EzMir`](file:///E:/Repos/EzPacker/EzMir)) and binary code emission ([`EzCodeEmitter`](file:///E:/Repos/EzPacker/EzCodeEmitter)).

---

## Architecture Overview

```
                          ┌────────────────────────────────────────────────────────┐
                          │         Compiler Subsystems & Compilation Phases       │
                          │   (EzDsl Lexer/Sema, EzMir IR, EzTriple, EzCompiler)   │
                          └───────────────────────────┬────────────────────────────┘
                                                      │
              ┌───────────────────────────┬───────────┴───────────┬───────────────────────────┐
              ▼                           ▼                       ▼                           ▼
   ┌──────────────────────┐    ┌──────────────────────┐┌──────────────────────┐    ┌──────────────────────┐
   │ Diagnostics System   │    │  FlexNumber Math     ││ Source Management    │    │ Helper Utilities     │
   │ - DiagnosticCollector│    │ - FlexInt            ││ - GenericSourceMgr   │    │ - DenseBitSet        │
   │ - DiagnosticBuilder  │    │   (LibTomMath)       ││ - SourceManager      │    │ - IntrusiveLinkedList│
   │ - DiagnosticScope    │    │ - FlexFloat (LibBF)  ││ - SourceReference    │    │ - NameRegistry       │
   │ - DiagnosticLogger   │    │ - LibBFWrapper       ││ - Binary Line Mapping│    │ - StringUtils        │
   └──────────┬───────────┘    └──────────┬───────────┘└──────────┬───────────┘    │ - CliExitCode        │
              │                           │                       │                └──────────┬───────────┘
              └───────────────────────────┼───────────────────────┴───────────────────────────┘
                                          │
                                          ▼
                         ┌─────────────────────────────────┐
                         │ Polymorphic Memory Res (pmr)    │
                         │ - std::pmr Monotonic Arenas     │
                         │ - Synchronized Pool Allocators  │
                         └─────────────────────────────────┘
```

---

## Core Subsystems

### 1. Diagnostics Management Subsystem (`EzCore/include/Diagnostics/`)

The Diagnostics subsystem enables compiler phases to emit rich, structured diagnostics (errors, warnings, notes, and trace records) without coupling algorithms directly to standard I/O streams or fixed loggers.

* **[`DiagnosticCollector`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticCollector.h)**:
  - Central thread-safe hub coordinating compilation diagnostics.
  - Manages active severity bitmasks (`m_enabledDiags`) using `std::atomic<uint8_t>` to allow lock-free early filtering.
  - Maintains an internal stack of [`DiagnosticScope`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticScope.h) instances backed by a `std::pmr::synchronized_pool_resource`.
  - Supports dynamic registration and removal of [`DiagnosticListener`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticListener.h) sinks under a `std::recursive_mutex`.
  - Severity categories defined in [`DiagnosticMessageType`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticMessage.h):
    - `Diag_None = 0`: Inactive flag.
    - `Diag_Debug = 1`: Low-level developer/pass tracing.
    - `Diag_Error = (1 << 1)`: Semantic or syntax violation preventing successful compilation.
    - `Diag_Trace = (1 << 2)`: Compiler phase transition logging.
    - `Diag_Warning = (1 << 3)`: Non-fatal issue (e.g. unused labels or implicit narrowing).

* **[`DiagnosticBuilder`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticBuilder.h)**:
  - Single-threaded, fluent RAII builder for constructing diagnostics.
  - Features zero-allocation short-circuiting: when the requested severity is disabled in the collector, formatting templates (`std::format_string`) are bypassed entirely and no strings are allocated.
  - Supports stream chaining with `operator<<` for string fragments and [`SourceReference*`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/GenericSourceManager.h) location attachments.
  - Automatically flushes its assembled message into the parent collector upon destruction (`~DiagnosticBuilder()`) or explicit `.flush()`.
  - Move-constructible to transfer message assembly across call boundaries without triggering premature flushes.

* **[`DiagnosticScope`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticScope.h) & [`DiagnosticScopeAction`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticScope.h)**:
  - Controls how collected messages are dispatched when a compilation block terminates:
    - `Commit`: Immediately notifies all registered listeners with every message collected in the scope.
    - `Discard`: Drops and frees all gathered diagnostics without notifying listeners. Essential for speculative parsing, lookahead backtracking, and pattern probing in `EzDsl` and `EzMir`.
    - `Propagate`: Bubbles collected diagnostics up to the enclosing parent scope via [`DiagnosticScope::moveMessagesTo()`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticScope.h). If no parent scope exists, commits directly to listeners.

* **[`DiagnosticMessage`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticMessage.h) & [`DiagnosticNote`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticMessage.h)**:
  - Immutable diagnostic records storing severity type, sender component name (`std::pmr::string`), primary [`SourceReference*`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/GenericSourceManager.h), and formatted main message text.
  - Holds an intrusive `std::list` of [`DiagnosticNote`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticMessage.h) objects, each linking secondary source references with clarifying remarks.

* **[`DiagnosticListener`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticListener.h) & [`DiagnosticLogger`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticLogger.h)**:
  - [`DiagnosticListener`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticListener.h): Abstract observer interface declaring `virtual void onDiag(const DiagnosticMessage &msg) = 0`.
  - [`DiagnosticLogger`](file:///E:/Repos/EzPacker/EzCore/include/Diagnostics/DiagnosticLogger.h): Production listener bridging diagnostics into `EzLogger`. Translates attached [`SourceReference*`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/GenericSourceManager.h) spans through [`SourceManager`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h) to print source file paths, 1-based line numbers, column positions, the raw source code line, and ANSI-colored caret indicators (`^~~~~`). Optional file redirection via `outLogPath`.

---

### 2. FlexNumber Arbitrary-Precision Math Subsystem (`EzCore/include/FlexNumber/`)

Modern compiler middle-ends and code emitters require constant folding, scalar legalization, and immediate truncation across target bitwidths (e.g. `i1` through `i1024+` bits) without host architecture limitations.

* **[`FlexInt`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h)**:
  - High-performance arbitrary-precision integer wrapper built on **LibTomMath** (`mp_int`).
  - Supports arbitrary configurable bit widths (`m_bitWidth`) and signed/unsigned semantics.
  - Strict two's-complement bounds clamping: [`clampToTwosComplement(OverflowPolicy policy)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L345) supports:
    - [`OverflowPolicy::Wrap`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L12): Silent modular wrapping modulo $2^N$ (standard hardware integer arithmetic).
    - [`OverflowPolicy::Trap`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L13): Throws `std::overflow_error` or `std::underflow_error` on range violation.
  - Complete arithmetic suite: `+`, `-`, `*`, `/`, `%`, `++`, `--`, with overflow tracking variants: [`addWithOverflow()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L228), [`subWithOverflow()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L233), [`mulWithOverflow()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L238).
  - Bitwise manipulation: bitwise NOT (`~`), AND (`&`), OR (`|`), XOR (`^`), logical shift left ([`shl()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L278)), logical shift right ([`lshr()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L281)), and arithmetic sign-extending shift right ([`ashr()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L286)).
  - Bit slicing and extraction: [`extractBits(startBit, numBits)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L313), [`extractWord64(wordIndex)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L318), [`getHighHalf()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L138), and [`getLowHalf()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L143) for scalar legalization passes.
  - Target endianness serialization:
    - [`writeBytes(std::span<uint8_t> dest, Endianness endian)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L323): Dumps raw binary bytes directly into emission buffers.
    - [`readBytes(std::span<const uint8_t> src, bitWidth, isSigned, endian)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L328): Ingests target machine bytes into `FlexInt`.
    - [`fromLimbs64(std::span<const uint64_t> limbs, bitWidth, isSigned)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexInt.h#L223): Zero-copy construction from native 64-bit limb words.

* **[`FlexFloat`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h)**:
  - IEEE-754 compliant arbitrary-precision floating-point wrapper built atop **LibBF** (`bf_t`).
  - Supports arbitrary precisions from 32-bit single precision, 64-bit double precision, 128-bit quad precision, up to 256+ bits.
  - Bounds clamping and rounding modes: [`clampToFloatBounds()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h#L215) enforces IEEE mantissa precision and exponent ranges using `BF_RNDN` (Round to Nearest, ties to Even).
  - Floating-point arithmetic: `+`, `-`, `*`, `/`, with divide-by-zero domain error validation.
  - Half-splitting: [`getHighHalf()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h#L112) (sign and exponent fields) and [`getLowHalf()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h#L117) (mantissa bits) for floating-point expansion rules.
  - IEEE-754 binary serialization & bitcasting:
    - [`writeIeeeBytes(std::span<uint8_t> dest, Endianness endian)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h#L191): Emits exact hardware float bytes into object sections.
    - [`readIeeeBytes(std::span<const uint8_t> src, bitWidth, endian)`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h#L196): Ingests raw float bytes.
    - [`bitcastToFlexInt()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h#L202) and [`bitcastFromFlexInt()`](file:///E:/Repos/EzPacker/EzCore/include/FlexNumber/FlexFloat.h#L207): Lossless bitwise reinterpretation between integer and floating-point representations.

* **[`LibBFWrapper`](file:///E:/Repos/EzPacker/EzCore/include/LibBFWrapper.h)**:
  - RAII C-linkage isolation header encapsulating LibBF inside `namespace libbf`. Prevents naming collisions between LibBF C symbols (`bf_t`, `limb_t`) and standard compiler types.

---

### 3. Source Management & Location Tracking Subsystem (`EzCore/include/SourceManager/`)

The source management subsystem delivers memory-efficient file buffering, include path resolution, and debug reference translation for AST and MIR nodes.

* **[`GenericSourceManager`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/GenericSourceManager.h)**:
  - Abstract base interface decoupling source tracking from concrete storage schemes. Declares query methods for line content, source text slices, and reference creation.

* **[`SourceManager`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h)**:
  - Production source registry backed by `std::pmr::memory_resource`.
  - Stores `SourceFileEntry` objects containing file buffers and precomputed line bounds.
  - O(1) lookup by 1-based numeric source ID (`getSourceName()`, `getSourceContent()`) or canonical path.
  - Include resolution: [`addIncludePath()`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L66), [`resolveSourcePath()`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L73), and [`loadFile()`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L80) resolve include hierarchies relative to working directories and search paths.
  - In-memory source registration: [`addSourceContent()`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L41) allows direct ingestion of string buffers for testing or synthetic inputs.

* **[`SourceReference`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/GenericSourceManager.h)**:
  - Compact 16-byte structure (`m_startOffset`, `m_endOffset`, `m_sourceFileId`) embedded directly inside AST nodes and [`MirInstruction`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstruction.h) instances.
  - Completely decoupled from file text; retains full provenance while avoiding string copies.

* **Binary-Searched Line Lookup**:
  - [`SourceManager::getReferenceLine()`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L61) performs $O(\log N)$ binary search over the entry's precomputed `SourceLineRange` table to translate byte offsets into 1-based line numbers and column offsets.
  - Zero-copy line slicing via [`getRawLineContent()`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L86) and span slicing via [`getReferenceContent()`](file:///E:/Repos/EzPacker/EzCore/include/SourceManager/SourceManager.h#L91).

---

### 4. Helper Classes & High-Performance Utilities

* **[`DenseBitSet`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/DenseBitSet.h)**:
  - Fast, 64-bit word-aligned dense bitset used for compiler dataflow analysis and register liveness tracking.
  - Operates over contiguous `uint64_t` words stored in a `std::vector<uint64_t>`.
  - Core operations:
    - [`set(bit)`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/DenseBitSet.h#L28): Sets bit $i$ to 1 ($O(1)$).
    - [`test(bit)`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/DenseBitSet.h#L34): Tests if bit $i$ is set ($O(1)$).
    - [`unionWith(other)`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/DenseBitSet.h#L40): In-place bitwise OR ($\text{this} \gets \text{this} \cup \text{other}$); returns `true` if any bits changed.
    - [`computeLiveIn(use, liveOut, def)`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/DenseBitSet.h#L46): Evaluates standard compiler backward dataflow liveness transfer equation in a single word-level pass:
      $$\text{LiveIn} = \text{Use} \cup (\text{LiveOut} \setminus \text{Def})$$
      Returns `true` if $\text{LiveIn}$ changed, enabling clean fixed-point loop termination in [`LivenessAnalysisPass`](file:///E:/Repos/EzPacker/EzMir/include/MirPasses/Passes/LivenessAnalysisPass.h).

* **[`IntrusiveLinkedList<T>`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/IntrusiveLinkedList.h)**:
  - High-performance doubly-linked list container embedding prev/next pointers directly inside element nodes (`getPrev()`, `setPrev()`, `getNext()`, `setNext()`).
  - **Zero allocation overhead**: Inserting, removing, and moving nodes requires zero dynamic heap allocations.
  - Supports bidirectional iterators (`begin()`, `end()`, `rbegin()`, `rend()`).
  - $O(1)$ splice primitives:
    - Splice entire list: [`splice(pos, other)`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/IntrusiveLinkedList.h#L348).
    - Splice single node: [`splice(pos, other, it)`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/IntrusiveLinkedList.h#L404).
    - Splice range: [`splice(pos, other, first, last)`](file:///E:/Repos/EzPacker/EzCore/include/HelperClasses/IntrusiveLinkedList.h#L515).
  - Used extensively by [`MirBlock`](file:///E:/Repos/EzPacker/EzMir/include/Block/MirBlock.h) to contain [`MirInstruction`](file:///E:/Repos/EzPacker/EzMir/include/Instruction/MirInstruction.h) nodes.

* **[`NameRegistry<T>`](file:///E:/Repos/EzPacker/EzCore/include/NameRegistry.h)**:
  - Case-insensitive, alias-aware symbol table mapping names to values of `T`.
  - Normalizes keys by folding ASCII characters to lowercase and '-' to '_' via [`NormalizeKey()`](file:///E:/Repos/EzPacker/EzCore/include/StringUtils.h#L116).
  - Allows seamless aliasing between name variants (e.g. `"x86_64"`, `"x86-64"`, and `"AMD64"` map to the same entry).
  - Methods: [`add(name, val)`](file:///E:/Repos/EzPacker/EzCore/include/NameRegistry.h#L25), [`find(name)`](file:///E:/Repos/EzPacker/EzCore/include/NameRegistry.h#L36), and [`contains(name)`](file:///E:/Repos/EzPacker/EzCore/include/NameRegistry.h#L48).

* **[`StringUtils`](file:///E:/Repos/EzPacker/EzCore/include/StringUtils.h)**:
  - High-speed string manipulation routines:
    - [`EscapeString(value, mode)`](file:///E:/Repos/EzPacker/EzCore/include/StringUtils.h#L24): Escapes characters for C++ string literals (`EscapeMode::CppStringLiteral`) or JSON output (`EscapeMode::Json`).
    - [`IsCppIdentifierChar(c)`](file:///E:/Repos/EzPacker/EzCore/include/StringUtils.h#L75): Validates legal C++ identifier characters.
    - [`SanitizeCppIdentifier(raw, fallback)`](file:///E:/Repos/EzPacker/EzCore/include/StringUtils.h#L87): Rewrites strings into valid C++ identifiers, replacing illegal characters with `_` and prepending `_` to leading digits.
    - [`StrToLower(str)`](file:///E:/Repos/EzPacker/EzCore/include/StringUtils.h#L134) and [`StrToUpper(str)`](file:///E:/Repos/EzPacker/EzCore/include/StringUtils.h#L148): Safe ASCII casing conversions.

* **[`CliExitCode`](file:///E:/Repos/EzPacker/EzCore/include/CliExitCode.h)**:
  - Standardized process exit codes shared between `EzDslCli` and `ezc`:
    - [`EzCli::kSuccess = 0`](file:///E:/Repos/EzPacker/EzCore/include/CliExitCode.h#L13): Requested execution succeeded (or help/version was displayed).
    - [`EzCli::kError = 1`](file:///E:/Repos/EzPacker/EzCore/include/CliExitCode.h#L14): User usage error, syntax error, or compilation failure.
    - [`EzCli::kFatalException = 2`](file:///E:/Repos/EzPacker/EzCore/include/CliExitCode.h#L15): An unexpected fatal exception escaped the compiler driver.

---

## Memory Architecture & Lifecycle

`EzCore` enforces structured memory management through C++ Polymorphic Memory Resources (`std::pmr`):
- **Compilation Session Arenas**: Core registries, source buffers, and diagnostics use `std::pmr::monotonic_buffer_resource` or `std::pmr::synchronized_pool_resource`.
- **Zero-Allocation Hot Paths**: Diagnostic builders and bitset transfers allocate within thread-local arenas or inline buffers.
- **Bulk Reclamation**: When a compilation session completes, disposing of the root memory resource instantly reclaims all memory without walking individual object trees.

---

## Working Code Examples

### 1. Diagnostics with Scoped Commit/Discard Backtracking

```cpp
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "SourceManager/SourceManager.h"
#include <iostream>

void parseWithBacktracking()
{
    std::pmr::synchronized_pool_resource memPool;
    SourceManager sourceMgr(std::filesystem::current_path(), &memPool);
    DiagnosticCollector diags;

    DiagnosticLogger logger(&sourceMgr);
    diags.addListener(&logger);

    size_t fileId = sourceMgr.addSourceContent("speculative.ez", "func test(): i32 { return 'x'; }");
    SourceReference *tokRef = sourceMgr.createReference(26, 3, fileId);

    // 1. Begin speculative parsing scope
    diags.beginScope(DiagnosticScopeAction::Discard);
    {
        // Try parsing rule A...
        diags.error("SpeculativeParser", "Rule A failed: expected integer literal") << tokRef;
    }
    // Speculation failed; discard all errors emitted during Rule A attempt
    diags.endScope();

    // 2. Begin committed scope for rule B
    diags.beginScope(DiagnosticScopeAction::Commit);
    {
        diags.warn("Parser", "Rule B matched with implicit char-to-int promotion") << tokRef;
    }
    diags.endScope();
}
```

### 2. Arbitrary-Precision Math & Endianness Serialization

```cpp
#include "FlexNumber/FlexInt.h"
#include "FlexNumber/FlexFloat.h"
#include <iostream>
#include <vector>

void performArbitraryMath()
{
    // 1. 128-bit signed integer addition with overflow wrapping
    FlexInt a("0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", 128, true, 16);
    FlexInt b(1, 128);
    FlexInt sum = a + b; // Clamps and wraps to negative 128-bit boundary

    std::cout << "128-bit wrapped sum: " << sum.toString(16) << "\n";

    // 2. Serialize to little-endian binary bytes for machine code emission
    std::vector<uint8_t> buffer(16);
    sum.writeBytes(buffer, Endianness::Little);

    // 3. 64-bit IEEE float manipulation and bitcasting
    FlexFloat f1("3.14159265358979323846", 64);
    FlexFloat f2("2.0", 64);
    FlexFloat product = f1 * f2;

    FlexInt rawIeeeBits = product.bitcastToFlexInt();
    std::cout << "Product raw IEEE bits: 0x" << rawIeeeBits.toString(16) << "\n";
}
```

### 3. Fast Liveness Equation Evaluation with DenseBitSet

```cpp
#include "HelperClasses/DenseBitSet.h"
#include <cassert>

void computeDataflow()
{
    DenseBitSet liveIn(256);
    DenseBitSet use(256);
    DenseBitSet liveOut(256);
    DenseBitSet def(256);

    // Variable 10 is used; variable 20 is defined; variable 30 is live-out
    use.set(10);
    def.set(20);
    liveOut.set(20);
    liveOut.set(30);

    // LiveIn = Use | (LiveOut & ~Def)
    bool changed = liveIn.computeLiveIn(use, liveOut, def);
    assert(changed == true);
    assert(liveIn.test(10) == true);  // From Use
    assert(liveIn.test(20) == false); // Killed by Def
    assert(liveIn.test(30) == true);  // Propagated from LiveOut
}
```

---

## Testing & CMake Integration

### Linking EzCore
To link `EzCore` in downstream targets:

```cmake
target_link_libraries(YourTarget PRIVATE EzCore)
target_include_directories(YourTarget PRIVATE ${EZPACKER_ROOT}/EzCore/include)
```

### Subproject Test Suite
The test fixtures for `EzCore` reside in [`EzCore/tests/`](file:///E:/Repos/EzPacker/EzCore/tests/):
- [`EzCore/tests/T_DenseBitSet.cpp`](file:///E:/Repos/EzPacker/EzCore/tests/T_DenseBitSet.cpp): Validates word indexing, bitwise unions, and liveness transfer equations.
- [`EzCore/tests/T_IntrusiveLinkedList.cpp`](file:///E:/Repos/EzPacker/EzCore/tests/T_IntrusiveLinkedList.cpp): Tests zero-allocation node insertion, deletion, bidirectional iteration, and whole/range splicing.
- [`EzCore/tests/T_FlexNumber.cpp`](file:///E:/Repos/EzPacker/EzCore/tests/T_FlexNumber.cpp): Validates arbitrary-precision integer arithmetic, LibTomMath wrapping/trapping, LibBF float precision, IEEE bitcasts, and endian serialization.
- [`EzCore/tests/T_SourceManager.cpp`](file:///E:/Repos/EzPacker/EzCore/tests/T_SourceManager.cpp): Tests file buffering, path resolution, binary-search line translation, and slice extraction.
- [`EzCore/tests/T_DiagnosticLogger.cpp`](file:///E:/Repos/EzPacker/EzCore/tests/T_DiagnosticLogger.cpp): Verifies diagnostic emission, scope committing, note appending, and ANSI caret rendering.
