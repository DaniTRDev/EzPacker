# EzCore Subproject Documentation

[EzPacker Documentation Index](../index.md) > **EzCore**

---

## 1. Overview & Architectural Role

`EzCore` is the foundational utility and infrastructure library for EzPacker. Every other non-vendored subproject (`EzMir`, `EzDsl`, `EzCodeEmitter`, `EzTriple`, `EzCompiler`, `EzTargets`) depends on `EzCore`. It provides standardized primitives, polymorphic memory management abstractions, a fluent compiler diagnostic reporting engine, arbitrary-precision numerical representations for compiler constants, specialized compiler data structures, and source coordinate mapping.

```
       +-------------------------------------------------------------+
       |                         EzCompiler                          |
       +-------------------------------------------------------------+
         |              |                |            |            |
         v              v                v            v            v
     EzTargets      EzTriple       EzCodeEmitter    EzMir        EzDsl
         \              \                /            /            /
          +---------------+------------+-------------+------------+
                          |            |             |
                          v            v             v
                     +-----------------------------------+
                     |              EzCore               |
                     +-----------------------------------+
                     | PMR Allocators | Diagnostics      |
                     | FlexInt/Float  | DenseBitSet      |
                     | SourceManager  | IntrusiveList    |
                     +-----------------------------------+
```

### Key Responsibilities
- **Polymorphic Memory Resource (PMR) Integration**: Standardized allocation patterns using `std::pmr::memory_resource` to minimize heap churn during compilation passes.
- **Compiler Diagnostic Subsystem**: Structured, thread-safe, severity-graded diagnostic emission with line-column coordinates, source snippets, and ANSI color highlighting.
- **Arbitrary-Precision Numbers (`FlexInt` and `FlexFloat`)**: Machine-independent constant representation capable of modeling any target integer width (1 to 256+ bits) and IEEE 754 floating-point format (f32, f64, f128).
- **Compiler-Optimized Containers**: Intrusive linked lists for zero-overhead instruction streams and dense bit sets for bit-vector dataflow analysis.
- **Source Coordinate Tracking**: Unified buffer and coordinate management translating byte offsets into file, line, and column positions.

---

## 2. Polymorphic Memory Architecture (PMR)

EzPacker leverages C++20 Polymorphic Memory Resources (`<memory_resource>`) across all data-intensive operations. Rather than binding containers to global `malloc`/`free`, core compiler entities take a `std::pmr::memory_resource*` or receive monotonic arena allocators.

### Core PMR Guidelines in EzPacker
1. **Pass-Scoped Arenas**: Intensive compilation passes (e.g. `MirRegisterAllocatorPass`, `CodeFlowAnalysisPass`) create local monotonic buffers (`std::pmr::monotonic_buffer_resource`) on top of the parent driver allocator. When the pass completes, all intermediate graphs, interference tables, and temporary nodes are discarded simultaneously in $O(1)$ without per-node deallocations.
2. **Standard Typedefs**: Container aliases in `EzCommonStd.h` enforce PMR semantics:
   ```cpp
   namespace std::pmr {
       using string;
       using vector;
       using unordered_map;
       using unordered_set;
       using set;
   }
   ```
3. **Explicit Allocator Propagation**: Classes storing dynamically allocated state require an explicit allocator parameter in their constructor.

---

## 3. Diagnostic Engine

EzPacker includes a modern, fluent compiler diagnostics system defined in `EzCore/include/Diagnostics/`.

```
  DiagnosticCollector
         |
         +--> builder(type, sender) ---> DiagnosticBuilder
         |                                    |
         |                                    +--> .at(loc)
         |                                    +--> .message(...)
         |                                    +--> .note(...)
         |                                    +--> .emit()
         v
  DiagnosticListener (e.g., DiagnosticLogger)
```

### 3.1 Diagnostic Message Types & Locations
- **`DiagnosticMessageType`** (`DiagnosticMessage.h`):
  - `Diag_Trace`: Granular compiler debugging output.
  - `Diag_Info`: Informational status updates (e.g., pass timings).
  - `Diag_Warning`: Non-fatal issues (e.g., unreachable basic blocks, deprecated syntax).
  - `Diag_Error`: Compilation errors halting code generation.
  - `Diag_Fatal`: Unrecoverable compiler crashes or internal assertion violations.
- **`DiagnosticSourceLocation`** (`DiagnosticMessage.h`):
  - Encapsulates file name, byte offset, line number, column number, and length span.

### 3.2 Key Classes
- **`DiagnosticCollector`** (`DiagnosticCollector.h`):
  Central thread-safe sink for all diagnostics. Maintains active severity filters, counts total errors/warnings, and dispatches messages to registered listeners.
- **`DiagnosticBuilder`** (`DiagnosticBuilder.h`):
  RAII-capable fluent builder for assembling diagnostics. Supports message formatting via `std::format`, attaching primary and secondary source spans, and appending explanatory notes.
- **`DiagnosticLogger`** (`DiagnosticLogger.h`):
  Implements `DiagnosticListener` to print compiler diagnostics to `stderr` or a log file, complete with ANSI color highlighting and source code context previews.
- **`DiagnosticScope`** (`DiagnosticScope.h`):
  RAII helper pushing contextual tags (e.g. current function, current pass) onto the active diagnostic context.

### 3.3 Example: Emitting a Diagnostic
```cpp
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"

DiagnosticCollector collector;
DiagnosticLogger logger(std::cerr);
collector.addListener(&logger);

DiagnosticSourceLocation loc{
    .m_file = "arithmetic.mir",
    .m_line = 12,
    .m_col = 5,
    .m_length = 4
};

// Fluent diagnostic creation
collector.builder(DiagnosticMessageType::Diag_Error, "InstructionSelector")
    .at(loc)
    .message("Cannot select target instruction for opcode '{}' with type 'i128'", "ADD")
    .note("Hardware target 'x86_64' requires 128-bit additions to be legalized into ADD64 + ADC64")
    .emit();

if (collector.hasErrors()) {
    std::cerr << "Compilation failed with " << collector.getErrorCount() << " errors.\n";
}
```

---

## 4. Arbitrary-Precision Constants: `FlexInt` & `FlexFloat`

Compiler backends must compute and manipulate integer and floating-point constants of arbitrary target bit-widths without host architecture overflow or undefined behavior.

### 4.1 `FlexInt` (`FlexNumber/FlexInt.h`)
`FlexInt` wraps `libtommath` (`mp_int`) to provide arbitrary-precision, two's-complement integer arithmetic with explicit bit-width tracking and sign-awareness.

- **Arbitrary Bit Widths**: Models types from `i1` (booleans) up to `i256` or higher.
- **Arithmetic Operations**: Full support for `+`, `-`, `*`, `/`, `%` (signed and unsigned).
- **Bitwise Operations**: `&`, `|`, `^`, `~`, `<<`, `>>` (arithmetic and logical shifts).
- **Sign Extension & Truncation**:
  - `signExtend(targetBits)`: Extends the sign bit to larger bit widths.
  - `zeroExtend(targetBits)`: Zero-extends to larger bit widths.
  - `truncate(targetBits)`: Truncates high bits.
- **Host Conversions**: Safe extraction to `int64_t`, `uint64_t`, `int32_t`, `uint32_t` with bounds validation.
- **String Formatting**: Formats to decimal, hexadecimal (`0x...`), and binary representations.

```cpp
#include "FlexNumber/FlexInt.h"

FlexInt a(0x7FFFFFFF, 32, true); // 32-bit signed max
FlexInt b(1, 32, true);
FlexInt sum = a + b; // Does not overflow host types

FlexInt mask = FlexInt::allOnes(64);
FlexInt shifted = mask.logicalShiftRight(16);
```

### 4.2 `FlexFloat` (`FlexNumber/FlexFloat.h`)
`FlexFloat` wraps Bellard's `LibBf` (`bf_t`) to provide IEEE 754-compliant floating-point arithmetic across any target precision:
- `f32`: Single precision (24-bit significand, 8-bit exponent).
- `f64`: Double precision (53-bit significand, 11-bit exponent).
- `f128`: Quadruple precision (113-bit significand, 15-bit exponent).
- IEEE 754 rounding modes: Nearest-even, toward zero, toward +infinity, toward -infinity.
- Detection and creation of infinities, quiet NaNs, signaling NaNs, and signed zeros.

---

## 5. Specialized Compiler Data Structures

### 5.1 `DenseBitSet` (`HelperClasses/DenseBitSet.h`)
A dense, word-aligned bit vector tailored for fast iterative dataflow analysis passes (such as liveness analysis, reaching definitions, and available expressions).

- **Operations**:
  - `setBit(size_t index)` / `clearBit(size_t index)` / `testBit(size_t index)`
  - In-place bitwise operations: `unionWith(other)`, `intersectWith(other)`, `differenceWith(other)`
  - Comparison: `isSubsetOf(other)`, equality check `==`
  - Population count: `count()`
  - Fast iteration over set bits: `forEachSetBit(Callback&& cb)` skips 64-bit zero words entirely.

```cpp
#include "HelperClasses/DenseBitSet.h"

DenseBitSet liveIn(128); // 128 virtual registers
DenseBitSet gen(128);
DenseBitSet kill(128);

gen.setBit(4);
gen.setBit(12);
kill.setBit(4);

liveIn.differenceWith(kill); // liveIn = liveIn \ kill
liveIn.unionWith(gen);        // liveIn = liveIn U gen
```

### 5.2 `IntrusiveLinkedList<T>` (`HelperClasses/IntrusiveLinkedList.h`)
An intrusive doubly-linked list template where the previous and next pointers reside directly inside the node object (`MirInstruction`).

- **Zero Allocation Overhead**: Inserting or removing instructions in basic blocks requires zero heap allocation.
- **$O(1)$ Splicing & Iteration**: Basic block instruction splitting, hoisting, and reordering are executed in constant time.
- **Iterator Stability**: Inserting before or after an iterator does not invalidate existing iterators.

---

## 6. Source Management (`SourceManager`)

Defined in `SourceManager/SourceManager.h` and `GenericSourceManager.h`, the source manager abstracts loading and inspecting source files and textual MIR:
- Loads source files into contiguous memory buffers.
- Maintains line-start offset tables for O(log N) binary-search coordinate translation from linear byte offsets to `(line, column)`.
- Extracts source line slices for compiler error messages with caret indicators.

---

## 7. Additional Utilities

- **`CliExitCode.h`**: Enumerates standard process exit codes (`Success = 0`, `InvalidArgs = 1`, `CompilationFailed = 2`, `InternalError = 3`).
- **`NameRegistry.h`**: Fast string intern pool and unique identifier dispenser for temporary symbols and basic block labels.
- **`StringUtils.h`**: String manipulation functions (trim, split, join, case conversion, integer parsing).
- **`EzCommonStd.h`**: Common standard library includes and PMR type definitions.

---

## 8. API Reference & Further Reading

- Generated Doxygen API documentation: [Doxygen Documentation Index](../doxygen/index.html)
- Next subproject: [EzMir Subproject Documentation](EzMir.md)
- Return to [EzPacker Landing Page](../index.md)
