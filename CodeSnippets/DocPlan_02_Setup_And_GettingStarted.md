# Plan Part 2: Setup and Getting Started Guide

## 1. Objective
Author a comprehensive, beginner-friendly yet technically rigorous **Getting Started & Setup Guide** (`docs/pages/getting_started.md`), covering environment prerequisites, build configuration options, test execution, CLI tool usage, downstream CMake project integration, and troubleshooting.

---

## 2. Deliverables & File Locations

| File | Purpose |
|:---|:---|
| [`docs/pages/getting_started.md`](file:///E:/Repos/EzPacker/docs/pages/getting_started.md) | Dedicated getting started manual integrated into Doxygen via `@page getting_started Getting Started & Setup Guide`. |

---

## 3. Detailed Technical Content Outline

### A. System Requirements & Toolchain Prerequisites
- **Compilers**: Strict C++20 standard compliance required:
  - Microsoft Visual Studio 2022 (MSVC v19.34+ / 17.4+)
  - Clang 16.0.0 or higher
  - GCC 13.1 or higher
- **Build Tools**:
  - CMake 3.27+ (required for modern generator expressions and FetchContent features)
  - Ninja 1.11+ (recommended for high-speed parallel compilation) or Visual Studio Solution generator
- **Documentation Tools (Optional)**:
  - Doxygen 1.10+ (tested with 1.16+)
  - Graphviz (`dot`) for class hierarchy diagrams
- **Vendored / Fetched Dependencies**:
  - `EzLib` (data structures and build helpers, fetched automatically via CMake FetchContent)
  - `LibBf` (Fabrice Bellard's arbitrary-precision IEEE-754 floating point library, included in-tree)
  - `LibTomMath` (arbitrary-precision integer arithmetic, vendored via CMake)
  - `Lexy` (modern C++ parser combinator for DSL processing, vendored via CMake)
  - `GoogleTest` (unit and integration testing framework)

### B. Cloning & Repository Initialization
- Command-line instructions for cloning with full history:
  ```bash
  git clone https://github.com/DaniTRDev/EzPacker.git
  cd EzPacker
  ```

### C. Build Configurations & CMake Options
- **Default Out-of-Source Build (Ninja)**:
  ```powershell
  # Windows (PowerShell)
  cmake -B cmake-build-debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
  cmake --build cmake-build-debug --config Debug -j
  ```
  ```bash
  # Linux / macOS (Bash)
  cmake -B cmake-build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
  cmake --build cmake-build-release -j$(nproc)
  ```
- **CMake Options Reference Table**:
  | Option | Default | Description |
  |:---|:---|:---|
  | `EZPACKER_BUILD_TESTS` | `ON` | Compiles the full GoogleTest test suites (93 tests). |
  | `EZPACKER_BUILD_STATIC` | `OFF` | Builds EzPacker components as static (`.lib`/`.a`) rather than shared (`.dll`/`.so`). |
  | `EZPACKER_ENABLE_SANITIZERS` | `OFF` | Enables AddressSanitizer and UndefinedBehaviorSanitizer (`/fsanitize=address` on MSVC, `-fsanitize=address,undefined` on GCC/Clang). |
  | `CMAKE_BUILD_TYPE` | `Debug` | Standard CMake build configuration (`Debug`, `Release`, `RelWithDebInfo`). |

### D. Running the Test Suite
- Executing all unit, Sema, and end-to-end compiler tests via CTest:
  ```powershell
  ctest --test-dir cmake-build-debug --output-on-failure
  ```
- Running targeted test binaries directly:
  - Middle-end IR tests: `./bin/EzMirTestSuite`
  - Lexer and Sema tests: `./bin/EzDslLexerTestSuite`, `./bin/EzDslSemaTestSuite`
  - Code generators: `./bin/EzDslCodeGeneratorsTestSuite`
  - Object emitter tests: `./bin/EzCodeEmitterTestSuite`
  - End-to-end compilation: `./bin/EzCompilerEndToEndTests`

### E. CLI Driver Tools Quickstart
- **`ezc` Compiler Executable**:
  - Compiling textual MIR to an object file:
    ```bash
    ezc input.mir -o output.o --target=x86_64-pc-windows-msvc -O2
    ```
  - Emitting human-readable intermediate MIR after passes:
    ```bash
    ezc input.mir --emit-mir -o optimized.mir -O2
    ```
  - Compiling for Linux ELF64:
    ```bash
    ezc input.mir -o output.o --target=x86_64-unknown-linux-gnu -fPIC
    ```
  - Full CLI flag reference table (`-o`, `--target`, `-O0`..`-O3`, `--emit-mir`, `--emit-obj`, `-fPIC`, `--dump-passes`, `--verbose`, `--help`).
- **`EzDslCli` DSL Processor**:
  - Generating C++ target files from DSL definitions:
    ```bash
    ezdsl -i x86_64.tdesc -o generated/
    ```

### F. Integrating EzPacker in Downstream Projects
- **Via CMake `FetchContent`**:
  ```cmake
  include(FetchContent)
  FetchContent_Declare(
      EzPacker
      GIT_REPOSITORY https://github.com/DaniTRDev/EzPacker.git
      GIT_TAG main
  )
  FetchContent_MakeAvailable(EzPacker)

  add_executable(my_compiler main.cpp)
  target_link_libraries(my_compiler PRIVATE EzCompiler EzMir EzCore)
  ```
- **Via CMake Subdirectory (`add_subdirectory`)**:
  - Structure, include paths, and linking target names:
    - `EzCore`: Utilities, arbitrary precision math, diagnostics.
    - `EzMir`: Intermediate representation and SSA analysis.
    - `EzDsl`: Declarative language toolchain.
    - `EzCodeEmitter`: Binary section and ELF/COFF writers.
    - `EzTriple`: Target triple parser and normalizer.
    - `EzCompiler`: Pipeline driver and target resolver.
    - `EzTargets::X86_64`: x86-64 concrete target implementation.

### G. Troubleshooting & Frequently Asked Questions
- MSVC C++20 module scan warnings (`CMAKE_CXX_SCAN_FOR_MODULES OFF`).
- Runtime path setup on Windows for shared `.dll` binaries in `bin/`.
- Sanitizer runtime configuration with MSVC `/fsanitize=address`.

---

## 4. Verification & Acceptance Criteria
1. The markdown page is formatted with valid Doxygen `@page getting_started` metadata and cross-referenced on the main page.
2. All command lines are verified against actual repository paths and CMake options.
3. Building Doxygen compiles the page cleanly without warnings.
