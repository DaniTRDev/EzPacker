# Plan Part 1: Doxygen Infrastructure, Styling, and Main Landing Page

## 1. Objective
Establish the Doxygen documentation infrastructure for **EzPacker**, including configuration, modern responsive styling, assets, CMake integration, and the primary landing page (`docs/mainpage.md`).

---

## 2. Deliverables & File Locations

| File | Purpose |
|:---|:---|
| [`docs/Doxyfile`](file:///E:/Repos/EzPacker/docs/Doxyfile) | Master Doxygen 1.16+ configuration file. |
| [`docs/assets/custom.css`](file:///E:/Repos/EzPacker/docs/assets/custom.css) | Custom modern CSS stylesheet (responsive layout, dark/light theme, modern typography, code blocks, alerts, cards). |
| [`docs/assets/logo.svg`](file:///E:/Repos/EzPacker/docs/assets/logo.svg) | High-resolution SVG vector logo for EzPacker. |
| [`docs/mainpage.md`](file:///E:/Repos/EzPacker/docs/mainpage.md) | Main entry point (`@mainpage`) containing the hero banner, interactive navigation grid, and architectural pipeline overview. |
| [`CMakeLists.txt`](file:///E:/Repos/EzPacker/CMakeLists.txt) | Root CMake configuration update to detect Doxygen and expose the `docs` target (`cmake --build . --target docs`). |

---

## 3. Detailed Technical Specifications

### A. Doxygen Configuration (`docs/Doxyfile`)
- **Project Identity**:
  - `PROJECT_NAME = "EzPacker"`
  - `PROJECT_BRIEF = "A Modular, Multi-Tier Compiler Infrastructure & Backend in C++20"`
  - `PROJECT_NUMBER = "0.1.0"`
  - `PROJECT_LOGO = docs/assets/logo.svg`
- **Output Settings**:
  - `OUTPUT_DIRECTORY = docs/html`
  - `GENERATE_HTML = YES`
  - `GENERATE_LATEX = NO`
  - `HTML_OUTPUT = .`
  - `HTML_FILE_EXTENSION = .html`
- **Input Sources**:
  - `INPUT = docs/mainpage.md docs/pages EzCore/include EzMir/include EzDsl EzCodeEmitter/include EzTriple/include EzCompiler/include EzTargets/X86_64/include`
  - `USE_MDFILE_AS_MAINPAGE = docs/mainpage.md`
  - `RECURSIVE = YES`
  - `FILE_PATTERNS = *.h *.hpp *.md *.dox`
  - `EXCLUDE_PATTERNS = */tests/* */cmake-build* */.git/* */LibBf/*`
- **Navigation & Theme**:
  - `GENERATE_TREEVIEW = YES` (sidebar tree navigation)
  - `FULL_SIDEBAR = YES`
  - `DISABLE_INDEX = NO`
  - `SEARCHENGINE = YES` (interactive real-time search box)
  - `HTML_EXTRA_STYLESHEET = docs/assets/custom.css`
  - `HTML_COLORSTYLE = TOGGLE` (support dark/light switching)
- **C++ Extraction Rules**:
  - `EXTRACT_ALL = YES`
  - `EXTRACT_PRIVATE = NO`
  - `EXTRACT_STATIC = YES`
  - `JAVADOC_AUTOBRIEF = YES`
  - `BUILTIN_STL_SUPPORT = YES`
  - `MARKDOWN_SUPPORT = YES`
  - `AUTOLINK_SUPPORT = YES`

### B. Modern Styling (`docs/assets/custom.css`)
- Clean typography utilizing modern system sans fonts (Inter, Segoe UI, Roboto) and monospace fonts for code (Cascadia Code, JetBrains Mono, Fira Code).
- High-contrast syntax highlighting for C++, MIR, and DSL code snippets.
- Interactive card grid on the landing page with hover transitions for quick navigation.
- Alert callouts styling (`[!NOTE]`, `[!TIP]`, `[!IMPORTANT]`, `[!WARNING]`, `[!CAUTION]`).
- Clean table formatting with alternating row colors and distinct header styling.

### C. Primary Landing Page (`docs/mainpage.md`)
- **Hero Section**: Mission statement and design philosophy of EzPacker (zero-cost abstractions, multi-tier IR, declarative DSL, retargetability).
- **Navigation Grid**: Direct, styled card links to:
  1. 🚀 **Getting Started & Setup**: Compiler prerequisites, build instructions, CMake integration.
  2. 💡 **Examples & Use Cases**: AOT compilation, in-memory JIT backend, custom passes, binary inspection.
  3. 🎯 **Adding a Target Architecture**: Step-by-step masterclass on bringing up a new architecture.
  4. 🧩 **Subsystem Deep Dives**: EzCore, EzMir, EzDsl, EzCodeEmitter, EzCompiler, EzTargets, EzTriple.
  5. 📚 **API Reference**: Classes, namespaces, interfaces, and function documentation.
- **End-to-End Pipeline Diagram**: ASCII / Mermaid visualization tracing input textual MIR or AST down to final ELF/COFF binary object emission.
- **Quickstart Code Snippet**: A minimal 10-line C++ snippet showing how to invoke `EzCompiler::CompilationPipeline` programmatically.

### D. CMake Integration (`CMakeLists.txt`)
- Use `find_package(Doxygen OPTIONAL_COMPONENTS dot)`.
- If `DOXYGEN_FOUND`, register `add_custom_target(docs ...)` with working directory set to project root.
- Print informational status message during CMake configuration indicating whether doc building is available.

---

## 4. Verification & Acceptance Criteria
1. Running `doxygen docs/Doxyfile` completes with zero fatal errors.
2. Opening `docs/html/index.html` displays the styled landing page with logo, search bar, sidebar tree, and navigation cards.
3. CMake target `docs` builds successfully when Doxygen is installed.
