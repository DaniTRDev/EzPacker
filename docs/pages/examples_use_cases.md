# Examples & Real-World Use Cases {#examples_use_cases}

This guide explores real-world compiler engineering use cases and provides fully working, copy-paste-ready, compilable C++ and MIR code examples.

---

## 🏛️ Real-World Architectural Use Cases

### 1. Ahead-of-Time (AOT) Language Backend
EzPacker serves as a complete, retargetable backend for custom programming language frontends (e.g., Python-like, Rust-like, or domain-specific languages). A frontend parses source text into an Abstract Syntax Tree (AST), generates EzPacker high-level MIR via `MirBuilderContext`, and delegates optimization, legalization, register allocation, and native object generation to `CompilationPipeline`. The output is a standard ELF64 or COFF object file that links seamlessly with the system C runtime.

### 2. In-Memory Just-in-Time (JIT) Engine
In runtime execution engines—such as analytical SQL query executors, regex engines, or dynamic language runtimes—compilation latency must be minimal. EzPacker's middle-end passes and target lowerers operate entirely in memory using fast Polymorphic Memory Resource (PMR) arenas. Instead of serializing to disk, an application can extract linearized machine code directly from `CodeSection` buffers, mark memory as executable (`VirtualProtect` or `mprotect`), and jump to function pointers with zero file I/O overhead.

### 3. Rapid Architecture Prototyping via EzDsl
When evaluating novel hardware architectures, custom RISC-V extensions, or vector coprocessors, writing a custom compiler backend by hand typically takes months. With EzPacker's declarative DSL suite (`.tdesc`, `.idf`, `.ezcc`, `.lad`, `.isf`), hardware architects can specify registers, instructions, calling conventions, and legalization rules in declarative syntax. EzPacker automatically generates C++ selector tables and legality verification code in minutes.

### 4. Binary Transformation, Hardening & Instrumentation
Security tooling, control-flow integrity (CFI) instrumentation, and binary packers require precise manipulation of low-level machine instructions, labels, and relocations. `EzCodeEmitter` provides a doubly-linked stream of `DataNode`, `LabelNode`, and `AlignNode` objects, allowing developers to inject security trampolines, rewrite call targets, and emit valid relocatable binaries without invoking full compiler passes.

---

## 💻 Working Code Examples

### Example 1: Programmatic IR Construction (Loop with SSA Phi Nodes)

This example demonstrates how to build an in-memory function that computes the sum of an array of 64-bit integers:
```c
int64_t sum_array(int64_t *arr, int64_t count);
```

```cpp
#include "Builder/MirBuilderContext.h"
#include "Builder/MirFunctionBuilder.h"
#include "Builder/MirBlockBuilder.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Type/MirTypeTable.h"
#include "Printer/MirPrinter.h"
#include <iostream>

void buildSumArrayFunction()
{
    // 1. Create the builder context owning the memory arena and symbol tables
    MirBuilderContext ctx;
    MirTypeTable &typeTable = ctx.getTypeTable();
    MirOperandBuilder opBuilder(&ctx);

    // 2. Resolve interned primitive types
    MirType *i64 = typeTable.getIntegerType(64, true);
    MirType *ptrI64 = typeTable.getPointerType(i64);

    // 3. Create module and function
    MirModule *module = ctx.createModule("ArrayModule");
    MirFunctionBuilder funcBuilder(&ctx, module);

    std::vector<MirType*> paramTypes = { ptrI64, i64 };
    MirFunction *func = funcBuilder.createFunction("sum_array", i64, paramTypes);

    // 4. Create basic blocks
    MirBlockBuilder blockBuilder(&ctx, func);
    MirBlock *entryBlock = blockBuilder.createBlock("entry");
    MirBlock *loopHeader = blockBuilder.createBlock("loop_header");
    MirBlock *loopBody   = blockBuilder.createBlock("loop_body");
    MirBlock *loopExit   = blockBuilder.createBlock("loop_exit");

    // Retrieve function parameter virtual registers
    MirOperand *arrParam   = func->getParameter(0); // ptr to int64_t
    MirOperand *countParam = func->getParameter(1); // count

    // --- Entry Block ---
    MirInstructionBuilder instBuilder(&ctx, entryBlock);
    MirOperand *zeroLit = opBuilder.createIntegerLiteral(0, 64);
    
    // Unconditional branch into loop header
    instBuilder.buildBranch(loopHeader);

    // --- Loop Header Block (SSA Phi Nodes) ---
    instBuilder.setTargetBlock(loopHeader);
    
    // Allocate SSA virtual registers for loop index and accumulator
    MirOperand *idxPhi = opBuilder.createVirtualRegister(i64);
    MirOperand *sumPhi = opBuilder.createVirtualRegister(i64);

    // Virtual registers for next iteration values
    MirOperand *nextIdx = opBuilder.createVirtualRegister(i64);
    MirOperand *nextSum = opBuilder.createVirtualRegister(i64);

    // Phi for index: takes 0 from entry, nextIdx from loop_body
    instBuilder.buildPhi(idxPhi, { {zeroLit, entryBlock}, {nextIdx, loopBody} });

    // Phi for accumulator: takes 0 from entry, nextSum from loop_body
    instBuilder.buildPhi(sumPhi, { {zeroLit, entryBlock}, {nextSum, loopBody} });

    // Compare index < count (signed less than)
    MirOperand *condReg = opBuilder.createVirtualRegister(typeTable.getIntegerType(1, false));
    instBuilder.buildIcmp(MirInstructionOpCode::ICMP_SLT, condReg, idxPhi, countParam);

    // Conditional branch: true -> loopBody, false -> loopExit
    instBuilder.buildCondBranch(condReg, loopBody, loopExit);

    // --- Loop Body Block ---
    instBuilder.setTargetBlock(loopBody);

    // Calculate element address: arr + idx * 8
    MirOperand *eightLit = opBuilder.createIntegerLiteral(8, 64);
    MirOperand *byteOffset = opBuilder.createVirtualRegister(i64);
    instBuilder.buildMul(byteOffset, idxPhi, eightLit);

    MirOperand *elemAddr = opBuilder.createVirtualRegister(ptrI64);
    instBuilder.buildAdd(elemAddr, arrParam, byteOffset);

    // Load value from array
    MirOperand *elemVal = opBuilder.createVirtualRegister(i64);
    instBuilder.buildLoad(elemVal, elemAddr);

    // Accumulate sum: nextSum = sumPhi + elemVal
    instBuilder.buildAdd(nextSum, sumPhi, elemVal);

    // Increment index: nextIdx = idxPhi + 1
    MirOperand *oneLit = opBuilder.createIntegerLiteral(1, 64);
    instBuilder.buildAdd(nextIdx, idxPhi, oneLit);

    // Jump back to loop header
    instBuilder.buildBranch(loopHeader);

    // --- Loop Exit Block ---
    instBuilder.setTargetBlock(loopExit);
    instBuilder.buildReturn(sumPhi);

    // 5. Serialize and dump textual MIR
    MirPrinter printer;
    printer.dumpModule(module, std::cout);
}
```

---

### Example 2: Authoring a Custom SSA Optimization Pass

This example demonstrates how to implement a custom middle-end peephole optimization pass that eliminates algebraic identities (`x + 0 => x`, `x * 1 => x`, `x ^ x => 0`):

```cpp
#include "Pass/MirPass.h"
#include "Function/MirFunction.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstruction.h"
#include "Operand/MirOperands.h"
#include <iostream>

class IdentityFoldingPass : public MirFunctionPass
{
public:
    const char *getName() const override { return "IdentityFoldingPass"; }

    bool runOnFunction(MirFunction *func) override
    {
        bool changed = false;

        // Iterate over all blocks in the function
        for (MirBlock &block : func->getBlocks())
        {
            auto it = block.begin();
            while (it != block.end())
            {
                MirInstruction *inst = &(*it);
                ++it; // Advance iterator before possible instruction removal

                if (tryFoldInstruction(inst))
                {
                    changed = true;
                }
            }
        }

        return changed;
    }

private:
    bool tryFoldInstruction(MirInstruction *inst)
    {
        MirInstructionOpCode op = inst->getOpCode();

        // Check if instruction defines a register
        if (inst->getOperandCount() < 3)
            return false;

        MirOperand *dest = inst->getOperand(0);
        MirOperand *lhs  = inst->getOperand(1);
        MirOperand *rhs  = inst->getOperand(2);

        // Fold: ADD dest, lhs, 0 => replace uses of dest with lhs
        if (op == MirInstructionOpCode::ADD)
        {
            if (isConstantZero(rhs))
            {
                dest->replaceAllUsesWith(lhs);
                inst->eraseFromParent();
                return true;
            }
            if (isConstantZero(lhs))
            {
                dest->replaceAllUsesWith(rhs);
                inst->eraseFromParent();
                return true;
            }
        }

        // Fold: XOR dest, lhs, rhs (where lhs == rhs) => replace uses of dest with 0
        if (op == MirInstructionOpCode::XOR && lhs->isIdenticalTo(rhs))
        {
            MirBuilderContext *ctx = inst->getParentBlock()->getContext();
            MirOperandBuilder opBuilder(ctx);
            MirOperand *zero = opBuilder.createIntegerLiteral(0, dest->getType()->getSizeInBits());
            dest->replaceAllUsesWith(zero);
            inst->eraseFromParent();
            return true;
        }

        return false;
    }

    bool isConstantZero(MirOperand *op)
    {
        if (auto *imm = dynamic_cast<MirInteger*>(op))
        {
            return imm->getValue().isZero();
        }
        return false;
    }
};
```

---

### Example 3: End-to-End Programmatic Compilation Pipeline

Compile an in-memory `MirModule` down to a native Windows COFF (`.obj`) or Linux ELF64 (`.o`) file directly from C++:

```cpp
#include "Compiler/CompilationPipeline.h"
#include "Compiler/DriverContext.h"
#include "Compiler/TargetResolver.h"
#include "Builder/MirBuilderContext.h"
#include "EzTargetsX86_64Registration.h"
#include <iostream>

bool compileModuleToDisk(MirModule *module, const std::string &tripleStr, const std::string &outputFile)
{
    // 1. Ensure target architecture factory is registered
    EzTargets::X86_64::registerTarget();

    // 2. Configure compilation options
    EzCompiler::CommandLineOptions opts;
    opts.m_targetTriple = tripleStr;
    opts.m_outputFile   = outputFile;
    opts.m_optLevel     = EzCompiler::OptimizationLevel::O2;
    opts.m_isPic        = true; // Generate position-independent code

    // 3. Initialize driver context
    EzCompiler::DriverContext driverCtx(opts);

    // 4. Resolve target architecture
    EzCompiler::ResolvedTarget target = EzCompiler::TargetResolver::resolve(
        driverCtx.getTriple(),
        driverCtx.getMirContext(),
        opts.m_isPic,
        opts.m_targetFeatures
    );

    if (!target.isValid())
    {
        std::cerr << "Error: Target " << tripleStr << " could not be resolved.\n";
        return false;
    }

    // 5. Instantiate and execute the multi-phase pipeline
    EzCompiler::CompilationPipeline pipeline(driverCtx, target);

    std::cout << "Executing Middle-End Optimization...\n";
    if (!pipeline.runMiddleEnd())
        return false;

    std::cout << "Executing Target Legalization...\n";
    if (!pipeline.runLegalization())
        return false;

    std::cout << "Executing Instruction Selection & Lowering...\n";
    if (!pipeline.runTargetLowering())
        return false;

    std::cout << "Emitting Native Object File: " << outputFile << "...\n";
    if (!pipeline.emitBinaryObject())
        return false;

    std::cout << "Compilation completed successfully!\n";
    return true;
}
```

---

### Example 4: Direct Binary Section & Relocation Emission with `EzCodeEmitter`

Assemble machine code bytes and emit standard relocations directly via `EzCodeEmitter` without going through the middle-end IR:

```cpp
#include "Section/CodeSection.h"
#include "Node/DataNode.h"
#include "Node/LabelNode.h"
#include "Node/AlignNode.h"
#include "ObjectWriter/Elf64Writer.h"
#include "Relocation/TargetCodeRelocationType.h"
#include <fstream>
#include <vector>

void emitCustomElfObject(const std::string &filename)
{
    // 1. Create a code section named .text
    CodeSection textSection(".text", SectionFlags::Alloc | SectionFlags::Exec);

    // 2. Append an alignment node (16-byte align)
    textSection.appendNode(std::make_unique<AlignNode>(16));

    // 3. Define a local label for function entry
    auto entryLabel = std::make_unique<LabelNode>("my_entry_function");
    LabelNode *pEntry = entryLabel.get();
    textSection.appendNode(std::move(entryLabel));

    // 4. Emit raw x86-64 machine instructions:
    //    push rbp        => 0x55
    //    mov rbp, rsp    => 0x48, 0x89, 0xE5
    std::vector<uint8_t> prologueBytes = { 0x55, 0x48, 0x89, 0xE5 };
    textSection.appendNode(std::make_unique<DataNode>(prologueBytes));

    // 5. Emit a call instruction with a 32-bit PC-relative relocation
    //    call <external_printf> => 0xE8, 0x00, 0x00, 0x00, 0x00
    std::vector<uint8_t> callBytes = { 0xE8, 0x00, 0x00, 0x00, 0x00 };
    auto callNode = std::make_unique<DataNode>(callBytes);

    // Attach external relocation fixup
    callNode->addRelocation(
        1, // Offset within node (displacement starts at byte 1)
        "printf", // External target symbol
        TargetCodeRelocationType::Branch32
    );
    textSection.appendNode(std::move(callNode));

    // 6. Emit epilogue:
    //    pop rbp  => 0x5D
    //    ret      => 0xC3
    std::vector<uint8_t> epilogueBytes = { 0x5D, 0xC3 };
    textSection.appendNode(std::make_unique<DataNode>(epilogueBytes));

    // 7. Finalize section layout (computes node offsets)
    textSection.finalize();

    // 8. Write to ELF64 relocatable file
    Elf64Writer writer;
    writer.addSection(&textSection);
    writer.writeToFile(filename);
}
```

---

### Example 5: Textual MIR File Syntax & Parsing

Below is a complete, valid textual `.mir` file demonstrating global variables, basic blocks, SSA virtual registers, memory operands, and conditional branching:

```mir
module "MathExample"

; 64-bit integer global variable
global @g_multiplier : i64 = 42

function @calculate_scaled_sum(i64 %a, i64 %b) -> i64 {
entry:
    ; Add %a and %b
    %sum = add i64 %a, %b

    ; Load @g_multiplier from memory
    %mult_addr = ref @g_multiplier
    %multiplier = load i64 [%mult_addr]

    ; Check if sum > 100
    %is_large = icmp sgt i64 %sum, 100
    br_cond %is_large, large_case, default_case

large_case:
    ; Multiply sum by multiplier
    %res_large = mul i64 %sum, %multiplier
    br exit(%res_large)

default_case:
    ; Just return the sum
    br exit(%sum)

exit(%result : i64):
    ret %result
}
```

#### Parsing Textual MIR in C++

```cpp
#include "Parser/MirParser.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostic/DiagnosticCollector.h"
#include "Diagnostic/DiagnosticListener.h"
#include <iostream>

void parseMirSource(const std::string &mirText)
{
    MirBuilderContext ctx;
    DiagnosticCollector &diags = ctx.getDiagnosticCollector();

    // Register a console logger listener
    ConsoleDiagnosticListener consoleListener;
    diags.addListener(&consoleListener);

    MirParser parser(&ctx);
    MirModule *module = parser.parseString(mirText);

    if (!module || diags.hasErrors())
    {
        std::cerr << "Failed to parse MIR text.\n";
        return;
    }

    std::cout << "Successfully parsed module: " << module->getName() << "\n";
    std::cout << "Function count: " << module->getFunctions().size() << "\n";
}
```

---

> [!TIP]
> Ready to port EzPacker to a custom architecture? Read the **[Adding a Target Architecture Guide](adding_a_target.html)** next!
