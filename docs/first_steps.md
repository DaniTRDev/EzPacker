# EzPacker: First Steps & Quickstart

[EzPacker Documentation Index](index.md) > **First Steps**

---

## 1. Introduction

EzPacker is a modern C++20 compiler backend and code generation framework. It accepts Machine Intermediate Representation (MIR) and transforms it through middle-end SSA optimizations, table-driven legalization, tree-matching instruction selection, graph-coloring register allocation, and frame lowering into native, relocatable machine code object files (`.o` for ELF64 on Linux; `.obj` for PE/COFF on Windows).

This quickstart guide walks you through:
1. Writing your first MIR module.
2. Compiling the module with the `EzCompiler` command-line driver.
3. Inspecting the compiler pipeline stages.
4. Linking the resulting object file into a runnable native binary.

---

## 2. Writing Your First MIR File

EzPacker MIR is a strongly-typed, human-readable intermediate representation. Create a file named `simple_math.mir`:

```mir
// simple_math.mir: Adds two 32-bit integers and multiplies by a constant.

fn @calculate(i32 %a, i32 %b) -> i32 {
entry:
    %sum = ADD i32 %a, %b;
    %factor = MOV i32 10;
    %result = IMUL i32 %sum, %factor;
    RET i32 %result;
}
```

### Syntax Breakdown
- **Function Declaration**: `fn @calculate(i32 %a, i32 %b) -> i32` defines an exported function returning a 32-bit integer.
- **Basic Blocks**: `entry:` marks the entry basic block. Every basic block must end with a terminator instruction (`RET`, `BR_COND`, `JMP`, or `UNREACHABLE`).
- **Virtual Registers**: `%sum`, `%factor`, and `%result` are virtual registers. In SSA form, each virtual register is assigned exactly once.
- **Constants**: Immediate values like `10` are parsed into machine-independent `FlexInt` constants.

---

## 3. Compiling with `EzCompiler`

The `EzCompiler` executable drives the compilation pipeline.

### 3.1 Basic Compilation to Native Object File
To compile `simple_math.mir` to an object file matching your host platform:

```bash
EzCompiler simple_math.mir -o simple_math.o
```
On Windows, `EzCompiler` automatically emits a PE/COFF object file `simple_math.obj`; on Linux, it emits an ELF64 object file `simple_math.o`.

### 3.2 Explicit Target Triples
You can cross-compile or explicitly request a specific OS object format using `--target`:

```bash
# Target Linux ELF64 (uses System V AMD64 calling convention)
EzCompiler simple_math.mir --target x86_64-unknown-linux-gnu -o simple_math.o

# Target Windows PE/COFF (uses Microsoft Win64 calling convention)
EzCompiler simple_math.mir --target x86_64-pc-windows-coff -o simple_math.obj
```

---

## 4. Inspecting the Compiler Pipeline Stages

EzPacker provides inspection gates that halt compilation at specific milestones and dump the intermediate state to `stdout`:

### 4.1 Inspecting Middle-End Generic MIR (`--emit-mir`)
Dumps the generic SSA MIR after control-flow analysis, SSA construction, and liveness analysis:
```bash
EzCompiler simple_math.mir --emit-mir
```

### 4.2 Inspecting Legalized MIR (`--emit-legalized-mir`)
Dumps the MIR after type and opcode legalization (e.g. scalar widening, strength reduction, libcall substitution):
```bash
EzCompiler simple_math.mir --emit-legalized-mir
```

### 4.3 Inspecting Target-Lowered Machine MIR (`--emit-lowered-mir`)
Dumps the MIR after instruction selection, graph-coloring register allocation, and frame lowering (PEI):
```bash
EzCompiler simple_math.mir --emit-lowered-mir
```
*Notice that virtual registers (`%sum`, `%factor`) have been replaced by physical hardware registers (`edi`, `esi`, `eax` on Linux; `ecx`, `edx`, `eax` on Windows), and the function includes prologue/epilogue instructions.*

### 4.4 Emitting Human-Readable Assembly (`--emit-asm` or `-S`)
Emits formatted assembly instructions:
```bash
EzCompiler simple_math.mir --emit-asm -o simple_math.s
```

### 4.5 Tracing Passes & Execution Times
To observe the compilation pass pipeline in real time:
```bash
EzCompiler simple_math.mir --print-passes --time-passes -o simple_math.o
```
Output:
```text
[Pipeline] Running CodeFlowAnalysisPass on @calculate ... [0.042 ms]
[Pipeline] Running NonSsaToSsaPass on @calculate ... [0.031 ms]
[Pipeline] Running LivenessAnalysisPass on @calculate ... [0.058 ms]
[Pipeline] Running MirFunctionSignatureLegalizerPass on @calculate ... [0.021 ms]
[Pipeline] Running MirLegalizerPass on @calculate ... [0.065 ms]
[Pipeline] Running MirAbiLowererPass on @calculate ... [0.049 ms]
[Pipeline] Running MirInstructionSelectorPass on @calculate ... [0.112 ms]
[Pipeline] Running MirRegisterAllocatorPass on @calculate ... [0.184 ms]
[Pipeline] Running MirFrameLowererPass on @calculate ... [0.038 ms]
[Emission] Emitted 28 bytes to .text section.
[Writer] Generated 64-bit relocatable object file: simple_math.o
```

---

## 5. Linking into an Executable

The object files produced by `EzPacker` are 100% compliant with standard system linkers (`ld`, `lld`, `link.exe`, `gcc`, `clang`, `cl.exe`).

### 5.1 Writing a C Test Harness
Create `main.c` to call your compiled function:

```c
#include <stdio.h>

// Declare the external MIR function
extern int calculate(int a, int b);

int main(void) {
    int x = 15;
    int y = 7;
    int result = calculate(x, y);
    printf("calculate(%d, %d) = %d\n", x, y, result);
    return 0;
}
```

### 5.2 Linking on Linux (GCC / Clang)
```bash
gcc main.c simple_math.o -o my_app
./my_app
# Output: calculate(15, 7) = 220
```

### 5.3 Linking on Windows (MSVC)
```cmd
cl main.c simple_math.obj /Fe:my_app.exe
my_app.exe
:: Output: calculate(15, 7) = 220
```

---

## 6. Inspecting EzDsl Files (For Target Developers)

If you are developing or modifying target descriptions, register sets, or instruction selection patterns, you can use the `EzDslCli` tool to validate DSL files:

```bash
# Validate and dump AST of legalization rules
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_rules.lrd --dump-ast

# Validate and dump symbol definitions of calling conventions
EzDslCli -i EzTargets/X86_64/targets/x86_64/x86_64_calling_conv.ezcc --dump-symbols
```

---

## 7. Next Steps

- Consult the [Build Guide](build_guide.md) to set up and compile EzPacker from source.
- Explore the [Examples Guide](examples.md) for deep dives into complex control flow, 64-bit hashing, memory load-folding, and recursion.
- Learn how to add new architectures in [How to Build a Target Architecture](how_to_build_a_target.md).
- Browse the subproject documentation:
  - [EzCore](projects/EzCore.md)
  - [EzMir](projects/EzMir.md)
  - [EzDsl](projects/EzDsl.md)
  - [EzCodeEmitter](projects/EzCodeEmitter.md)
  - [EzTriple](projects/EzTriple.md)
  - [EzCompiler](projects/EzCompiler.md)
  - [EzTargets](projects/EzTargets.md)
- Explore the full C++ API in the [Doxygen Documentation](doxygen/index.html).
