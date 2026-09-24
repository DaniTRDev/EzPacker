# EzPacker Doxygen Documentation Plan: Master Index

This directory contains the modular implementation plans for building the complete, production-grade **Doxygen Documentation Site** for EzPacker. Each plan is self-contained with clear separations so that you can direct execution step-by-step or all at once.

---

## Modular Plan Index

| Part | Document | Title & Focus Area | Key Deliverables |
|:---|:---|:---|:---|
| **Part 1** | [`DocPlan_01_Infrastructure_And_Landing.md`](file:///E:/Repos/EzPacker/CodeSnippets/DocPlan_01_Infrastructure_And_Landing.md) | **Infrastructure, Modern Styling & Main Landing Page** | `docs/Doxyfile`, `docs/assets/custom.css`, `docs/assets/logo.svg`, `docs/mainpage.md`, `CMakeLists.txt` (`docs` target). |
| **Part 2** | [`DocPlan_02_Setup_And_GettingStarted.md`](file:///E:/Repos/EzPacker/CodeSnippets/DocPlan_02_Setup_And_GettingStarted.md) | **Setup & Getting Started Guide** | `docs/pages/getting_started.md` (prerequisites, build, CMake flags, CTest, CLI tools `ezc`/`EzDslCli`, downstream CMake integration, troubleshooting). |
| **Part 3** | [`DocPlan_03_Examples_And_UseCases.md`](file:///E:/Repos/EzPacker/CodeSnippets/DocPlan_03_Examples_And_UseCases.md) | **Examples & Real-World Use Cases** | `docs/pages/examples_use_cases.md` (4 real-world use cases + 5 complete, runnable C++ examples: programmatic IR builder, custom SSA optimization pass, JIT/AOT pipeline, binary section emission, textual MIR parser). |
| **Part 4** | [`DocPlan_04_Adding_A_Target_Architecture.md`](file:///E:/Repos/EzPacker/CodeSnippets/DocPlan_04_Adding_A_Target_Architecture.md) | **Masterclass: Adding a Target Architecture** | `docs/pages/adding_a_target.md` (4-phase comprehensive retargeting guide: DSL specifications `.tdesc`/`.idf`/`.ezcc`/`.lad`/`.lrd`/`.isf`, generated C++ stubs, `TargetDesc`, `FrameLowerer`, `CodeEmitter`, `RelocationResolver`, target registration & tests). |
| **Part 5** | [`DocPlan_05_Subsystems_And_Api_Reference.md`](file:///E:/Repos/EzPacker/CodeSnippets/DocPlan_05_Subsystems_And_Api_Reference.md) | **Subsystems Deep Dives & API Groups** | `docs/pages/subsystem_guides.md`, `@defgroup` module definitions, public C++ header docstrings (`@brief`, `@param`, `@return`). |
| **Part 6** | [`DocPlan_06_Build_Validation_And_CI.md`](file:///E:/Repos/EzPacker/CodeSnippets/DocPlan_06_Build_Validation_And_CI.md) | **Build Generation, Validation & Verification** | Doxygen HTML generation (`docs/html/index.html`), interactive search check, sidebar tree validation, CTest 93-test pass, `graphify` synchronization. |

---

## Execution Instructions
You can review the individual plans above and command execution in any of the following modes:
- **Sequential Execution**: Tell me *"Build Part 1"*, *"Build Part 2"*, etc.
- **Batch Execution**: Tell me *"Build Parts 1 through 3"* or *"Execute the full plan"*.
