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
      [ "2. Core Abstractions &amp; Classes", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md127", [
        [ "2.1 <span class=\"tt\">GenericCodeEmitter</span> (<span class=\"tt\">include/GenericCodeEmitter.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md128", null ],
        [ "2.2 <span class=\"tt\">CodeEmitterContext</span> (<span class=\"tt\">include/CodeEmitterContext.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md129", null ],
        [ "2.3 <span class=\"tt\">CodeSection</span> (<span class=\"tt\">include/CodeSection.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md130", null ],
        [ "2.4 <span class=\"tt\">ObjectSymbol</span> (<span class=\"tt\">include/ObjectFormat/ObjectSymbol.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md131", null ],
        [ "2.5 <span class=\"tt\">ObjectRelocEntry</span> (<span class=\"tt\">include/ObjectFormat/ObjectSymbol.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md132", null ]
      ] ],
      [ "3. Object File Writers (<span class=\"tt\">IObjectWriter</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md134", [
        [ "3.1 <span class=\"tt\">Elf64Writer</span> (<span class=\"tt\">include/ObjectFormat/Elf64Writer.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md135", null ],
        [ "3.2 <span class=\"tt\">CoffWriter</span> (<span class=\"tt\">include/ObjectFormat/CoffWriter.h</span>)", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md136", null ]
      ] ],
      [ "4. Usage Example: Serializing an Object File", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md138", null ],
      [ "5. API Reference &amp; Further Reading", "md_docs_2projects_2_ez_code_emitter.html#autotoc_md140", null ]
    ] ],
    [ "EzCompiler Subproject Documentation", "md_docs_2projects_2_ez_compiler.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_compiler.html#autotoc_md143", null ],
      [ "2. Core Components", "md_docs_2projects_2_ez_compiler.html#autotoc_md145", [
        [ "2.1 <span class=\"tt\">CommandLineOptions</span> &amp; <span class=\"tt\">CommandLineParser</span> (<span class=\"tt\">include/CommandLineOptions.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md146", [
          [ "Key Configuration Fields:", "md_docs_2projects_2_ez_compiler.html#autotoc_md147", null ]
        ] ],
        [ "2.2 <span class=\"tt\">DriverContext</span> (<span class=\"tt\">include/DriverContext.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md149", null ],
        [ "2.3 <span class=\"tt\">TargetTriple</span> (<span class=\"tt\">include/TargetTriple.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md151", null ],
        [ "2.4 <span class=\"tt\">TargetResolver</span> (<span class=\"tt\">include/TargetResolver.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md153", null ],
        [ "2.5 <span class=\"tt\">CompilationPipeline</span> (<span class=\"tt\">include/CompilationPipeline.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md155", null ],
        [ "2.6 <span class=\"tt\">EmissionEngine</span> (<span class=\"tt\">include/EmissionEngine.h</span>)", "md_docs_2projects_2_ez_compiler.html#autotoc_md157", null ]
      ] ],
      [ "3. CLI Command-Line Reference", "md_docs_2projects_2_ez_compiler.html#autotoc_md159", null ],
      [ "4. API Reference &amp; Further Reading", "md_docs_2projects_2_ez_compiler.html#autotoc_md161", null ]
    ] ],
    [ "EzCore Subproject Documentation", "md_docs_2projects_2_ez_core.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_core.html#autotoc_md164", [
        [ "Key Responsibilities", "md_docs_2projects_2_ez_core.html#autotoc_md165", null ]
      ] ],
      [ "2. Polymorphic Memory Architecture (PMR)", "md_docs_2projects_2_ez_core.html#autotoc_md167", [
        [ "Core PMR Guidelines in EzPacker", "md_docs_2projects_2_ez_core.html#autotoc_md168", null ]
      ] ],
      [ "3. Diagnostic Engine", "md_docs_2projects_2_ez_core.html#autotoc_md170", [
        [ "3.1 Diagnostic Message Types &amp; Locations", "md_docs_2projects_2_ez_core.html#autotoc_md171", null ],
        [ "3.2 Key Classes", "md_docs_2projects_2_ez_core.html#autotoc_md172", null ],
        [ "3.3 Example: Emitting a Diagnostic", "md_docs_2projects_2_ez_core.html#autotoc_md173", null ]
      ] ],
      [ "4. Arbitrary-Precision Constants: <span class=\"tt\">FlexInt</span> &amp; <span class=\"tt\">FlexFloat</span>", "md_docs_2projects_2_ez_core.html#autotoc_md175", [
        [ "4.1 <span class=\"tt\">FlexInt</span> (<span class=\"tt\">FlexNumber/FlexInt.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md176", null ],
        [ "4.2 <span class=\"tt\">FlexFloat</span> (<span class=\"tt\">FlexNumber/FlexFloat.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md177", null ]
      ] ],
      [ "5. Specialized Compiler Data Structures", "md_docs_2projects_2_ez_core.html#autotoc_md179", [
        [ "5.1 <span class=\"tt\">DenseBitSet</span> (<span class=\"tt\">HelperClasses/DenseBitSet.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md180", null ],
        [ "5.2 <span class=\"tt\">IntrusiveLinkedList&lt;T&gt;</span> (<span class=\"tt\">HelperClasses/IntrusiveLinkedList.h</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md181", null ]
      ] ],
      [ "6. Source Management (<span class=\"tt\">SourceManager</span>)", "md_docs_2projects_2_ez_core.html#autotoc_md183", null ],
      [ "7. Additional Utilities", "md_docs_2projects_2_ez_core.html#autotoc_md185", null ],
      [ "8. API Reference &amp; Further Reading", "md_docs_2projects_2_ez_core.html#autotoc_md187", null ]
    ] ],
    [ "EzDsl Subproject Documentation", "md_docs_2projects_2_ez_dsl.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_dsl.html#autotoc_md190", null ],
      [ "2. Subproject Structure", "md_docs_2projects_2_ez_dsl.html#autotoc_md192", null ],
      [ "3. The 8 DSL Languages", "md_docs_2projects_2_ez_dsl.html#autotoc_md194", [
        [ "3.1 Type Definition Language (<span class=\"tt\">.tyf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md195", null ],
        [ "3.2 IR Instruction Definition Language (<span class=\"tt\">.irdf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md196", null ],
        [ "3.3 Target Descriptor Language (<span class=\"tt\">.tdesc</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md197", null ],
        [ "3.4 Calling Convention Language (<span class=\"tt\">.ezcc</span>, <span class=\"tt\">.ccd</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md198", null ],
        [ "3.5 Legalization Action Language (<span class=\"tt\">.lad</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md199", null ],
        [ "3.6 Legalization Rule Language (<span class=\"tt\">.lrd</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md200", null ],
        [ "3.7 Target Instruction Definition Language (<span class=\"tt\">.idf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md201", null ],
        [ "3.8 Instruction Selection Pattern Language (<span class=\"tt\">.isf</span>)", "md_docs_2projects_2_ez_dsl.html#autotoc_md202", null ]
      ] ],
      [ "4. Code Generators", "md_docs_2projects_2_ez_dsl.html#autotoc_md204", null ],
      [ "5. CLI Tool: <span class=\"tt\">ezdsl_cli</span>", "md_docs_2projects_2_ez_dsl.html#autotoc_md206", [
        [ "Command-Line Arguments", "md_docs_2projects_2_ez_dsl.html#autotoc_md207", null ],
        [ "Example Usage", "md_docs_2projects_2_ez_dsl.html#autotoc_md208", null ]
      ] ],
      [ "6. API Reference &amp; Further Reading", "md_docs_2projects_2_ez_dsl.html#autotoc_md210", null ]
    ] ],
    [ "EzMir Subproject Documentation", "md_docs_2projects_2_ez_mir.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_mir.html#autotoc_md213", null ],
      [ "2. In-Memory Intermediate Representation (IR)", "md_docs_2projects_2_ez_mir.html#autotoc_md215", [
        [ "2.1 <span class=\"tt\">MirFunction</span> (<span class=\"tt\">Function/MirFunction.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md216", null ],
        [ "2.2 <span class=\"tt\">MirBlock</span> (<span class=\"tt\">Block/MirBlock.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md217", null ],
        [ "2.3 <span class=\"tt\">MirInstruction</span> (<span class=\"tt\">Instruction/MirInstruction.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md218", null ],
        [ "2.4 <span class=\"tt\">MirOperand</span> (<span class=\"tt\">Operand/MirOperand.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md219", null ],
        [ "2.5 <span class=\"tt\">MirType</span> (<span class=\"tt\">Type/MirType.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md220", null ]
      ] ],
      [ "3. Middle-End Pass Framework", "md_docs_2projects_2_ez_mir.html#autotoc_md222", [
        [ "3.1 <span class=\"tt\">CodeFlowAnalysisPass</span> (<span class=\"tt\">MirPasses/Passes/CodeFlowAnalysisPass.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md223", null ],
        [ "3.2 <span class=\"tt\">NonSsaToSsaPass</span> (<span class=\"tt\">MirPasses/Passes/NonSsaToSsaPass.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md224", null ],
        [ "3.3 <span class=\"tt\">LivenessAnalysisPass</span> (<span class=\"tt\">MirPasses/Passes/LivenessAnalysisPass.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md225", null ]
      ] ],
      [ "4. Programmatic MIR Construction (Builder API)", "md_docs_2projects_2_ez_mir.html#autotoc_md227", null ],
      [ "5. Textual MIR: Parser &amp; Printer", "md_docs_2projects_2_ez_mir.html#autotoc_md229", [
        [ "5.1 Textual Format Syntax", "md_docs_2projects_2_ez_mir.html#autotoc_md230", null ],
        [ "5.2 <span class=\"tt\">MirLexer</span> &amp; <span class=\"tt\">MirParser</span> (<span class=\"tt\">Parser/MirLexer.h</span>, <span class=\"tt\">Parser/MirParser.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md231", null ],
        [ "5.3 <span class=\"tt\">MirPrinter</span> (<span class=\"tt\">Printer/MirPrinter.h</span>)", "md_docs_2projects_2_ez_mir.html#autotoc_md232", null ]
      ] ],
      [ "6. API Reference &amp; Further Reading", "md_docs_2projects_2_ez_mir.html#autotoc_md234", null ]
    ] ],
    [ "EzTargets Subproject Documentation", "md_docs_2projects_2_ez_targets.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_targets.html#autotoc_md237", null ],
      [ "2. The x86-64 Target (<span class=\"tt\">EzTargets/X86_64</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md239", [
        [ "2.1 Target Descriptor: <span class=\"tt\">X86_64TargetDesc</span> (<span class=\"tt\">include/X86_64TargetDesc.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md240", null ],
        [ "2.2 Calling Conventions", "md_docs_2projects_2_ez_targets.html#autotoc_md242", [
          [ "System V AMD64 (Linux / macOS / BSD)", "md_docs_2projects_2_ez_targets.html#autotoc_md243", null ],
          [ "Microsoft Win64 (Windows)", "md_docs_2projects_2_ez_targets.html#autotoc_md244", null ]
        ] ],
        [ "2.3 Frame Lowering: <span class=\"tt\">X86_64FrameLowerer</span> (<span class=\"tt\">include/X86_64FrameLowerer.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md246", null ],
        [ "2.4 Instruction Selection: <span class=\"tt\">X86_64TargetInstructionSelector</span>", "md_docs_2projects_2_ez_targets.html#autotoc_md248", null ],
        [ "2.5 Machine Code Emission &amp; Encoding", "md_docs_2projects_2_ez_targets.html#autotoc_md250", [
          [ "<span class=\"tt\">X86_64InstructionEncoder</span> (<span class=\"tt\">include/Encoding/X86_64InstructionEncoder.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md251", null ],
          [ "<span class=\"tt\">BranchRelaxer</span> (<span class=\"tt\">include/BranchRelaxation/BranchRelaxer.h</span>)", "md_docs_2projects_2_ez_targets.html#autotoc_md252", null ]
        ] ],
        [ "2.6 Binary Descriptors &amp; Relocations", "md_docs_2projects_2_ez_targets.html#autotoc_md254", null ],
        [ "2.7 Build-Time Plugin: <span class=\"tt\">EzTargetsX86_64Dsl</span>", "md_docs_2projects_2_ez_targets.html#autotoc_md256", null ],
        [ "2.8 Target Registration", "md_docs_2projects_2_ez_targets.html#autotoc_md258", null ]
      ] ],
      [ "3. API Reference &amp; Further Reading", "md_docs_2projects_2_ez_targets.html#autotoc_md260", null ]
    ] ],
    [ "EzTriple Subproject Documentation", "md_docs_2projects_2_ez_triple.html", [
      [ "1. Overview &amp; Architectural Role", "md_docs_2projects_2_ez_triple.html#autotoc_md263", null ],
      [ "2. The 5 Core Subsystems", "md_docs_2projects_2_ez_triple.html#autotoc_md265", [
        [ "2.1 The Legalizer (<span class=\"tt\">include/Legalizer/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md266", [
          [ "3-Tier Architecture (<span class=\"tt\">LegalizerInfo.h</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md267", null ],
          [ "Built-In Action Handlers (<span class=\"tt\">include/Legalizer/Actions/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md268", null ]
        ] ],
        [ "2.2 ABI Lowerer (<span class=\"tt\">include/AbiLowerer/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md270", null ],
        [ "2.3 Instruction Selector (<span class=\"tt\">include/InstructionSelector/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md272", null ],
        [ "2.4 Register Allocator (<span class=\"tt\">include/RegisterAllocator/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md274", null ],
        [ "2.5 Frame Lowerer (<span class=\"tt\">include/FrameLowerer/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md276", null ]
      ] ],
      [ "3. Architecture Descriptors (<span class=\"tt\">include/Descriptors/</span>)", "md_docs_2projects_2_ez_triple.html#autotoc_md278", null ],
      [ "4. API Reference &amp; Further Reading", "md_docs_2projects_2_ez_triple.html#autotoc_md280", null ]
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
"md_docs_2projects_2_ez_mir.html#autotoc_md217",
"namespace_ez_mir_1_1_ast.html#a68a384eb3c60d7842eced1de13228f21a7413d5aedc19aa4f37d774f8365cfc7e",
"struct_cli_1_1_driver_result.html#a9bb992bb1456c329e9b2a8c641b50e6b",
"struct_d_s_l_1_1_ast_1_1_common_1_1_sourced_ast_node.html#ad604f3f6810a456867ff00c2507091b0",
"struct_d_s_l_1_1_ast_1_1_legalize_rule_def_1_1_rule_instruction_operand.html#aa0a0b2e68f239cbee033fa1d17870d0b",
"struct_d_s_l_1_1_parser_1_1_calling_conv_def_1_1_aggregate_condition_parser_1_1_size_cond_1_1_le_branch.html#ad52fe31b87c74e272438d0ea3332b9e2",
"struct_d_s_l_1_1_parser_1_1_calling_conv_def_1_1_pass_rule_parser_1_1_pass_source_spec.html",
"struct_d_s_l_1_1_parser_1_1_calling_conv_def_1_1_stack_cleanup_rule.html",
"struct_d_s_l_1_1_parser_1_1_common_1_1_keyword.html",
"struct_d_s_l_1_1_parser_1_1_instruction_select_def_1_1_file_item.html#a95d778694748f6c21ff3e3781ecbe121",
"struct_d_s_l_1_1_parser_1_1_ir_inst_def_1_1_body_item_1_1_category_decl.html#a0a01fff65439244a721a92229e81de67",
"struct_d_s_l_1_1_parser_1_1_legalize_action_def_1_1_type_constraint.html#a194375ead3b12c1585f078ba9c9023e9",
"struct_d_s_l_1_1_parser_1_1_register_def_1_1_register_name_binding.html#acfc5ed352a27609a0bedcbdce1fd8cbb",
"struct_d_s_l_1_1_parser_1_1_target_desc_1_1_mem_disp_type_decl.html#a84c7ca891e268be1946712c75c3fb873",
"struct_d_s_l_1_1_parser_1_1_type_def_1_1_type_def_file.html#af8c73bd78fd665767ce7f53dd1d2ba26",
"struct_ez_mir_1_1_ast_1_1_mir_ast_param.html#aad861412ab9093481df5b7d408778a6a",
"struct_mir_function_analysis_data.html#a9ecd21bbd6c6e026497401e25af90f73",
"struct_symbols_1_1_legalize_rule_predicate_symbol.html#a2402cdc62e11d783b8d25fa6eaf14c75"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';