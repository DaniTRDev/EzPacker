# Getting Started & Setup Guide {#getting_started}

This guide provides step-by-step instructions to configure, build, test, and integrate the **EzPacker** compiler toolchain into your environment.

---

## 🛠️ System Requirements & Prerequisites

EzPacker is engineered in modern **C++20** with strict adherence to ISO standards and zero-warning compilation.

### Supported Compilers
| Compiler Family | Minimum Version | Recommended Version | Target Platforms |
|:---|:---|:---|:---|
| **Microsoft Visual C++ (MSVC)** | VS 2022 v19.34 (17.4) | VS 2022 v19.38+ | Windows x64 |
| **LLVM Clang** | Clang 16.0.0 | Clang 18.0.0+ | Linux x86_64, Windows (clang-cl) |
| **GNU Compiler Collection (GCC)** | GCC 13.1.0 | GCC 13.2.0+ | Linux x86_64 |

### Build Tools
- **CMake**: Version **3.27** or higher is required.
- **Build System**: **Ninja** (recommended for parallel builds) or Visual Studio Solution generator.
- **Git**: For version control and dependency retrieval via `FetchContent`.
- **Doxygen (Optional)**: Version 1.10+ (tested with 1.16+) if building documentation locally.
- **Graphviz Dot (Optional)**: For generating graphical class hierarchy and collaboration diagrams.

### External & In-Tree Dependencies
EzPacker automates dependency resolution via CMake `FetchContent` or vendored sources:
- **`EzLib`**: Utility containers and build scaffolding (retrieved automatically via Git).
- **`LibBf`**: Fabrice Bellard's arbitrary-precision IEEE-754 floating point library (included in-tree).
- **`LibTomMath`**: Multi-precision integer arithmetic engine for `FlexInt` (vendored).
- **`Lexy`**: Modern C++ parser combinator library used by EzDsl (vendored).
- **`GoogleTest`**: Automated unit and end-to-end integration test runner.

---

## 📥 Cloning the Repository

Clone the repository recursively to ensure any submodules and nested resources are present:

```bash
git clone https://github.com/DaniTRDev/EzPacker.git
cd EzPacker
```

---

## ⚙️ Building EzPacker

EzPacker strongly recommends out-of-source builds using **Ninja**.

### Windows (PowerShell with MSVC / Ninja)

Open the **x64 Native Tools Command Prompt for VS 2022** (or PowerShell with MSVC tools loaded on `PATH`):

```powershell
# 1. Configure the build directory (Debug mode with tests enabled)
cmake -B cmake-build-debug -G "Ninja" `
      -DCMAKE_BUILD_TYPE=Debug `
      -DEZPACKER_BUILD_TESTS=ON

# 2. Compile the complete project across all cores
cmake --build cmake-build-debug --config Debug -j
```

For an optimized **Release** build:

```powershell
cmake -B cmake-build-release -G "Ninja" `
      -DCMAKE_BUILD_TYPE=Release `
      -DEZPACKER_BUILD_TESTS=ON

cmake --build cmake-build-release --config Release -j
```

### Linux / macOS (Bash with GCC or Clang)

```bash
# 1. Configure the build directory
cmake -B cmake-build-debug -G "Ninja" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DEZPACKER_BUILD_TESTS=ON

# 2. Build in parallel
cmake --build cmake-build-debug -j$(nproc)
```

---

## 🎛️ CMake Configuration Options

The following CMake options are exposed to configure the compilation process:

| Option | Default | Description |
|:---|:---|:---|
| `EZPACKER_BUILD_TESTS` | `ON` | Compiles the full GoogleTest test suites (93 tests). |
| `EZPACKER_BUILD_STATIC` | `OFF` | Builds EzPacker components as static libraries (`.lib`/`.a`) instead of shared (`.dll`/`.so`). |
| `EZPACKER_ENABLE_SANITIZERS` | `OFF` | Enables AddressSanitizer and UndefinedBehaviorSanitizer (`/fsanitize=address` on MSVC, `-fsanitize=address,undefined` on GCC/Clang). |
| `CMAKE_BUILD_TYPE` | `Debug` | Specifies the build configuration (`Debug`, `Release`, `RelWithDebInfo`). |
| `CMAKE_CXX_SCAN_FOR_MODULES` | `OFF` | Disables CMake C++20 module dependency scanning to accelerate build times. |

To enable AddressSanitizer:

```powershell
cmake -B cmake-build-asan -G "Ninja" -DEZPACKER_ENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-asan -j
```

---

## 🧪 Running the Test Suite

EzPacker includes 93 comprehensive automated tests covering intermediate representations, parser combinators, semantic validation, code generation, relocation resolving, and end-to-end compilation.

### Run All Tests via CTest

```powershell
ctest --test-dir cmake-build-debug --output-on-failure
```

Expected output:
```
100% tests passed, 0 tests failed out of 93
Total Test time (real) = ~3.20 sec
```

### Targeted Test Suite Binaries

Individual test suites can be executed directly from `cmake-build-debug/bin/`:

```powershell
# Middle-end IR, SSA, and dominator tree tests
./cmake-build-debug/bin/EzMirTestSuite

# DSL Lexer and Grammar tests
./cmake-build-debug/bin/EzDslLexerTestSuite

# DSL Semantic analysis and symbol table tests
./cmake-build-debug/bin/EzDslSemaTestSuite

# DSL C++ code generator tests
./cmake-build-debug/bin/EzDslCodeGeneratorsTestSuite

# Object emitter and binary writer tests
./cmake-build-debug/bin/EzCodeEmitterTestSuite

# Full compiler driver and end-to-end tests
./cmake-build-debug/bin/EzCompilerTestSuite
./cmake-build-debug/bin/EzCompilerEndToEndTests
```

---

## 🖥️ Command-Line Driver Tools

EzPacker installs two primary executable tools into `bin/`:

### 1. `ezc` (Compiler Driver)
The standalone compiler driver executable translates `.mir` textual intermediate files into native object files.

```bash
# Basic compilation to a Windows x64 COFF object
ezc input.mir -o output.obj --target=x86_64-pc-windows-msvc -O2

# Compilation to a Linux x86_64 ELF object with Position Independent Code
ezc input.mir -o output.o --target=x86_64-unknown-linux-gnu -fPIC -O3

# Emitting human-readable optimized MIR after Middle-End passes
ezc input.mir --emit-mir -o optimized.mir -O2

# Displaying pipeline pass execution trace
ezc input.mir -o output.o --dump-passes
```

#### CLI Flag Reference
| Flag | Description |
|:---|:---|
| `-o <file>` | Path to the output binary (`.o`/`.obj`) or text file (`.mir`). |
| `--target=<triple>` | Target architecture triple (e.g. `x86_64-pc-windows-msvc`, `x86_64-unknown-linux-gnu`). |
| `-O0`, `-O1`, `-O2`, `-O3` | Optimization level applied to middle-end and legalization passes. |
| `--emit-mir` | Halts after middle-end passes and dumps textual MIR instead of machine code. |
| `--emit-obj` | Emits binary relocatable object file (default). |
| `-fPIC`, `--pic` | Generates Position-Independent Code for shared libraries. |
| `--reloc-model=<model>` | Relocation model (`static`, `pic`, `dynamic-no-pic`). |
| `--dump-passes` | Prints the sequence of executed compiler passes to `stderr`. |
| `--verbose`, `-v` | Enables verbose diagnostic output. |
| `--help`, `-h` | Prints available command-line flags. |

---

### 2. `EzDslCli` (Architecture DSL Processor)
The declarative architecture processor parses `.tdesc`, `.idf`, `.ezcc`, `.lad`, `.lrd`, and `.isf` files and generates C++ tables and selector classes.

```bash
# Process a target descriptor and emit C++ headers to generated/
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64.tdesc -o generated/
```

---

## 🔗 Consuming EzPacker in Downstream Projects

You can incorporate EzPacker into downstream projects using modern CMake.

### Option A: Via CMake `FetchContent` (Recommended)

Add the following snippet to your downstream project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.27)
project(MyLanguageCompiler CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    EzPacker
    GIT_REPOSITORY https://github.com/DaniTRDev/EzPacker.git
    GIT_TAG main
)
FetchContent_MakeAvailable(EzPacker)

add_executable(my_compiler src/main.cpp)

# Link against the EzPacker compiler library and IR engine
target_link_libraries(my_compiler PRIVATE
    EzCompiler
    EzMir
    EzCore
    EzTargetsX86_64Registration
)
```

### Option B: Via `add_subdirectory`

If vendoring EzPacker as a Git submodule inside `extern/EzPacker`:

```cmake
add_subdirectory(extern/EzPacker EXCLUDE_FROM_ALL)

target_link_libraries(my_compiler PRIVATE
    EzCompiler
    EzMir
    EzCore
    EzTargetsX86_64Registration
)
```

### EzPacker CMake Targets Reference
- **`EzCore`**: Foundational memory resources (`std::pmr`), `FlexInt`, `FlexFloat`, `DiagnosticCollector`, and `SourceManager`.
- **`EzMir`**: Intermediate Representation, builders, operands, SSA transformations, and dominator analysis.
- **`EzDsl`**: Declarative target specification parsers, Sema, and C++ code generator libraries.
- **`EzCodeEmitter`**: Section node stream, relocation fixups, and ELF64/COFF binary writers.
- **`EzTriple`**: Target platform parser and normalization predicates.
- **`EzCompiler`**: Compilation pipeline, driver context, and target resolver.
- **`EzTargetsX86_64`**: x86-64 concrete backend descriptor, lowerers, instruction selector, and encoder.
- **`EzTargetsX86_64Registration`**: Startup registration hook for the x86-64 target factory.

---

## 📖 Building Documentation

To compile this documentation website locally:

```powershell
# Using the CMake target
cmake --build cmake-build-debug --target docs

# Or using Doxygen directly
doxygen docs/Doxyfile
```

The resulting website is located at `docs/html/index.html`. Open it in any browser:

```powershell
Start-Process docs/html/index.html
```

---

## ❓ Troubleshooting & FAQs

### MSVC Runtime Mismatch in Shared Builds
**Symptom**: Unresolved external symbols or heap corruption when allocating across DLL boundaries.  
**Solution**: Ensure that your downstream application is built with the identical runtime library matching EzPacker (`/MDd` in Debug, `/MD` in Release). Alternatively, configure `-DEZPACKER_BUILD_STATIC=ON`.

### AddressSanitizer MSVC Symbol Errors
**Symptom**: Link errors citing `clang_rt.asan_dynamic-x86_64.lib` when `-DEZPACKER_ENABLE_SANITIZERS=ON`.  
**Solution**: Ensure the Visual Studio Installer component **C++ AddressSanitizer** is installed under Desktop development with C++.

### DLL Loading Error on Windows
**Symptom**: `EzCompilerEndToEndTests.exe` or `ezc.exe` crashes immediately upon launch with missing DLL error.  
**Solution**: CMake outputs all runtime DLLs directly into `bin/` (`cmake-build-debug/bin/`). Run executables directly from or ensure `bin/` is in your `PATH`.
