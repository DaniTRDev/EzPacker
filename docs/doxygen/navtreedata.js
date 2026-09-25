/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "EzPacker", "index.html", [
    [ "EzPacker: Modern C++20 Compiler Backend &amp; Code Generation Framework", "index.html", "index" ],
    [ "EzPacker: Comprehensive Build Guide", "md_docs_2build__guide.html", [
      [ "1. System Requirements &amp; Toolchains", "md_docs_2build__guide.html#autotoc_md2", [
        [ "Supported Platforms &amp; Compilers", "md_docs_2build__guide.html#autotoc_md3", null ],
        [ "Required Build Tools", "md_docs_2build__guide.html#autotoc_md4", null ]
      ] ],
      [ "2. Dependencies Architecture", "md_docs_2build__guide.html#autotoc_md6", [
        [ "Fetched Dependencies (<span class=\"tt\">CMake/Vendor.cmake</span>)", "md_docs_2build__guide.html#autotoc_md7", null ],
        [ "Vendored In-Tree Dependencies", "md_docs_2build__guide.html#autotoc_md8", null ]
      ] ],
      [ "3. CMake Configuration Options", "md_docs_2build__guide.html#autotoc_md10", null ],
      [ "4. Step-by-Step Build Instructions", "md_docs_2build__guide.html#autotoc_md12", [
        [ "4.1 Windows (Visual Studio 2022 Generator)", "md_docs_2build__guide.html#autotoc_md13", null ],
        [ "4.2 Windows / Linux (Ninja Generator)", "md_docs_2build__guide.html#autotoc_md14", null ],
        [ "4.3 Enabling Sanitizers (Debug Builds)", "md_docs_2build__guide.html#autotoc_md15", null ]
      ] ],
      [ "5. Running the Test Suite", "md_docs_2build__guide.html#autotoc_md17", [
        [ "5.1 Running All Tests via CTest", "md_docs_2build__guide.html#autotoc_md18", null ],
        [ "5.2 Test Target Breakdown", "md_docs_2build__guide.html#autotoc_md19", null ]
      ] ],
      [ "6. Building the Documentation", "md_docs_2build__guide.html#autotoc_md21", [
        [ "6.1 Using the CMake <span class=\"tt\">docs</span> Target", "md_docs_2build__guide.html#autotoc_md22", null ],
        [ "6.2 Invoking Doxygen Directly", "md_docs_2build__guide.html#autotoc_md23", null ]
      ] ],
      [ "7. Troubleshooting", "md_docs_2build__guide.html#autotoc_md25", null ],
      [ "8. Next Steps", "md_docs_2build__guide.html#autotoc_md27", null ]
    ] ],
    [ "EzPacker: Examples &amp; Use Cases", "md_docs_2examples.html", [
      [ "1. Overview", "md_docs_2examples.html#autotoc_md30", null ],
      [ "2. Walkthrough of Bundled Examples", "md_docs_2examples.html#autotoc_md32", [
        [ "2.1 <span class=\"tt\">arithmetic_32bit.mir</span>: 32-Bit Arithmetic &amp; Logic", "md_docs_2examples.html#autotoc_md33", [
          [ "Compilation &amp; Output (x86_64 Linux):", "md_docs_2examples.html#autotoc_md34", null ]
        ] ],
        [ "2.2 <span class=\"tt\">branch_control_flow.mir</span>: Conditional Branching &amp; Phi Nodes", "md_docs_2examples.html#autotoc_md36", [
          [ "How EzPacker Lowers This:", "md_docs_2examples.html#autotoc_md37", null ]
        ] ],
        [ "2.3 <span class=\"tt\">calling_conventions.mir</span>: System V vs Microsoft Win64 ABI", "md_docs_2examples.html#autotoc_md39", [
          [ "Side-by-Side ABI Comparison:", "md_docs_2examples.html#autotoc_md40", null ]
        ] ],
        [ "2.4 <span class=\"tt\">crypto_primitives.mir</span>: Feistel Rounds &amp; 64-Bit Mixers", "md_docs_2examples.html#autotoc_md42", null ],
        [ "2.5 <span class=\"tt\">math_ops.mir</span>: Multi-Function &amp; Strength Reduction", "md_docs_2examples.html#autotoc_md44", null ],
        [ "2.6 <span class=\"tt\">memory_fold.mir</span>: Instruction Selector Load-Folding", "md_docs_2examples.html#autotoc_md46", [
          [ "How Load-Folding Works:", "md_docs_2examples.html#autotoc_md47", null ]
        ] ],
        [ "2.7 <span class=\"tt\">multiple_returns.mir</span>: Functions with Multiple Epilogues", "md_docs_2examples.html#autotoc_md49", null ],
        [ "2.8 <span class=\"tt\">multi_arguments.mir</span>: Stack-Passed Arguments", "md_docs_2examples.html#autotoc_md51", null ],
        [ "2.9 <span class=\"tt\">recursive_factorial.mir</span>: Recursion &amp; Relocations", "md_docs_2examples.html#autotoc_md53", [
          [ "What Happens Under the Hood:", "md_docs_2examples.html#autotoc_md54", null ]
        ] ]
      ] ],
      [ "3. End-to-End Verification Harness", "md_docs_2examples.html#autotoc_md56", [
        [ "Build &amp; Run Commands", "md_docs_2examples.html#autotoc_md57", null ]
      ] ],
      [ "4. Next Steps", "md_docs_2examples.html#autotoc_md59", null ]
    ] ],
    [ "EzPacker: First Steps &amp; Quickstart", "md_docs_2first__steps.html", [
      [ "1. Introduction", "md_docs_2first__steps.html#autotoc_md62", null ],
      [ "2. Writing Your First MIR File", "md_docs_2first__steps.html#autotoc_md64", [
        [ "Syntax Breakdown", "md_docs_2first__steps.html#autotoc_md65", null ]
      ] ],
      [ "3. Compiling with <span class=\"tt\">EzCompiler</span>", "md_docs_2first__steps.html#autotoc_md67", [
        [ "3.1 Basic Compilation to Native Object File", "md_docs_2first__steps.html#autotoc_md68", null ],
        [ "3.2 Explicit Target Triples", "md_docs_2first__steps.html#autotoc_md69", null ]
      ] ],
      [ "4. Inspecting the Compiler Pipeline Stages", "md_docs_2first__steps.html#autotoc_md71", [
        [ "4.1 Inspecting Middle-End Generic MIR (<span class=\"tt\">--emit-mir</span>)", "md_docs_2first__steps.html#autotoc_md72", null ],
        [ "4.2 Inspecting Legalized MIR (<span class=\"tt\">--emit-legalized-mir</span>)", "md_docs_2first__steps.html#autotoc_md73", null ],
        [ "4.3 Inspecting Target-Lowered Machine MIR (<span class=\"tt\">--emit-lowered-mir</span>)", "md_docs_2first__steps.html#autotoc_md74", null ],
        [ "4.4 Emitting Human-Readable Assembly (<span class=\"tt\">--emit-asm</span> or <span class=\"tt\">-S</span>)", "md_docs_2first__steps.html#autotoc_md75", null ],
        [ "4.5 Tracing Passes &amp; Execution Times", "md_docs_2first__steps.html#autotoc_md76", null ]
      ] ],
      [ "5. Linking into an Executable", "md_docs_2first__steps.html#autotoc_md78", [
        [ "5.1 Writing a C Test Harness", "md_docs_2first__steps.html#autotoc_md79", null ],
        [ "5.2 Linking on Linux (GCC / Clang)", "md_docs_2first__steps.html#autotoc_md80", null ],
        [ "5.3 Linking on Windows (MSVC)", "md_docs_2first__steps.html#autotoc_md81", null ]
      ] ],
      [ "6. Inspecting EzDsl Files (For Target Developers)", "md_docs_2first__steps.html#autotoc_md83", null ],
      [ "7. Next Steps", "md_docs_2first__steps.html#autotoc_md85", null ]
    ] ],
    [ "How to Build a Target Architecture in EzPacker", "md_docs_2how__to__build__a__target.html", [
      [ "1. Architectural Philosophy &amp; Retargetability", "md_docs_2how__to__build__a__target.html#autotoc_md88", null ],
      [ "2. The 7 DSL Specifications", "md_docs_2how__to__build__a__target.html#autotoc_md90", [
        [ "2.1 Target Descriptor: <span class=\"tt\">&lt;target&gt;.tdesc</span>", "md_docs_2how__to__build__a__target.html#autotoc_md91", null ],
        [ "2.2 Calling Convention: <span class=\"tt\">&lt;target&gt;_calling_conv.ezcc</span>", "md_docs_2how__to__build__a__target.html#autotoc_md92", null ],
        [ "2.3 Legalization Actions: <span class=\"tt\">&lt;target&gt;_legalize.lad</span>", "md_docs_2how__to__build__a__target.html#autotoc_md93", null ],
        [ "2.4 Legalization Rules: <span class=\"tt\">&lt;target&gt;_rules.lrd</span>", "md_docs_2how__to__build__a__target.html#autotoc_md94", null ],
        [ "2.5 Target Instructions: <span class=\"tt\">&lt;target&gt;_instructions.idf</span>", "md_docs_2how__to__build__a__target.html#autotoc_md95", null ],
        [ "2.6 Instruction Selection Patterns: <span class=\"tt\">&lt;target&gt;_patterns.isf</span>", "md_docs_2how__to__build__a__target.html#autotoc_md96", null ]
      ] ],
      [ "3. Implementing the C++ Runtime Classes", "md_docs_2how__to__build__a__target.html#autotoc_md98", [
        [ "3.1 Subclassing <span class=\"tt\">TargetDesc</span>", "md_docs_2how__to__build__a__target.html#autotoc_md99", null ],
        [ "3.2 Subclassing <span class=\"tt\">MirFrameLowerer</span>", "md_docs_2how__to__build__a__target.html#autotoc_md100", null ],
        [ "3.3 Subclassing <span class=\"tt\">GenericCodeEmitter</span>", "md_docs_2how__to__build__a__target.html#autotoc_md101", null ]
      ] ],
      [ "4. CMake Integration", "md_docs_2how__to__build__a__target.html#autotoc_md103", null ],
      [ "5. Target Registration Hook", "md_docs_2how__to__build__a__target.html#autotoc_md105", null ],
      [ "6. Testing Your Target", "md_docs_2how__to__build__a__target.html#autotoc_md107", null ],
      [ "7. Next Steps", "md_docs_2how__to__build__a__target.html#autotoc_md109", null ]
    ] ],
    [ "EzCodeEmitter Subproject Documentation", "md_docs_2projects_2_ez_code_emitter.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md125", null ],
      [ "2. Dynamic Linked Code Sections (<span class=\"tt\">CodeSection.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md127", [
        [ "2.1 Section Categorization (<span class=\"tt\">SectionType</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md128", null ],
        [ "2.2 <span class=\"tt\">CodeSection</span> Operations", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md129", null ]
      ] ],
      [ "3. Emission Context &amp; Relocations (<span class=\"tt\">CodeEmitterContext.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md131", [
        [ "3.1 Relocation Fixup Types (<span class=\"tt\">TargetCodeRelocationType</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md132", null ],
        [ "3.2 Label &amp; Relocation Records", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md133", null ],
        [ "3.3 <span class=\"tt\">CodeEmitterContext</span> API", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md134", null ]
      ] ],
      [ "4. Generic Target Code Emitter (<span class=\"tt\">GenericCodeEmitter.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md136", null ],
      [ "5. Relocatable Object File Writers (<span class=\"tt\">IObjectWriter.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md138", [
        [ "5.1 <span class=\"tt\">Elf64Writer</span> (<span class=\"tt\">ObjectFormat/Elf64Writer.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md139", null ],
        [ "5.2 <span class=\"tt\">CoffWriter</span> (<span class=\"tt\">ObjectFormat/CoffWriter.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md140", null ]
      ] ],
      [ "6. Header &amp; Class Index", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md142", null ]
    ] ],
    [ "EzCompiler Subproject Documentation", "md_docs_2projects_2_ez_compiler.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_compiler.html#autotoc_md145", null ],
      [ "2. Driver Context &amp; Lifecycle (<span class=\"tt\">DriverContext.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md147", null ],
      [ "3. Target Triples &amp; Dynamic Target Resolution", "md_docs_2projects_2_ez_compiler.html#autotoc_md149", [
        [ "3.1 <span class=\"tt\">TargetTriple</span> (<span class=\"tt\">TargetTriple.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md150", null ],
        [ "3.2 <span class=\"tt\">TargetResolver</span> (<span class=\"tt\">TargetResolver.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md151", null ]
      ] ],
      [ "4. The Compilation Pipeline (<span class=\"tt\">CompilationPipeline.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md153", [
        [ "Pass Sequence:", "md_docs_2projects_2_ez_compiler.html#autotoc_md154", null ]
      ] ],
      [ "5. Machine Code Emission Engine (<span class=\"tt\">EmissionEngine.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md156", null ],
      [ "6. Command-Line Options (<span class=\"tt\">ezc</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md158", [
        [ "Exact CLI Options (<span class=\"tt\">CommandLineOptions.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md159", null ]
      ] ],
      [ "7. Header &amp; Class Index", "md_docs_2projects_2_ez_compiler.html#autotoc_md161", null ]
    ] ],
    [ "EzCore Subproject Documentation", "md_docs_2projects_2_ez_core.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_core.html#autotoc_md164", [
        [ "Key Responsibilities", "md_docs_2projects_2_ez_core.html#autotoc_md165", null ]
      ] ],
      [ "2. Polymorphic Memory Architecture (PMR)", "md_docs_2projects_2_ez_core.html#autotoc_md167", [
        [ "Core PMR Guidelines in EzPacker", "md_docs_2projects_2_ez_core.html#autotoc_md168", null ]
      ] ],
      [ "3. Diagnostic Engine", "md_docs_2projects_2_ez_core.html#autotoc_md170", [
        [ "3.1 Severity Classification (<span class=\"tt\">DiagnosticMessage.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md171", null ],
        [ "3.2 Diagnostic Records &amp; Notes", "md_docs_2projects_2_ez_core.html#autotoc_md172", null ],
        [ "3.3 Diagnostic Collector (<span class=\"tt\">DiagnosticCollector.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md173", null ],
        [ "3.4 Diagnostic Builder (<span class=\"tt\">DiagnosticBuilder.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md174", null ],
        [ "3.5 Usage Example: Reporting Compiler Diagnostics", "md_docs_2projects_2_ez_core.html#autotoc_md175", null ]
      ] ],
      [ "4. Arbitrary-Precision Constants: <span class=\"tt\">FlexInt</span> &amp; <span class=\"tt\">FlexFloat</span>", "md_docs_2projects_2_ez_core.html#autotoc_md177", [
        [ "4.1 <span class=\"tt\">FlexInt</span> (<span class=\"tt\">FlexNumber/FlexInt.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md178", [
          [ "Constructors &amp; State", "md_docs_2projects_2_ez_core.html#autotoc_md179", null ],
          [ "Arithmetic &amp; Overflow Checking", "md_docs_2projects_2_ez_core.html#autotoc_md180", null ],
          [ "Bitwise, Shift &amp; Slice Operations", "md_docs_2projects_2_ez_core.html#autotoc_md181", null ],
          [ "Legalization &amp; Splitting Helpers", "md_docs_2projects_2_ez_core.html#autotoc_md182", null ],
          [ "Serialization &amp; Conversions", "md_docs_2projects_2_ez_core.html#autotoc_md183", null ],
          [ "Example: <span class=\"tt\">FlexInt</span> in Compiler Constant Folding", "md_docs_2projects_2_ez_core.html#autotoc_md184", null ]
        ] ],
        [ "4.2 <span class=\"tt\">FlexFloat</span> (<span class=\"tt\">FlexNumber/FlexFloat.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md185", null ]
      ] ],
      [ "5. High-Performance Compiler Containers", "md_docs_2projects_2_ez_core.html#autotoc_md187", [
        [ "5.1 <span class=\"tt\">IntrusiveLinkedList&lt;T&gt;</span> (<span class=\"tt\">HelperClasses/IntrusiveLinkedList.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md188", [
          [ "Concept Requirements on Element Type <span class=\"tt\">T</span>", "md_docs_2projects_2_ez_core.html#autotoc_md189", null ],
          [ "API Methods", "md_docs_2projects_2_ez_core.html#autotoc_md190", null ]
        ] ],
        [ "5.2 <span class=\"tt\">DenseBitSet</span> (<span class=\"tt\">HelperClasses/DenseBitSet.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md191", [
          [ "Transfer Function Evaluation in Liveness Analysis", "md_docs_2projects_2_ez_core.html#autotoc_md192", null ]
        ] ]
      ] ],
      [ "6. Source Coordinate &amp; File Tracking", "md_docs_2projects_2_ez_core.html#autotoc_md194", [
        [ "6.1 Descriptors", "md_docs_2projects_2_ez_core.html#autotoc_md195", null ],
        [ "6.2 <span class=\"tt\">SourceManager</span> API", "md_docs_2projects_2_ez_core.html#autotoc_md196", null ]
      ] ],
      [ "7. Header &amp; Class Index", "md_docs_2projects_2_ez_core.html#autotoc_md198", null ]
    ] ],
    [ "EzDsl Subproject Documentation", "md_docs_2projects_2_ez_dsl.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_dsl.html#autotoc_md201", null ],
      [ "2. EzDSL Dialects &amp; File Extensions", "md_docs_2projects_2_ez_dsl.html#autotoc_md203", null ],
      [ "3. The 3-Stage Toolchain Architecture", "md_docs_2projects_2_ez_dsl.html#autotoc_md205", [
        [ "3.1 Lexer &amp; Recursive Descent Parser (<span class=\"tt\">EzDsl/Lexer/</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md206", null ],
        [ "3.2 Semantic Analysis (Sema) (<span class=\"tt\">EzDsl/Sema/</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md207", null ],
        [ "3.3 Code Generators (<span class=\"tt\">EzDsl/CodeGenerators/</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md208", null ]
      ] ],
      [ "4. Dialect Syntax Examples", "md_docs_2projects_2_ez_dsl.html#autotoc_md210", [
        [ "4.1 Type Definitions (<span class=\"tt\">types.tyf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md211", null ],
        [ "4.2 Register Definitions (<span class=\"tt\">registers.rdf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md212", null ],
        [ "4.3 Calling Convention (<span class=\"tt\">sysv.ccdf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md213", null ],
        [ "4.4 Legalization Rules (<span class=\"tt\">rules.lrd</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md214", null ],
        [ "4.5 Instruction Selection (<span class=\"tt\">patterns.isdf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md215", null ]
      ] ],
      [ "5. Command-Line Interface (<span class=\"tt\">ezdsl-cli</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md217", [
        [ "Exact CLI Options (<span class=\"tt\">Cli::CliOptions</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md218", null ],
        [ "CLI Execution Examples", "md_docs_2projects_2_ez_dsl.html#autotoc_md219", null ]
      ] ],
      [ "6. Header &amp; Class Index", "md_docs_2projects_2_ez_dsl.html#autotoc_md221", null ]
    ] ],
    [ "EzMir Subproject Documentation", "md_docs_2projects_2_ez_mir.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_mir.html#autotoc_md224", null ],
      [ "2. In-Memory Intermediate Representation (IR)", "md_docs_2projects_2_ez_mir.html#autotoc_md226", [
        [ "2.1 <span class=\"tt\">MirFunction</span> (<span class=\"tt\">Function/MirFunction.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md227", null ],
        [ "2.2 <span class=\"tt\">MirBlock</span> (<span class=\"tt\">Block/MirBlock.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md228", null ],
        [ "2.3 <span class=\"tt\">MirInstruction</span> (<span class=\"tt\">Instruction/MirInstruction.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md229", null ],
        [ "2.4 <span class=\"tt\">MirOperand</span> (<span class=\"tt\">Operand/MirOperand.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md230", null ],
        [ "2.5 <span class=\"tt\">MirType</span> &amp; <span class=\"tt\">MirTypeTable</span> (<span class=\"tt\">Type/MirType.h</span>, <span class=\"tt\">Type/MirTypeTable.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md231", null ]
      ] ],
      [ "3. Middle-End Pass Framework", "md_docs_2projects_2_ez_mir.html#autotoc_md233", [
        [ "3.1 <span class=\"tt\">CodeFlowAnalysisPass</span> (<span class=\"tt\">MirPasses/Passes/CodeFlowAnalysisPass.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md234", null ],
        [ "3.2 <span class=\"tt\">NonSsaToSsaPass</span> (<span class=\"tt\">MirPasses/Passes/NonSsaToSsaPass.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md235", null ],
        [ "3.3 <span class=\"tt\">LivenessAnalysisPass</span> (<span class=\"tt\">MirPasses/Passes/LivenessAnalysisPass.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md236", null ]
      ] ],
      [ "4. Programmatic MIR Construction (Builder API)", "md_docs_2projects_2_ez_mir.html#autotoc_md238", [
        [ "4.1 Builder Hierarchy", "md_docs_2projects_2_ez_mir.html#autotoc_md239", null ],
        [ "4.2 Complete Programmatic Example", "md_docs_2projects_2_ez_mir.html#autotoc_md240", null ]
      ] ],
      [ "5. Textual MIR Format", "md_docs_2projects_2_ez_mir.html#autotoc_md242", null ],
      [ "6. Header &amp; Class Index", "md_docs_2projects_2_ez_mir.html#autotoc_md244", null ]
    ] ],
    [ "EzTargets Subproject Documentation", "md_docs_2projects_2_ez_targets.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_targets.html#autotoc_md247", null ],
      [ "2. X86-64 Target Architecture Implementation", "md_docs_2projects_2_ez_targets.html#autotoc_md249", [
        [ "2.1 <span class=\"tt\">X86_64TargetDesc</span> (<span class=\"tt\">X86_64/include/X86_64TargetDesc.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md250", null ],
        [ "2.2 Hand-Written Lowering Shims (<span class=\"tt\">X86_64/include/X86_64Lowering.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md252", null ],
        [ "2.3 Table-Driven Machine Instruction Encoder (<span class=\"tt\">X86_64/include/Encoding/</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md254", [
          [ "Operand Slot Classifications (<span class=\"tt\">EncSlotKind</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md255", null ],
          [ "Instruction Forms (<span class=\"tt\">EncForm</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md256", null ]
        ] ],
        [ "2.4 Branch Relaxation Pass (<span class=\"tt\">X86_64/include/BranchRelaxation/BranchRelaxer.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md258", null ],
        [ "2.5 Frame Lowering (<span class=\"tt\">X86_64/include/X86_64FrameLowerer.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md260", null ],
        [ "2.6 Target Registration (<span class=\"tt\">X86_64/Registration/</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md262", null ]
      ] ],
      [ "3. Header &amp; Class Index", "md_docs_2projects_2_ez_targets.html#autotoc_md264", null ]
    ] ],
    [ "EzTriple Subproject Documentation", "md_docs_2projects_2_ez_triple.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_triple.html#autotoc_md267", null ],
      [ "2. The 5 Core Subsystems", "md_docs_2projects_2_ez_triple.html#autotoc_md269", [
        [ "2.1 The Legalizer (<span class=\"tt\">include/Legalizer/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md270", [
          [ "Legalization Action Kinds (<span class=\"tt\">LegalizeQuery.h</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md271", null ],
          [ "Legality Query &amp; Response", "md_docs_2projects_2_ez_triple.html#autotoc_md272", null ],
          [ "3-Tier Architecture", "md_docs_2projects_2_ez_triple.html#autotoc_md273", null ]
        ] ],
        [ "2.2 ABI Lowerer (<span class=\"tt\">include/AbiLowerer/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md275", null ],
        [ "2.3 Instruction Selector (<span class=\"tt\">include/InstructionSelector/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md277", null ],
        [ "2.4 Register Allocator (<span class=\"tt\">include/RegisterAllocator/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md279", [
          [ "Working Context (<span class=\"tt\">RegisterAllocatorCtx</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md280", null ],
          [ "Abstract Allocator Class (<span class=\"tt\">MirRegisterAllocator</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md281", null ]
        ] ],
        [ "2.5 Frame Lowerer (<span class=\"tt\">include/FrameLowerer/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md283", [
          [ "Lowering Sequence", "md_docs_2projects_2_ez_triple.html#autotoc_md284", null ]
        ] ]
      ] ],
      [ "3. Target Descriptors (<span class=\"tt\">include/Descriptors/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md286", [
        [ "3.1 <span class=\"tt\">TargetDesc</span> (<span class=\"tt\">Descriptors/TargetDesc.h</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md287", null ],
        [ "3.2 <span class=\"tt\">TargetBinaryDesc</span> (<span class=\"tt\">Descriptors/TargetBinaryDesc.h</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md288", null ]
      ] ],
      [ "4. Header &amp; Class Index", "md_docs_2projects_2_ez_triple.html#autotoc_md290", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerations", "globals_enum.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_argument_location_desc_8h.html",
"_legalize_bitcast_action_8h.html",
"_register_def_lang_ast_8h_source.html",
"class_code_generators_1_1_code_generator.html#a262cb4c5f40f75b05246a7f929125694",
"class_code_generators_1_1_cpp_target_desc_generator.html#a925379ccc1297290c970f87f1def2c47",
"class_ez_compiler_1_1_mir_module_loader.html#a0b311d0d91b4cd90e36402dfb77ede45",
"class_ez_targets_1_1_x86__64_1_1_x86__64_frame_lowerer.html#a5852a5d21919bd79cfa766be2f1f4abb",
"class_intrusive_linked_list.html#a190403c7389a1679243ae2ac858d5920",
"class_mir_global_var_builder.html#a36d397d22fd48072c2e135aa3c740b5f",
"class_mir_register_ref.html#a3314e75a6c5cd48a713ff00c79b48e87",
"dir_be9e14de3a55961a5b2656fbec9f2b55.html",
"md_docs_2projects_2_ez_dsl.html#autotoc_md211",
"namespace_ez_mir_1_1_ast.html#a3764413ba1f796a48239e3261ef78d90a4410ec34d9e6c1a68100ca0ce033fb17",
"struct_cli_1_1_cli_options.html#ab330271f5e57ce4f1a28f019081e6937",
"struct_d_s_l_1_1_ast_1_1_calling_conv_def_1_1_varargs_def.html#aa67d7fbac782464d787426234c1101d7",
"struct_d_s_l_1_1_ast_1_1_legalize_rule_def_1_1_rule_instruction.html",
"struct_d_s_l_1_1_parser_1_1_calling_conv_def_1_1_aggregate_condition_parser_1_1_size_cond_1_1_in_branch_1_1_size_list.html",
"struct_d_s_l_1_1_parser_1_1_calling_conv_def_1_1_pass_rule_parser_1_1_alias_source.html#a41a41ab7dac8d926773fe4eae298147c",
"struct_d_s_l_1_1_parser_1_1_calling_conv_def_1_1_sret_def_parser_1_1_ptr_decl.html#a3a24eb6c3f692018df1602d83fe97902",
"struct_d_s_l_1_1_parser_1_1_common_1_1_comment.html#a7b1f4c9e793eb621a13f0474c7d31c95",
"struct_d_s_l_1_1_parser_1_1_instruction_select_def_1_1_addr_mode_variant_rule_1_1_when_item.html#aba31efa2811b175c96ca5ad08ff23df4",
"struct_d_s_l_1_1_parser_1_1_instruction_select_def_1_1_typed_prefix_ssa_operand.html#a4f2c236cf691b2ac5946a5e6096d908e",
"struct_d_s_l_1_1_parser_1_1_legalize_action_def_1_1_legalize_instruction_decl.html#a660bb6c587100f0471f810614a68e91b",
"struct_d_s_l_1_1_parser_1_1_register_def_1_1_register_decl.html#ae012e73521fc38b35b6d1936175790fa",
"struct_d_s_l_1_1_parser_1_1_target_desc_1_1_libcalls_decl.html#ae09e86d5afee8417294ff7f1b1f6baad",
"struct_d_s_l_1_1_parser_1_1_target_inst_def_1_1_target_operand.html",
"struct_ez_mir_1_1_ast_1_1_mir_ast_operand.html#af61eb9cf60ef007868d8f4d6bae345f6",
"struct_liveness_result.html",
"struct_symbols_1_1_legalize_rule_operand_symbol.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';