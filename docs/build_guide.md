# EzPacker: Comprehensive Build Guide

[EzPacker Documentation Index](index.md) > **Build Guide**

---

## 1. System Requirements & Toolchains

EzPacker is written in modern **C++20**. Building the project requires a compiler and standard library supporting C++20 language features (concepts, `<format>`, `<ranges>`, and polymorphic memory resources `<memory_resource>`).

### Supported Platforms & Compilers
- **Windows 10 / 11**:
  - Microsoft Visual C++ (MSVC) 2022 version 17.8 or newer (Visual Studio 2022).
  - Clang 16.0 or newer with MSVC toolchain (`clang-cl`).
- **Linux (x86_64)**:
  - GCC 13.1 or newer with `libstdc++`.
  - Clang 16.0 or newer with `libc++` or `libstdc++`.
- **macOS (x86_64 / Apple Silicon)**:
  - Apple Clang 15.0 or newer (LLVM 16+).

### Required Build Tools
- **CMake**: Version 3.27 or newer (`cmake_minimum_required(VERSION 3.27)`).
- **Ninja** or **Make** (recommended generator on Linux; Ninja also supported on Windows).
- **Git**: For automated retrieval of dependencies via `FetchContent`.
- **Doxygen** (optional, recommended): Version 1.9 or newer for building local API documentation.

---

## 2. Dependencies Architecture

EzPacker automates third-party dependency management through CMake's `FetchContent`. You do not need to manually clone or install external libraries before building.

### Fetched Dependencies (`CMake/Vendor.cmake`)
| Dependency | Repository | Purpose |
| :--- | :--- | :--- |
| **`EzLib`** | `https://github.com/DaniTRDev/EzLib` | CMake scaffolding, bootstrapping macros, and compiler configuration. |
| **`GTest`** | `https://github.com/google/googletest` | GoogleTest framework for unit and integration test suites. |
| **`libtommath`**| `https://github.com/libtom/libtommath` | Fast arbitrary-precision integer arithmetic engine for `FlexInt`. |
| **`lexy`** | `https://github.com/foonathan/lexy` | Modern C++ parser combinator library used by `EzDsl::Lexer`. |
| **`argparse`** | `https://github.com/p-ranav/argparse` | Argument parsing library used by `EzCompiler` and `EzDslCli`. |

### Vendored In-Tree Dependencies
- **`LibBf`** (`LibBf/`): Fabrice Bellard's high-precision floating-point library used by `FlexFloat` to model IEEE 754 scalar and vector types (f32, f64, f128).

---

## 3. CMake Configuration Options

The following CMake cache variables customize the build:

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `EZPACKER_BUILD_TESTS` | `BOOL` | `ON` | Builds the full GoogleTest suite for all subprojects. |
| `EZPACKER_BUILD_STATIC`| `BOOL` | `OFF` | Builds EzPacker libraries as static archives (`.lib`/`.a`) instead of shared libraries (`.dll`/`.so`). |
| `EZPACKER_ENABLE_SANITIZERS` | `BOOL` | `OFF` | Enables AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan). |
| `CMAKE_BUILD_TYPE` | `STRING` | `Debug` | Standard CMake configuration: `Debug`, `Release`, `RelWithDebInfo`, or `MinSizeRel`. |

---

## 4. Step-by-Step Build Instructions

### 4.1 Windows (Visual Studio 2022 Generator)
From PowerShell or the Developer Command Prompt for VS 2022:

```powershell
# 1. Clone repository
git clone https://github.com/DaniTRDev/EzPacker.git
cd EzPacker

# 2. Configure build directory
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release

# 3. Compile all targets
cmake --build build --config Release -j
```
Binaries will be placed in `build/bin/` (or `build/bin/Release/`).

### 4.2 Windows / Linux (Ninja Generator)
Using Ninja delivers significantly faster parallel compilation:

```bash
# Configure with Ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile all targets in parallel
cmake --build build -j
```

### 4.3 Enabling Sanitizers (Debug Builds)
To build with AddressSanitizer and UndefinedBehaviorSanitizer enabled:
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DEZPACKER_ENABLE_SANITIZERS=ON
cmake --build build -j
```

---

## 5. Running the Test Suite

EzPacker includes exhaustive unit, integration, and end-to-end tests across every compiler subproject.

### 5.1 Running All Tests via CTest
```bash
ctest --test-dir build --output-on-failure
```

### 5.2 Test Target Breakdown
You can run individual test suites targeting specific subsystems:

| Test Target | Executable | Scope |
| :--- | :--- | :--- |
| **`EzMirTestSuite`** | `bin/EzMirTestSuite` | Tests in-memory MIR, block construction, SSA verification, and textual parsing. |
| **`EzDslLexerTestSuite`** | `bin/EzDslLexerTestSuite` | Tests lexical analysis and parser combinators for all 8 DSL dialects. |
| **`EzDslSemaTestSuite`** | `bin/EzDslSemaTestSuite` | Tests semantic analysis, symbol tables, scopes, and conflict detection. |
| **`EzDslCodeGeneratorsTestSuite`** | `bin/EzDslCodeGeneratorsTestSuite` | Tests C++ table synthesizers and code emitter output. |
| **`EzDslCliTestSuite`** | `bin/EzDslCliTestSuite` | Tests the `EzDslCli` driver executable. |
| **`EzTripleTestSuite`** | `bin/EzTripleTestSuite` | Tests legalizer matrices, ABI lowerer, instruction selector, register allocator, and frame lowerer. |
| **`EzCodeEmitterTestSuite`** | `bin/EzCodeEmitterTestSuite` | Tests binary machine encoding, branch relaxation, ELF64 and COFF object serialization. |
| **`EzCompilerTestSuite`** | `bin/EzCompilerTestSuite` | Tests compiler option parsing, target resolvers, and pipeline pass sequencing. |
| **`EzCompilerEndToEndTests`** | `bin/EzCompilerEndToEndTests` | Full end-to-end compilation of sample MIR modules to executable object files. |

Example running a single test suite:
```bash
./build/bin/EzCompilerEndToEndTests
```

---

## 6. Building the Documentation

EzPacker integrates Doxygen documentation generation directly into CMake.

### 6.1 Using the CMake `doxygen` / `docs` Target
If Doxygen is detected during CMake configuration, custom targets `doxygen` and `docs` are automatically created:

```bash
cmake --build build --target doxygen
# or
cmake --build build --target docs
```

### 6.2 Invoking Doxygen Directly
You can also generate the documentation directly from the repository root:
```bash
doxygen docs/Doxyfile
```

The generated HTML documentation will be located in `docs/doxygen/index.html`. Open this file in any web browser to explore the full interactive API reference.

---

## 7. Troubleshooting

- **`std::format` or C++20 Concept Errors**: Ensure your compiler is updated to at least MSVC 17.8+, GCC 13.1+, or Clang 16+.
- **PMR Linker Errors**: On Linux, ensure `libstdc++` or `libc++` is linked with C++20 support enabled.
- **Git FetchContent Timeouts**: If git cannot reach GitHub, configure your system's HTTP/HTTPS proxy or SSH keys before configuring CMake.

---

## 8. Next Steps

- Proceed to [First Steps](first_steps.md) to compile your first MIR file.
- Review [Examples & Use Cases](examples.md) to inspect real-world compiler outputs.
- Consult the [Subproject Documentation](projects/EzCompiler.md) for architectural deep dives.
