# EzCore

`EzCore` is the foundational utility and infrastructure library for the **EzPacker** compiler toolchain. It provides robust, high-performance, and modular subsystems essential for building compilers and code generators:

1. **Diagnostic Management System**: Thread-safe, scoped diagnostic collection, hierarchical error bubbling/discarding, note attachments, and flexible listener sinks (e.g., console/file logging).
2. **FlexNumber Arbitrary-Precision Math**: Arbitrary-bitwidth integer (`FlexInt`) and floating-point (`FlexFloat`) arithmetic wrappers powered by **LibTomMath** and **LibBF**, with strict IEEE-754 and two's-complement bounds checking.
3. **Source Management & Symbol Tracking**: High-efficiency source file registry (`SourceManager`), zero-copy string slicing, fast binary-searched line mapping, and source reference creation for AST and IR nodes.
4. **String & Memory Utilities**: Polymorphic Memory Resource (`std::pmr`) allocations and fast string manipulation routines.

---

## Architecture Overview

```
                          ┌────────────────────────┐
                          │     Compiler Phases    │
                          │ (Lexer, Sema, IR, ISel)│
                          └───────────┬────────────┘
                                      │
              ┌───────────────────────┼───────────────────────┐
              ▼                       ▼                       ▼
   ┌────────────────────┐  ┌────────────────────┐  ┌────────────────────┐
   │    Diagnostics     │  │    FlexNumber      │  │   SourceManager    │
   │  - Collector       │  │  - FlexInt         │  │  - SourceManager   │
   │  - Builder         │  │    (LibTomMath)    │  │  - SourceReference │
   │  - Message/Scope   │  │  - FlexFloat       │  │  - Line Mapping    │
   │  - Logger Sink     │  │    (LibBF)         │  │  - Include Resolver│
   └────────────────────┘  └────────────────────┘  └────────────────────┘
              │                       │                       │
              └───────────────────────┴───────────────────────┘
                                      │
                                      ▼
                        ┌───────────────────────────┐
                        │   std::pmr Memory Pools   │
                        └───────────────────────────┘
```

---

## Subsystems

### 1. Diagnostics System (`EzCore/include/Diagnostics/`)

The Diagnostics system enables compiler passes to emit structured errors, warnings, and trace messages without coupling components directly to standard I/O or loggers.

- **`DiagnosticCollector`**: Central thread-safe hub. Manages active diagnostic levels, listeners, and a stack of `DiagnosticScope` objects.
- **`DiagnosticBuilder`**: Fluent, RAII-based message builder (`operator<<`, `appendNote()`). Automatically flushes upon destruction. Inactive diagnostic levels short-circuit immediately with zero string allocations.
- **`DiagnosticScope` & `DiagnosticScopeAction`**:
  - `Commit`: Flushes collected messages to listeners upon scope completion.
  - `Discard`: Drops all messages collected within the scope (useful for speculative parsing or backtracking).
  - `Propagate`: Bubbles up messages to the parent diagnostic scope.
- **`DiagnosticMessage` & `DiagnosticNote`**: Immutable records containing the message severity, sender component, primary `SourceReference`, and attached notes.
- **`DiagnosticLogger`**: Implements `DiagnosticListener` to format and route compiler diagnostics through `EzLogger` with line highlights.

#### Diagnostic Example
```cpp
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "SourceManager/SourceManager.h"

// 1. Setup SourceManager & Collector
std::pmr::synchronized_pool_resource pool;
SourceManager sourceMgr(std::filesystem::current_path(), &pool);
DiagnosticCollector diagCollector;

// 2. Attach Logger Listener
DiagnosticLogger diagLogger(&sourceMgr);
diagCollector.addListener(&diagLogger);

// 3. Emit Scoped Diagnostics
diagCollector.beginScope(DiagnosticScopeAction::Commit);
{
    SourceReference *ref = sourceMgr.createReference(0, 10, 1);
    diagCollector.error("FrontendCompiler", "Type mismatch: expected 'i32', got 'f64'")
        << ref;
    
    // Add explanatory notes
    diagCollector.builder(Diag_Warning, "SemanticPass")
        .appendNote(ref, "Implicit conversion might lose precision");
}
diagCollector.endScope();
```

---

### 2. FlexNumber Arbitrary-Precision Math (`EzCore/include/FlexNumber/`)

Compilers require compile-time constant evaluation across arbitrary integer bitwidths (e.g. `i1` through `i128+`) and float precisions without host architecture limitations.

- **`FlexInt`**:
  - Encapsulates arbitrary-width integers using LibTomMath (`mp_int`).
  - Supports configurable signedness and arbitrary target bit widths.
  - Strict two's-complement clamping (`clampToTwosComplement()`) with overflow and underflow detection.
  - Arithmetic and bitwise operations (`+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`).
  - Native serialization/deserialization with target endianness support (`dump()`).
- **`FlexFloat`**:
  - Encapsulates IEEE-754 compliant arbitrary-precision floating point numbers using LibBF (`bf_t`).
  - Configurable precision (e.g. 16-bit half, 32-bit single, 64-bit double, 128-bit quad).
  - Precision clamping and rounding modes (`BF_RNDN`).
  - Endianness-aware binary dumping into raw byte vectors (`dump()`).

#### FlexNumber Example
```cpp
#include "FlexNumber/FlexInt.h"
#include "FlexNumber/FlexFloat.h"

// Arbitrary-width integers (e.g., 64-bit signed integer)
FlexInt a(1000000000000ULL, 64);
FlexInt b("0xDEADBEEFCAFE", 64, false, 16);
FlexInt sum = a + b;

// Binary layout serialization for code emission
std::pmr::vector<uint8_t> bytes = sum.dump(/*bigEndian=*/false);

// Arbitrary-precision floats (e.g., 64-bit double)
FlexFloat f1("3.14159265358979323846", 64);
FlexFloat f2(2.718281828459045);
FlexFloat fProd = f1 * f2;
```

---

### 3. Source Manager & Location Tracking (`EzCore/include/SourceManager/`)

The source management subsystem provides memory-efficient source tracking, include path resolution, and debug reference translation.

- **`GenericSourceManager`**: Abstract interface decoupling source tracking from concrete storage schemes.
- **`SourceManager`**:
  - PMR-allocated source file buffer registry.
  - O(1) file access by dense numeric ID or canonical path.
  - Precomputes 1-based line bounds for fast binary-searched line lookup.
  - Resolves include directives relative to working directories and search paths (`resolveSourcePath()`, `loadFile()`).
  - Produces lightweight `SourceReference` structures (`startOffset`, `endOffset`, `sourceFileId`) that embed into AST and MIR nodes.

#### SourceManager Example
```cpp
#include "SourceManager/SourceManager.h"

std::pmr::monotonic_buffer_resource arena;
SourceManager sm(std::filesystem::current_path(), &arena);

sm.addIncludePath("/usr/local/include");
sm.addIncludePath("./include");

// Add in-memory code or load from disk
size_t fileId = sm.addSourceContent("main.ez", "func main(): i32 {\n    return 42;\n}");

// Create reference for token 'return 42'
SourceReference *ref = sm.createReference(23, 9, fileId);

// Query line and slice content
SourceLineRange *line = sm.getReferenceLine(ref);
std::string_view rawLine = sm.getRawLineContent(ref);
std::string_view refText = sm.getReferenceContent(ref);
```

---

## Memory Architecture

`EzCore` uses `std::pmr` (Polymorphic Memory Resources) across all subsystems:
- **Zero-allocation hot paths**: builders and diagnostics reuse local pool resources.
- **Lifetime guarantees**: source files and diagnostics live inside dedicated compilation-scoped arenas.
- **Thread isolation**: `DiagnosticCollector` provides thread synchronization over global listeners while allowing lock-free builder usage within threads.

---

## Building and Linking

`EzCore` is built as a static CMake library.

```cmake
target_link_libraries(YourTarget PRIVATE EzCore)
target_include_directories(YourTarget PRIVATE ${EZPACKER_ROOT}/EzCore/include)
```
