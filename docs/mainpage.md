# EzPacker Documentation {#mainpage}

<div class="ez-hero">
  <img src="logo.svg" alt="EzPacker Logo" width="96" style="margin-bottom: 1rem;" />
  <h1>EzPacker</h1>
  <p class="tagline">A High-Performance, Multi-Tier Compiler Infrastructure and Retargetable Binary Backend written in Modern C++20.</p>
  <div>
    <span class="ez-badge">ISO C++20</span>
    <span class="ez-badge">Multi-Tier SSA IR</span>
    <span class="ez-badge">Declarative DSL Engine</span>
    <span class="ez-badge">Arbitrary-Precision Math</span>
    <span class="ez-badge">ELF64 & COFF Emitter</span>
    <span class="ez-badge">Graph-Coloring RegAlloc</span>
  </div>
</div>

---

## 🧭 Essential Guides & Documentation Hub

Explore the core guides below for step-by-step tutorials, real-world examples, and architectural deep dives:

<div class="ez-card-grid">
  <div class="ez-card">
    <div class="card-icon">🚀</div>
    <a class="card-title" href="getting_started.html">Getting Started &amp; Setup</a>
    <p class="card-desc">Complete prerequisites, out-of-source CMake build instructions, CTest execution, CLI driver usage, downstream project integration, and troubleshooting.</p>
  </div>

  <div class="ez-card">
    <div class="card-icon">💡</div>
    <a class="card-title" href="examples_use_cases.html">Examples &amp; Use Cases</a>
    <p class="card-desc">Real-world use cases (AOT, JIT, binary rewriting) alongside copy-paste-ready, compilable C++ and MIR examples for IR builders, custom passes, and object emission.</p>
  </div>

  <div class="ez-card">
    <div class="card-icon">🎯</div>
    <a class="card-title" href="adding_a_target.html">Adding a Target Architecture</a>
    <p class="card-desc">Comprehensive, end-to-end masterclass tutorial for porting EzPacker to a new target architecture (DSL modeling, C++ lowerers, emitters, and registration).</p>
  </div>

  <div class="ez-card">
    <div class="card-icon">🧩</div>
    <a class="card-title" href="subsystem_guides.html">Subsystems Architecture</a>
    <p class="card-desc">Detailed architectural breakdowns of EzCore, EzMir, EzDsl, EzCodeEmitter, EzTriple, EzCompiler, and EzTargets with module taxonomies.</p>
  </div>

  <div class="ez-card">
    <div class="card-icon">📚</div>
    <a class="card-title" href="topics.html">Modules &amp; API Reference</a>
    <p class="card-desc">Browse documented C++ classes, interfaces, namespaces, and utility functions grouped by subsystem taxonomy.</p>
  </div>

  <div class="ez-card">
    <div class="card-icon">🔍</div>
    <a class="card-title" href="annotated.html">Class Index</a>
    <p class="card-desc">Alphabetical index and hierarchical view of all core compiler data structures, builders, passes, and descriptors.</p>
  </div>
</div>

---

## 🏗️ End-to-End Compilation Pipeline

EzPacker processes source intermediate representations into native, relocatable binary object files through a phased, strictly typed compilation pipeline:

```
[ Input Source / Frontend ] ──▶ [ Textual .mir / AST ]
                                          │
                                          ▼
                         ┌─────────────────────────────────┐
                         │   Middle-End Optimization       │
                         │   • Non-SSA to SSA Pass         │
                         │   • Cooper-Harvey Dominance     │
                         │   • Cytron Phi Insertion        │
                         │   • CFG & Liveness Analysis     │
                         └────────────────┬────────────────┘
                                          │ High-Level MIR
                                          ▼
                         ┌─────────────────────────────────┐
                         │   Legalization Engine           │
                         │   • Table-Driven Legality       │
                         │   • Scalar Clamping / Widening  │
                         │   • Libcall & Custom Lowering   │
                         └────────────────┬────────────────┘
                                          │ Legalized MIR
                                          ▼
                         ┌─────────────────────────────────┐
                         │   Target Lowering & Selection   │
                         │   • Tree Pattern Matching (ISF) │
                         │   • ABI Argument & Return Pack  │
                         │   • Frame Setup & Stack Layout  │
                         │   • Graph-Coloring Reg Alloc    │
                         └────────────────┬────────────────┘
                                          │ Target-Low MIR
                                          ▼
                         ┌─────────────────────────────────┐
                         │   Binary Code Emission          │
                         │   • Machine Instruction Encoder │
                         │   • Doubly-Linked Stream Nodes  │
                         │   • Relocation Offset Patching  │
                         │   • ELF64 / COFF Object Writer  │
                         └────────────────┬────────────────┘
                                          │
                                          ▼
                              [ Output .o / .obj Binary ]
```

---

## ⚡ Quickstart C++ Example

Here is how you can invoke the EzPacker compilation pipeline programmatically in your own compiler or JIT engine:

```cpp
#include "Compiler/CompilationPipeline.h"
#include "Compiler/DriverContext.h"
#include "Compiler/TargetResolver.h"
#include "Builder/MirBuilderContext.h"
#include "EzTargetsX86_64Registration.h"
#include <iostream>

int main()
{
    // 1. Initialize registered architecture targets
    EzTargets::X86_64::registerTarget();

    // 2. Set up driver session context
    EzCompiler::CommandLineOptions opts;
    opts.m_targetTriple = "x86_64-pc-windows-msvc";
    opts.m_optLevel = EzCompiler::OptimizationLevel::O2;
    opts.m_outputFile = "output.obj";

    EzCompiler::DriverContext driverCtx(opts);

    // 3. Resolve the target architecture factory
    EzCompiler::ResolvedTarget target = EzCompiler::TargetResolver::resolve(
        driverCtx.getTriple(), driverCtx.getMirContext());

    if (!target.isValid()) {
        std::cerr << "Failed to resolve target: " << opts.m_targetTriple << "\n";
        return 1;
    }

    // 4. Construct and execute the compilation pipeline
    EzCompiler::CompilationPipeline pipeline(driverCtx, target);
    if (!pipeline.execute()) {
        std::cerr << "Compilation failed.\n";
        return 1;
    }

    std::cout << "Successfully compiled object: " << opts.m_outputFile << "\n";
    return 0;
}
```

---

## 📂 Subprojects Navigation

| Subproject | Description | Key Components |
|:---|:---|:---|
| **EzCore** | Foundational utilities, diagnostics, and math | `DiagnosticCollector`, `FlexInt`, `FlexFloat`, `SourceManager`, `DenseBitSet`, `IntrusiveLinkedList`. |
| **EzMir** | Machine Intermediate Representation | `MirModule`, `MirFunction`, `MirBlock`, `MirInstruction`, `MirOperand`, `MirTypeTable`, `MirPassManager`. |
| **EzDsl** | Declarative target definition languages | `.tdesc`, `.idf`, `.ezcc`, `.lad`, `.lrd`, `.isf`, Lexy parser combinators, Sema, 10 code generators. |
| **EzCodeEmitter** | Doubly-linked stream & object writers | `CodeSection`, `DataNode`, `LabelNode`, `AlignNode`, `TargetCodeRelocationType`, `Elf64Writer`, `CoffWriter`. |
| **EzTriple** | Target platform & environment triple | `TargetTriple`, normalized architecture, vendor, OS, environment, and ABI predicates. |
| **EzCompiler** | Standalone driver & pipeline orchestration | `ezc`, `EzCompilerLib`, `DriverContext`, `TargetResolver`, `CompilationPipeline`, `EmissionEngine`. |
| **EzTargets** | Concrete hardware architecture backends | `X86_64TargetDesc`, `X86_64FrameLowerer`, `X86_64TargetInstructionSelector`, `X86_64CodeEmitter`. |

---

> [!TIP]
> Ready to build the project or inspect working examples? Head over to the **[Getting Started & Setup Guide](getting_started.html)** or dive straight into **[Examples & Use Cases](examples_use_cases.html)**!
