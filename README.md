# EzPacker

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![CMake](https://img.shields.io/badge/CMake-3.27%2B-064F8C.svg?logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**EzPacker** is a modular, high-performance compiler backend, Machine Intermediate Representation (MIR) framework, and binary code generation engine written in modern ISO C++20.

EzPacker transforms strongly-typed, SSA-form intermediate representation into native, relocatable machine code object files (**ELF64** `.o` for Linux; **PE/COFF** `.obj` for Windows) targeting x86-64 with full SSE and AVX vector extension support.

---

## 📚 Documentation

All comprehensive documentation resides in the [`docs/`](docs/) directory:

- 🏠 **[Documentation Landing Page](docs/index.md)** - Global architecture, pipeline overview, and navigation hub.
- 🚀 **[First Steps & Quickstart](docs/first_steps.md)** - Writing your first MIR file, compiling, and linking with C/C++.
- 🛠️ **[Comprehensive Build Guide](docs/build_guide.md)** - Prerequisites, CMake options, compiling on Windows and Linux, and running tests.
- 💡 **[Examples & Use Cases](docs/examples.md)** - In-depth breakdown of all 9 bundled MIR modules with assembly outputs.
- 🎯 **[How to Build a Target Architecture](docs/how_to_build_a_target.md)** - Guide to adding and building new CPU targets from first principles.
- 🔍 **[Generated Doxygen API Documentation](docs/doxygen/index.html)** - Interactive C++ API reference.

### Subproject Documentation
- **[EzCore](docs/projects/EzCore.md)** - PMR memory allocators, diagnostic engine, `FlexInt`/`FlexFloat`, `DenseBitSet`, `IntrusiveLinkedList`.
- **[EzMir](docs/projects/EzMir.md)** - Intermediate representation: functions, blocks, instructions, operands, SSA pass pipeline.
- **[EzDsl](docs/projects/EzDsl.md)** - Meta-compiler toolkit and `ezdsl_cli` driver for all 8 DSL dialects.
- **[EzCodeEmitter](docs/projects/EzCodeEmitter.md)** - Binary machine code emission, section management, ELF64, and PE/COFF writers.
- **[EzTriple](docs/projects/EzTriple.md)** - Backend lowering: Legalizer, ABI lowerer, instruction selector, register allocator, and frame lowerer.
- **[EzCompiler](docs/projects/EzCompiler.md)** - Compiler driver executable, options parsing, and pipeline orchestration.
- **[EzTargets](docs/projects/EzTargets.md)** - Architecture backends, featuring the x86-64 target and vector extensions.

---

## ⚡ Quick Build

```bash
# Configure with CMake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build compiler, tools, and test suites
cmake --build build -j

# Run the test suite
ctest --test-dir build --output-on-failure
```

For detailed instructions and platform-specific commands, see the **[Build Guide](docs/build_guide.md)**.
