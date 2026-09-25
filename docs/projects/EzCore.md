# EzCore Subproject Documentation

[EzPacker Documentation Index](../index.md) > [Subprojects](EzCore.md) > **EzCore** | [Doxygen API Reference](../doxygen/index.html)

---

## 1. Overview & Architectural Role

`EzCore` is the foundational utility and infrastructure library of the EzPacker compiler framework. Every other non-vendored subproject in the repository—including `EzMir`, `EzDsl`, `EzCodeEmitter`, `EzTriple`, `EzCompiler`, and `EzTargets`—directly depends upon `EzCore`.

It is deliberately designed to provide machine-independent primitives, zero-overhead data structures, high-performance polymorphic memory management abstractions, a fluent and thread-safe compiler diagnostic reporting engine, arbitrary-precision numerical representations for compiler constants, and unified source coordinate management.

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
- **Compiler Diagnostic Subsystem**: Thread-safe diagnostic collection, severity classification, scoped message staging, formatted note appending, and ANSI terminal rendering.
- **Arbitrary-Precision Numbers (`FlexInt` and `FlexFloat`)**: Machine-independent constant representation capable of modeling any target integer bit-width (1 to 256+ bits) using LibTomMath and floating-point values using LibBf.
- **Compiler-Optimized Containers**: `IntrusiveLinkedList<T>` for zero-overhead instruction streams and `DenseBitSet` for fast bit-vector dataflow analysis and liveness transfer functions.
- **Source Coordinate Tracking**: Unified buffer and coordinate management translating byte offsets into file, line, and column positions via binary search over precomputed line tables.

---

## 2. Polymorphic Memory Architecture (PMR)

EzPacker leverages C++20 Polymorphic Memory Resources (`<memory_resource>`) across all data-intensive operations. Rather than binding containers to global heap allocation (`malloc`/`free`), core compiler entities accept a `std::pmr::memory_resource*` or receive monotonic arena allocators.

### Core PMR Guidelines in EzPacker
1. **Pass-Scoped Arenas**: Intensive compilation passes (e.g., `LivenessAnalysisPass`, `MirRegisterAllocatorPass`) create local monotonic buffers (`std::pmr::monotonic_buffer_resource`) on top of the parent driver allocator. When the pass completes, all intermediate graphs, interference tables, and temporary nodes are discarded simultaneously in $O(1)$ time without per-node deallocations.
2. **Session Arenas**: Module-level entities (`MirFunction`, `MirBlock`, `MirInstruction`, `MirOperand`, `SourceFileEntry`) live inside the compilation session arena managed by `DriverContext` or `MirBuilderContext`.
3. **Explicit Allocator Propagation**: Classes storing dynamically allocated state require an explicit allocator parameter in their constructor. Standard PMR typedefs from `<vector>`, `<string>`, `<unordered_map>`, and `<unordered_set>` are used throughout:
   ```cpp
   std::pmr::vector<T>
   std::pmr::string
   std::pmr::unordered_map<Key, Value>
   std::pmr::unordered_set<Key>
   ```

---

## 3. Diagnostic Engine

The EzPacker diagnostic subsystem (`EzCore/include/Diagnostics/`) provides structured, thread-safe, severity-graded diagnostic emission with line-column coordinates, source snippets, secondary notes, and color highlighting.

```
   DiagnosticCollector (Thread-Safe Sink & Scope Stack)
          |
          +--> builder(type, sender) ---> DiagnosticBuilder (Single-Thread RAII)
          |                                    |
          |                                    +--> operator<<(std::string_view)
          |                                    +--> operator<<(SourceReference*)
          |                                    +--> appendNote(...)
          |                                    +--> flush() (or on ~DiagnosticBuilder)
          v
   DiagnosticListener (e.g. DiagnosticLogger)
```

### 3.1 Severity Classification (`DiagnosticMessage.h`)

Diagnostics are categorized via the `DiagnosticMessageType` bitflag enumeration:

```cpp
enum DiagnosticMessageType : uint8_t
{
    Diag_None    = 0,
    Diag_Debug   = 1,         // Debug information emitted during development or tracing.
    Diag_Error   = (1 << 1),  // Error condition preventing compilation or analysis.
    Diag_Trace   = (1 << 2),  // Trace information during specific compiler passes.
    Diag_Warning = (1 << 3)   // Warning condition that does not halt compilation.
};
```

### 3.2 Diagnostic Records & Notes

- **`DiagnosticNote`**: A supplementary annotation attached to a diagnostic message providing additional context or secondary source spans:
  ```cpp
  struct DiagnosticNote
  {
      SourceReference *m_sourceRef{ nullptr }; // Source span this note refers to, or nullptr.
      std::pmr::string m_noteContent{};        // Arena-allocated note text.
  };
  ```
- **`DiagnosticMessage`**: The fundamental diagnostic record holding:
  - `DiagnosticMessageType getType() const`
  - `std::string_view getSender() const`: Identifier name of the emitting compiler component.
  - `std::string_view getMainMsg() const`: Primary diagnostic text.
  - `SourceReference *getPrimarySourceRef() const`: Primary location pointer.
  - `const std::list<DiagnosticNote> &getNotes() const`: List of contextual notes.

### 3.3 Diagnostic Collector (`DiagnosticCollector.h`)

`DiagnosticCollector` is the central thread-safe sink for all compiler diagnostics. It maintains a stack of active diagnostic scopes, manages registered listeners, and controls active severity bitmasks.

```cpp
class DiagnosticCollector
{
public:
    DiagnosticCollector();

    // Severity filtering
    bool isDiagEnabledForType(DiagnosticMessageType type) const;
    void enableDiag(DiagnosticMessageType type);
    void setEnabledDiags(DiagnosticMessageType types);

    // Factory methods returning single-thread builders
    DiagnosticBuilder builder(DiagnosticMessageType type, std::string_view sender);

    // Formatted convenience builders (only formats if severity is enabled)
    template <typename... Args>
    DiagnosticBuilder error(std::string_view sender, std::format_string<Args...> fmt, Args &&...args);
    DiagnosticBuilder error(std::string_view sender, std::string_view message);

    template <typename... Args>
    DiagnosticBuilder warn(std::string_view sender, std::format_string<Args...> fmt, Args &&...args);
    DiagnosticBuilder warn(std::string_view sender, std::string_view message);

    template <typename... Args>
    DiagnosticBuilder trace(std::string_view sender, std::format_string<Args...> fmt, Args &&...args);
    DiagnosticBuilder trace(std::string_view sender, std::string_view message);

    // Listener registration
    void addListener(DiagnosticListener *listener);
    void removeListener(DiagnosticListener *listener);

    // Scoped transaction handling
    void beginScope(DiagnosticScopeAction action);
    void endScope();

    // Commit a message directly
    void onDiag(DiagnosticMessage message);

    std::pmr::memory_resource *getAllocator();
};
```

### 3.4 Diagnostic Builder (`DiagnosticBuilder.h`)

`DiagnosticBuilder` provides a single-thread RAII wrapper for assembling diagnostics. It prevents duplicate error emissions by deleting copy constructors and automatically commits its accumulated message to the parent collector upon destruction (or via an explicit `.flush()` call).

```cpp
class DiagnosticBuilder
{
public:
    DiagnosticBuilder(const DiagnosticBuilder &) = delete;
    DiagnosticBuilder &operator=(const DiagnosticBuilder &) = delete;
    DiagnosticBuilder(DiagnosticBuilder &&other);
    ~DiagnosticBuilder(); // Flushes message into collector

    // Primary message building
    DiagnosticBuilder &operator<<(std::string_view str);
    DiagnosticBuilder &operator<<(SourceReference *sourceRef);
    DiagnosticBuilder &build(DiagnosticMessageType type, std::string_view sender);

    // Formatted note appending
    template <typename... Args>
    DiagnosticBuilder &appendNote(SourceReference *ref, std::format_string<Args...> fmt, Args &&...args);

    template <typename... Args>
    DiagnosticBuilder &appendNote(std::format_string<Args...> fmt, Args &&...args);

    DiagnosticBuilder &appendNote(SourceReference *sourceRef, std::string_view message);
    DiagnosticBuilder &appendNote(std::string_view message);

    void flush();
};
```

### 3.5 Usage Example: Reporting Compiler Diagnostics

```cpp
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "SourceManager/SourceManager.h"

// 1. Initialize collector and attach a logger
DiagnosticCollector collector;
DiagnosticLogger logger(std::cerr);
collector.addListener(&logger);

// Enable errors and warnings
collector.setEnabledDiags(static_cast<DiagnosticMessageType>(Diag_Error | Diag_Warning));

// 2. Obtain a source reference from SourceManager
SourceManager srcMgr(std::filesystem::current_path(), collector.getAllocator());
size_t fileId = srcMgr.addSourceContent("test.mir", "fn @main() -> i32 {\n    %0 = add.i128 %a, %b\n}\n");
SourceReference *srcRef = srcMgr.createReference(28, 16, fileId);

// 3. Emit a formatted error with an attached note
collector.error("InstructionSelector", "Cannot select target instruction for opcode '{}' with type 'i128'", "ADD")
    << srcRef
    << " [unsupported on target arch]";

// Or use the fluent builder with an attached note:
auto b = collector.builder(Diag_Error, "Legalizer");
b << "Operand width exceeds maximum scalar hardware register width" << srcRef;
b.appendNote("Target 'x86_64' requires 128-bit operations to be decomposed via NarrowScalar");
b.flush();
```

---

## 4. Arbitrary-Precision Constants: `FlexInt` & `FlexFloat`

Compiler frontends and backends must evaluate and manipulate integer and floating-point constants of arbitrary target bit-widths without host architecture overflow or undefined behavior.

### 4.1 `FlexInt` (`FlexNumber/FlexInt.h`)

`FlexInt` is a multi-precision, two's-complement integer wrapper built atop LibTomMath (`mp_int`). It provides exact signed and unsigned integer arithmetic, bit slicing, word extraction, binary serialization, and legalization splitting.

#### Constructors & State
- `explicit FlexInt(uint32_t value, size_t bitWidth = 32)`
- `explicit FlexInt(uint64_t value, size_t bitWidth = 64)`
- `explicit FlexInt(int32_t value, size_t bitWidth = 32)`
- `explicit FlexInt(int64_t value, size_t bitWidth = 64)`
- `FlexInt(std::string_view numberStr, size_t bitWidth, bool _signed, size_t radix = 10)`: Supports parsing prefixes `0x` (hex), `0b` (binary), `0o` (octal), and decimal.
- `static FlexInt fromLimbs64(std::span<const uint64_t> limbs, size_t bitWidth, bool isSigned)`

#### Arithmetic & Overflow Checking
- Operators: `+`, `+=`, `++`, `-`, `-=`, `--`, `*`, `*=`, `/`, `/=`, `%`, `%=`
- Comparisons: `>`, `>=`, `<`, `<=`, `==`, `!=`
- Status queries: `isNeg()`, `isPositive()`, `isSigned()`, `isZero()`, `getBitSize()`
- Overflow-checked operations:
  ```cpp
  bool addWithOverflow(const FlexInt &other, FlexInt &result) const;
  bool subWithOverflow(const FlexInt &other, FlexInt &result) const;
  bool mulWithOverflow(const FlexInt &other, FlexInt &result) const;
  ```

#### Bitwise, Shift & Slice Operations
- Bitwise operators: `~`, `&`, `&=`, `|`, `|=`, `^`, `^=`
- Logical and arithmetic shifts:
  ```cpp
  FlexInt shl(size_t shiftBits) const;
  FlexInt lshr(size_t shiftBits) const; // Zero-fill
  FlexInt ashr(size_t shiftBits) const; // Sign-fill
  ```
- Bit extraction & word slicing:
  ```cpp
  // Extracts an arbitrary slice of bits [startBit, startBit + numBits - 1]
  FlexInt extractBits(size_t startBit, size_t numBits, bool resultSigned = false) const;

  // Extracts the k-th 64-bit limb (wordIndex * 64 .. wordIndex * 64 + 63)
  uint64_t extractWord64(size_t wordIndex) const;
  ```

#### Legalization & Splitting Helpers
- `FlexInt getHighHalf() const`: Splits this integer and extracts the upper $N/2$ bits.
- `FlexInt getLowHalf() const`: Splits this integer and extracts the lower $N/2$ bits.
- `void extend(size_t newBitSize, bool isSigned)`: Extends or truncates the integer to a new bit size.
- `bool clampToTwosComplement(OverflowPolicy policy = OverflowPolicy::Wrap)`: Clamps value to fit strictly within $2^N$ two's-complement bounds. `OverflowPolicy::Wrap` silently wraps modulo $2^N$; `OverflowPolicy::Trap` throws `std::overflow_error`.

#### Serialization & Conversions
- `int64_t getI64() const`
- `uint64_t getU64() const`
- `std::string toString(size_t radix = 10) const`
- `void writeBytes(std::span<uint8_t> dest, Endianness endian = Endianness::Little) const`
- `static FlexInt readBytes(std::span<const uint8_t> src, size_t bitWidth, bool isSigned, Endianness endian = Endianness::Little)`

#### Example: `FlexInt` in Compiler Constant Folding
```cpp
#include "FlexNumber/FlexInt.h"

// 64-bit signed integer
FlexInt valA(0x7FFFFFFFFFFFFFFFLL, 64);
FlexInt valB(1LL, 64);

FlexInt result(0, 64);
bool didOverflow = valA.addWithOverflow(valB, result);
// didOverflow == true, result wraps around to 0x8000000000000000 (-9223372036854775808)

// Splitting 128-bit integers during NarrowScalar legalization
FlexInt bigInt("0x123456789ABCDEF00FEDCBA987654321", 128, false, 16);
FlexInt high64 = bigInt.getHighHalf(); // 0x123456789ABCDEF0
FlexInt low64 = bigInt.getLowHalf();   // 0x0FEDCBA987654321
```

### 4.2 `FlexFloat` (`FlexNumber/FlexFloat.h`)
`FlexFloat` wraps LibBf (`bf_t`) to provide arbitrary-precision floating-point arithmetic compliant with IEEE 754 standards. It natively models 32-bit single precision (`f32`), 64-bit double precision (`f64`), and 128-bit quadruple precision (`f128`).

---

## 5. High-Performance Compiler Containers

### 5.1 `IntrusiveLinkedList<T>` (`HelperClasses/IntrusiveLinkedList.h`)

Standard containers like `std::list` allocate a heap node header for every element inserted. EzPacker's `IntrusiveLinkedList<T>` embeds the doubly-linked list pointers (`m_prev`, `m_next`) directly within the element itself, resulting in:
- **Zero dynamic memory allocations** on insertion or removal.
- **Cache-locality advantages** when iterating through instruction streams.
- **$O(1)$ splicing and reordering** of basic blocks and instructions.

#### Concept Requirements on Element Type `T`
Any class stored in `IntrusiveLinkedList<T>` (such as `MirInstruction`, `MirBlock`, or `MirFunction`) must provide:
```cpp
T* getPrev() const noexcept;
void setPrev(T* prev) noexcept;
T* getNext() const noexcept;
void setNext(T* next) noexcept;
```

#### API Methods
```cpp
template <typename T>
class IntrusiveLinkedList
{
public:
    class iterator; // Bidirectional iterator (operator*, operator->, ++, --, ==, !=)

    bool empty() const;
    size_t size() const;
    T *front() const;
    T *back() const;

    void push_back(T *node);
    void push_front(T *node);
    iterator insert(iterator pos, T *node);
    iterator erase(iterator pos);
    void clear();

    iterator begin();
    iterator end();
};
```

### 5.2 `DenseBitSet` (`HelperClasses/DenseBitSet.h`)

`DenseBitSet` is a compact, 64-bit word-aligned bit vector optimized specifically for compiler dataflow analysis and register liveness computations. It avoids per-bit branch overhead by processing 64 bits at a time via native CPU 64-bit integer operations.

```cpp
class DenseBitSet
{
public:
    DenseBitSet() = default;
    explicit DenseBitSet(size_t numBits);

    // Bit manipulation
    void set(size_t bit);
    bool test(size_t bit) const;

    // Bitwise union: this |= other
    // Returns true if this bitset changed value (new bits were set)
    bool unionWith(const DenseBitSet &other);

    // Evaluates compiler liveness transfer equation:
    // LiveIn = Use | (LiveOut & ~Def)
    // Updates internal words in-place and returns true if any word changed value.
    bool computeLiveIn(const DenseBitSet &use, const DenseBitSet &liveOut, const DenseBitSet &def);

private:
    std::vector<uint64_t> m_words;
};
```

#### Transfer Function Evaluation in Liveness Analysis
In iterative backward dataflow analysis, evaluating whether any block's `LiveIn` set changed is the termination condition. `computeLiveIn` performs:
```text
LiveIn = Use ∪ (LiveOut \ Def)
```
over raw 64-bit words using bitwise instructions (`word_use | (word_liveOut & ~word_def)`), returning `true` immediately if any word changes, allowing the outer fixed-point loop to detect convergence with minimal overhead.

---

## 6. Source Coordinate & File Tracking

The source management subsystem (`EzCore/include/SourceManager/`) translates raw source byte streams into file names, line numbers, and column offsets without duplicating buffer contents.

```
       SourceManager (GenericSourceManager)
             |
             +-- addSourceContent(name, content) / loadFile(path)
             |          |
             |          v
             |     SourceFileEntry
             |      +-- m_content (Arena-backed string)
             |      +-- m_name
             |      +-- m_lines (std::pmr::vector<SourceLineRange>, sorted)
             |
             +-- createReference(start, len, sourceId) ---> SourceReference
             |
             +-- getReferenceLine(ref) -------------------> SourceLineRange (via binary search)
             +-- getRawLineContent(ref) ------------------> std::string_view
             +-- getReferenceContent(ref) ----------------> std::string_view
```

### 6.1 Descriptors

- **`SourceReference`**: A lightweight 24-byte span descriptor attached to AST nodes, MIR instructions, and diagnostics:
  ```cpp
  struct SourceReference
  {
      size_t m_beginOffset{ 0 };  // Byte offset of first character of span
      size_t m_endOffset{ 0 };    // Byte offset one past last character
      size_t m_sourceFileId{ 0 }; // 1-based numeric ID of source file

      size_t length() const { return m_endOffset - m_beginOffset; }
  };
  ```
- **`SourceLineRange`**: Precomputed line bounds stored inside each source entry:
  ```cpp
  struct SourceLineRange
  {
      size_t m_beginOffset{ 0 }; // Byte offset where line begins
      size_t m_endOffset{ 0 };   // Byte offset past line content (excluding terminator)
      size_t m_lineNumber{ 0 };  // 1-based line number

      size_t length() const { return m_endOffset - m_beginOffset; }
  };
  ```
- **`SourceFileEntry`**: Arena-allocated file descriptor:
  ```cpp
  struct SourceFileEntry
  {
      std::pmr::string m_content;
      std::pmr::string m_name;
      std::pmr::vector<SourceLineRange> m_lines; // Sorted by m_beginOffset for binary search
  };
  ```

### 6.2 `SourceManager` API

```cpp
class SourceManager : public GenericSourceManager
{
public:
    SourceManager(const std::filesystem::path &workingPath, std::pmr::memory_resource *alloc);
    ~SourceManager();

    // Ingestion
    size_t addSourceContent(const std::string &name, const std::string_view &content) override;
    std::optional<size_t> loadFile(const std::filesystem::path &filePath,
                                   const std::optional<std::filesystem::path> &relativeTo = std::nullopt) override;

    // Span creation
    SourceReference *createReference(size_t startOffset, size_t length, size_t sourceId) override;
    SourceReference *createReference(size_t startOffset, size_t length, const std::string_view &sourceFile) override;

    // Location resolution
    const SourceLineRange *getReferenceLine(SourceReference *ref) const override;
    std::string_view getRawLineContent(SourceReference *ref) const override;
    std::string_view getReferenceContent(SourceReference *ref) const override;
    std::string_view getSourceContent(size_t id) const override;

    // Include path resolution
    void addIncludePath(const std::filesystem::path &path) override;
    std::filesystem::path resolveSourcePath(const std::filesystem::path &sourceFile,
                                           const std::optional<std::filesystem::path> &relativeTo = std::nullopt) const override;
};
```

---

## 7. Header & Class Index

| Component | Header Location | Key Classes / Structs |
|---|---|---|
| Diagnostics | `EzCore/include/Diagnostics/DiagnosticMessage.h` | `DiagnosticMessageType`, `DiagnosticNote`, `DiagnosticMessage` |
| Diagnostics | `EzCore/include/Diagnostics/DiagnosticCollector.h` | `DiagnosticCollector` |
| Diagnostics | `EzCore/include/Diagnostics/DiagnosticBuilder.h` | `DiagnosticBuilder` |
| Diagnostics | `EzCore/include/Diagnostics/DiagnosticListener.h` | `DiagnosticListener` |
| Diagnostics | `EzCore/include/Diagnostics/DiagnosticLogger.h` | `DiagnosticLogger` |
| Diagnostics | `EzCore/include/Diagnostics/DiagnosticScope.h` | `DiagnosticScope`, `DiagnosticScopeAction` |
| Multi-Precision | `EzCore/include/FlexNumber/FlexInt.h` | `FlexInt`, `OverflowPolicy`, `Endianness` |
| Multi-Precision | `EzCore/include/FlexNumber/FlexFloat.h` | `FlexFloat` |
| Containers | `EzCore/include/HelperClasses/IntrusiveLinkedList.h` | `IntrusiveLinkedList<T>`, `iterator` |
| Containers | `EzCore/include/HelperClasses/DenseBitSet.h` | `DenseBitSet` |
| Source Coordinates | `EzCore/include/SourceManager/GenericSourceManager.h` | `SourceReference`, `SourceLineRange`, `SourceFileEntry`, `GenericSourceManager` |
| Source Coordinates | `EzCore/include/SourceManager/SourceManager.h` | `SourceManager` |
