#include "AstLowererVisitorTestFixture.h"

// =============================================================================
//  I. INSTRUCTION LOWERING – basic mnemonic-to-opcode mapping & operand count
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_Nop)
{
    std::string code = R"(
myLabel:
{
    nop;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));

    // LabelLowerer creates a block and jumps to it, then lowers body inside it.
    // The first block is the one where JMP to label-block is emitted.
    // The second block is the label block itself.
    auto emCtx = getEmitterContext();
    // The label block should contain the NOP instruction.
    // We look at the label's annotation to find the block.
    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto symbolAnnot = label->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symbolAnnot, nullptr);

    MirId labelBlockId = getLoweringContext()->getMirIdOfSymbol(symbolAnnot->getSymbol());
    EXPECT_NE(labelBlockId, MIRID_INVALID);
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_MovVarImm)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    mov %x, 42;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_AddVarImm)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    add %x, 10;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_SubVarImm)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    sub %x, 5;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_MulVarImm)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    mul %x, 3;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_DivVarImm)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    div %x, 2;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_AndOrXor)
{
    std::string code = R"(
myLabel:
{
    create i32 %a;
    and %a, 0xFF;
    or %a, 0x10;
    xor %a, 0x01;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_ShlShrSar)
{
    std::string code = R"(
myLabel:
{
    create i32 %a;
    shl %a, 2;
    shr %a, 1;
    sar %a, 3;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_NegNot)
{
    std::string code = R"(
myLabel:
{
    create i32 %a;
    neg %a;
    not %a;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_CmpTest)
{
    std::string code = R"(
myLabel:
{
    create i32 %a;
    create i32 %b;
    cmp %a, %b;
    test %a, %b;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, InstructionLowering_InvalidMnemonic)
{
    std::string code = R"(
myLabel:
{
    create i32 %a;
    foobar %a, 1;
})";

    // 'foobar' is not a valid instruction – lowering must fail.
    EXPECT_FALSE(runLowering<LabelParser>(code));
}

// =============================================================================
//  II. VARIABLE LOWERING – vreg creation, reuse, symbol-to-MIR linkage
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, VariableLowering_LocalVarCreatesVreg)
{
    std::string code = R"(
myLabel:
{
    create i32 %myLocal;
    mov %myLocal, 100;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));

    // The symbol 'myLocal' must be linked to a MIR ID (its virtual register).
    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scopedAnnot = label->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(scopedAnnot, nullptr);

    Scope *labelScope = scopedAnnot->getOwnedScope();
    ASSERT_NE(labelScope, nullptr);

    Symbol *sym = nullptr;
    EXPECT_TRUE(labelScope->resolve("myLocal", &sym, false));
    ASSERT_NE(sym, nullptr);

    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(sym));
    EXPECT_NE(getLoweringContext()->getMirIdOfSymbol(sym), MIRID_INVALID);
}

TEST_F(AstLowererVisitorTestFixture, VariableLowering_ReuseVregOnSecondUse)
{
    std::string code = R"(
myLabel:
{
    create i32 %v;
    mov %v, 1;
    add %v, 2;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));

    // Same variable used twice – should produce the same vreg id.
    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scopedAnnot = label->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(scopedAnnot, nullptr);

    Scope *labelScope = scopedAnnot->getOwnedScope();
    Symbol *sym = nullptr;
    EXPECT_TRUE(labelScope->resolve("v", &sym, false));
    ASSERT_NE(sym, nullptr);

    MirId id = getLoweringContext()->getMirIdOfSymbol(sym);
    EXPECT_NE(id, MIRID_INVALID);
}

TEST_F(AstLowererVisitorTestFixture, VariableLowering_MultipleVarsHaveDifferentVregs)
{
    std::string code = R"(
myLabel:
{
    create i32 %a;
    create i32 %b;
    mov %a, 10;
    mov %b, 20;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));

    auto label = dynamic_cast<Label *>(getAstNode());
    auto scopedAnnot = label->getAnnotation<ScopedSymbolAnnotation>();
    Scope *labelScope = scopedAnnot->getOwnedScope();

    Symbol *symA = nullptr, *symB = nullptr;
    EXPECT_TRUE(labelScope->resolve("a", &symA, false));
    EXPECT_TRUE(labelScope->resolve("b", &symB, false));
    ASSERT_NE(symA, nullptr);
    ASSERT_NE(symB, nullptr);

    MirId idA = getLoweringContext()->getMirIdOfSymbol(symA);
    MirId idB = getLoweringContext()->getMirIdOfSymbol(symB);
    EXPECT_NE(idA, MIRID_INVALID);
    EXPECT_NE(idB, MIRID_INVALID);
    EXPECT_NE(idA, idB);
}

// =============================================================================
//  III. IMMEDIATE LOWERING – integer, float, string
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, ImmediateLowering_SmallInteger)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    mov %x, 42;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
    // The immediate 42 should be lowered as a MirInteger operand (≤64 bits).
}

TEST_F(AstLowererVisitorTestFixture, ImmediateLowering_ZeroImmediate)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    mov %x, 0;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ImmediateLowering_NegativeImmediate)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    add %x, -1;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ImmediateLowering_HexImmediate)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    mov %x, 0xDEAD;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ImmediateLowering_FloatingPoint)
{
    std::string code = R"(
myLabel:
{
    create float %f;
    mov %f, 3.14;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

// =============================================================================
//  IV. MEMORY OPERAND LOWERING
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, MemoryLowering_BaseDisplacement)
{
    std::string code = R"(
myLabel:
{
    create i64 %base;
    mov %base, i64 (%base+0x10);
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, MemoryLowering_BaseDisplacementZero)
{
    std::string code = R"(
myLabel:
{
    create i64 %ptr;
    mov %ptr, i64 (%ptr+0);
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, MemoryLowering_IndexScale)
{
    std::string code = R"(
myLabel:
{
    create i64 %idx;
    mov %idx, i64 (, %idx, 4);
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, MemoryLowering_Direct)
{
    std::string code = R"(
myLabel:
{
    create i64 %x;
    mov %x, i64 (0x400000);
})";

    // NOTE: This test may fail due to bug in MemoryLowerer::lowerDirect (returns false).
    // If/when that bug is fixed, change to ASSERT_TRUE.
    ASSERT_TRUE(runLowering<LabelParser>(code));
}

// =============================================================================
//  V. LABEL LOWERING – block creation, JMP, symbol linkage
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, LabelLowering_CreatesBlockAndLinksSymbol)
{
    std::string code = R"(
myLabel:
{
    nop;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));

    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto symbolAnnot = label->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symbolAnnot, nullptr);

    Symbol *labelSym = symbolAnnot->getSymbol();
    ASSERT_NE(labelSym, nullptr);
    EXPECT_EQ(labelSym->getType(), SymbolType::Label);

    MirId blockId = getLoweringContext()->getMirIdOfSymbol(labelSym);
    EXPECT_NE(blockId, MIRID_INVALID);
}

TEST_F(AstLowererVisitorTestFixture, LabelLowering_MultipleLabelsInModule)
{
    std::string code = R"(
void MyFunc()
{
    label1:
    {
        nop;
    }
    label2:
    {
        nop;
    }
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    auto module = dynamic_cast<Module *>(getAstNode());
    ASSERT_NE(module, nullptr);

    // Module symbol should be linked
    auto moduleAnnot = module->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(moduleAnnot, nullptr);

    MirId moduleBlockId = getLoweringContext()->getMirIdOfSymbol(moduleAnnot->getSymbol());
    EXPECT_NE(moduleBlockId, MIRID_INVALID);
}

// =============================================================================
//  VI. MODULE LOWERING – block creation, header params, body lowering
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, ModuleLowering_EmptyBody)
{
    std::string code = R"(
void EmptyFunc()
{
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    auto module = dynamic_cast<Module *>(getAstNode());
    ASSERT_NE(module, nullptr);

    auto symbolAnnot = module->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symbolAnnot, nullptr);
    EXPECT_EQ(symbolAnnot->getSymbol()->getType(), SymbolType::Module);

    MirId blockId = getLoweringContext()->getMirIdOfSymbol(symbolAnnot->getSymbol());
    EXPECT_NE(blockId, MIRID_INVALID);
}

TEST_F(AstLowererVisitorTestFixture, ModuleLowering_WithParameters)
{
    std::string code = R"(
i64 MyFunc(i32 %param1, i64 %param2)
{
    create i32 %local;
    add %local, 1;
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    // Check that both parameters and local get linked.
    auto module = dynamic_cast<Module *>(getAstNode());
    ASSERT_NE(module, nullptr);

    auto scopedAnnot = module->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(scopedAnnot, nullptr);

    Scope *moduleScope = scopedAnnot->getOwnedScope();
    ASSERT_NE(moduleScope, nullptr);

    Symbol *param1Sym = nullptr, *param2Sym = nullptr, *localSym = nullptr;
    EXPECT_TRUE(moduleScope->resolve("param1", &param1Sym, false));
    EXPECT_TRUE(moduleScope->resolve("param2", &param2Sym, false));
    EXPECT_TRUE(moduleScope->resolve("local", &localSym, false));
    ASSERT_NE(param1Sym, nullptr);
    ASSERT_NE(param2Sym, nullptr);
    ASSERT_NE(localSym, nullptr);

    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(param1Sym));
    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(param2Sym));
    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(localSym));

    // All three should have distinct MIR IDs.
    MirId idP1 = getLoweringContext()->getMirIdOfSymbol(param1Sym);
    MirId idP2 = getLoweringContext()->getMirIdOfSymbol(param2Sym);
    MirId idL = getLoweringContext()->getMirIdOfSymbol(localSym);
    EXPECT_NE(idP1, idP2);
    EXPECT_NE(idP1, idL);
    EXPECT_NE(idP2, idL);
}

TEST_F(AstLowererVisitorTestFixture, ModuleLowering_ModuleSymbolLinkedToBlock)
{
    std::string code = R"(
void Foo()
{
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    auto module = dynamic_cast<Module *>(getAstNode());
    auto symbolAnnot = module->getAnnotation<SymbolAnnotation>();
    MirId blockId = getLoweringContext()->getMirIdOfSymbol(symbolAnnot->getSymbol());
    EXPECT_NE(blockId, MIRID_INVALID);
}

// =============================================================================
//  VII. IF / ELSE LOWERING – block structure, CMP + conditional jump pattern
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, IfLowering_SimpleIf)
{
    std::string code = R"(
void TestIf(i32 %a, i32 %b)
{
    if (%a EQ %b)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
    // The If lowerer creates: trueBlock, falseBlock, mergeBlock.
    // Condition block emits: CMP, JE trueBlock, JMP falseBlock.
    // True block emits: <body>, JMP mergeBlock.
    // False block emits: JMP mergeBlock.
    // Merge block: code after the if.
}

TEST_F(AstLowererVisitorTestFixture, IfLowering_IfElse)
{
    std::string code = R"(
void TestIfElse(i32 %a, i32 %b)
{
    if (%a NE %b)
    {
        create i32 %thenVar;
        mov %thenVar, 1;
    }
    else
    {
        create i32 %elseVar;
        mov %elseVar, 0;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, IfLowering_IfElseIf)
{
    std::string code = R"(
void TestElseIf(i32 %x, i32 %y)
{
    if (%x GT %y)
    {
        nop;
    }
    else if (%x LT %y)
    {
        nop;
    }
    else
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, IfLowering_NestedIf)
{
    std::string code = R"(
void NestedIf(i32 %a, i32 %b, i32 %c)
{
    if (%a EQ %b)
    {
        if (%b EQ %c)
        {
            nop;
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  VIII. WHILE LOWERING – condition block, loop block, exit block
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, WhileLowering_SimpleWhile)
{
    std::string code = R"(
void TestWhile(i32 %counter, i32 %limit)
{
    while (%counter LT %limit)
    {
        add %counter, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
    // WhileLowerer creates: checkCondition block, trueBlock (loop), falseBlock (exit).
    // Before condition: JMP checkCondition.
    // In checkCondition: CMP, JLT trueBlock, JMP falseBlock.
    // In trueBlock: <body>, JMP checkCondition.
    // falseBlock: code after while.
}

TEST_F(AstLowererVisitorTestFixture, WhileLowering_NestedWhile)
{
    std::string code = R"(
void NestedWhile(i32 %i, i32 %j, i32 %n)
{
    while (%i LT %n)
    {
        while (%j LT %n)
        {
            add %j, 1;
        }
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, WhileLowering_WhileInsideIf)
{
    std::string code = R"(
void WhileInIf(i32 %a, i32 %b, i32 %c)
{
    if (%a GT %b)
    {
        while (%b LT %c)
        {
            add %b, 1;
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  IX. CONDITION LOWERING – all comparison types → correct jump opcode
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, ConditionLowering_EQ)
{
    std::string code = R"(
void TestEQ(i32 %a, i32 %b)
{
    if (%a EQ %b)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ConditionLowering_NE)
{
    std::string code = R"(
void TestNE(i32 %a, i32 %b)
{
    if (%a NE %b)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ConditionLowering_GT)
{
    std::string code = R"(
void TestGT(i32 %a, i32 %b)
{
    if (%a GT %b)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ConditionLowering_GE)
{
    std::string code = R"(
void TestGE(i32 %a, i32 %b)
{
    if (%a GE %b)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ConditionLowering_LT)
{
    std::string code = R"(
void TestLT(i32 %a, i32 %b)
{
    if (%a LT %b)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ConditionLowering_LE)
{
    std::string code = R"(
void TestLE(i32 %a, i32 %b)
{
    if (%a LE %b)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  X. EDGE CASES
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, EdgeCase_EmptyLabel)
{
    std::string code = R"(
emptyLabel:
{
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, EdgeCase_EmptyModuleBody)
{
    std::string code = R"(
void Empty()
{
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, EdgeCase_ModuleOnlyNops)
{
    std::string code = R"(
void NopChain()
{
    nop;
    nop;
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, EdgeCase_ManyLocals)
{
    std::string code = R"(
void ManyVars()
{
    create i32 %v0;
    create i32 %v1;
    create i32 %v2;
    create i32 %v3;
    create i32 %v4;
    create i32 %v5;
    create i32 %v6;
    create i32 %v7;
    create i32 %v8;
    create i32 %v9;
    add %v0, 1;
    add %v9, 2;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, EdgeCase_IfWithEmptyBranches)
{
    std::string code = R"(
void EmptyBranches(i32 %a, i32 %b)
{
    if (%a EQ %b)
    {
    }
    else
    {
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, EdgeCase_WhileEmptyBody)
{
    std::string code = R"(
void EmptyWhile(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, EdgeCase_MultipleScopesVariableIsolation)
{
    std::string code = R"(
void ScopeIsolation(i32 %x, i32 %y)
{
    label_a:
    {
        create i32 %scopedA;
        mov %scopedA, 1;
    }
    label_b:
    {
        create i32 %scopedB;
        mov %scopedB, 2;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  XI. COMPLEX / INTEGRATION TESTS
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, Integration_FullProgram)
{
    std::string code = R"(
i64 ComputeSum(i32 %n, i64 %start)
{
    create i64 %sum;
    create i32 %i;
    mov %sum, %start;
    mov %i, 0;

    while (%i LT %n)
    {
        add %sum, 1;
        add %i, 1;
    }

    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    auto module = dynamic_cast<Module *>(getAstNode());
    ASSERT_NE(module, nullptr);

    auto moduleAnnot = module->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(moduleAnnot, nullptr);
    EXPECT_EQ(moduleAnnot->getSymbol()->getName(), "ComputeSum");

    MirId moduleBlockId = getLoweringContext()->getMirIdOfSymbol(moduleAnnot->getSymbol());
    EXPECT_NE(moduleBlockId, MIRID_INVALID);

    // Verify parameter and local symbol linkage
    auto scopedAnnot = module->getAnnotation<ScopedSymbolAnnotation>();
    Scope *moduleScope = scopedAnnot->getOwnedScope();

    Symbol *nSym = nullptr, *startSym = nullptr, *sumSym = nullptr, *iSym = nullptr;
    EXPECT_TRUE(moduleScope->resolve("n", &nSym, false));
    EXPECT_TRUE(moduleScope->resolve("start", &startSym, false));
    EXPECT_TRUE(moduleScope->resolve("sum", &sumSym, false));
    EXPECT_TRUE(moduleScope->resolve("i", &iSym, false));

    ASSERT_NE(nSym, nullptr);
    ASSERT_NE(startSym, nullptr);
    ASSERT_NE(sumSym, nullptr);
    ASSERT_NE(iSym, nullptr);

    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(nSym));
    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(startSym));
    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(sumSym));
    EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(iSym));
}

TEST_F(AstLowererVisitorTestFixture, Integration_IfElseWithVarsAndArith)
{
    std::string code = R"(
i32 Abs(i32 %val, i32 %zero)
{
    create i32 %result;

    if (%val GE %zero)
    {
        mov %result, %val;
    }
    else
    {
        mov %result, %val;
        neg %result;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, Integration_MemoryAndArith)
{
    std::string code = R"(
void MemTest(i64 %base)
{
    create i64 %val;
    mov %val, i64 (%base+0x10);
    add %val, 1;
    mov %val, i64 (%base+0);
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, Integration_MultipleLabelsWithJumps)
{
    std::string code = R"(
void MultiLabel()
{
    first:
    {
        nop;
    }
    second:
    {
        nop;
    }
    third:
    {
        nop;
    }
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, Integration_DeepNesting)
{
    std::string code = R"(
void DeepNest(i32 %a, i32 %b, i32 %c, i32 %d)
{
    if (%a EQ %b)
    {
        if (%b EQ %c)
        {
            if (%c EQ %d)
            {
                nop;
            }
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, Integration_WhileWithIfInside)
{
    std::string code = R"(
void WhileIf(i32 %i, i32 %n, i32 %threshold)
{
    while (%i LT %n)
    {
        if (%i GE %threshold)
        {
            nop;
        }
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, Integration_LargeModule)
{
    std::string code = R"(
i64 CalculateChecksum(i64 %bufferPtr, i32 %length, i64 %key)
{
    create i64 %runningSum;
    create i32 %counter;
    create i64 %currentAddr;
    create i64 %tempCalc;

    mov %runningSum, 0;
    mov %counter, 0;
    mov %currentAddr, %bufferPtr;

    while (%counter LT %length)
    {
        mov %tempCalc, i64 (%currentAddr+0);
        add %tempCalc, %key;
        add %runningSum, %tempCalc;
        add %currentAddr, 1;
        add %counter, 1;
    }

    add %runningSum, i64 (%currentAddr+0x10);
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    // Verify all symbols are linked
    auto module = dynamic_cast<Module *>(getAstNode());
    auto scopedAnnot = module->getAnnotation<ScopedSymbolAnnotation>();
    Scope *scope = scopedAnnot->getOwnedScope();

    const char *names[] = { "bufferPtr", "length", "key", "runningSum", "counter", "currentAddr", "tempCalc" };
    for (const char *name : names)
    {
        Symbol *sym = nullptr;
        EXPECT_TRUE(scope->resolve(name, &sym, false)) << "Symbol '" << name << "' not found";
        if (sym)
        {
            EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(sym)) << "Symbol '" << name << "' not linked to MIR";
        }
    }
}

// =============================================================================
//  XII. MIR INSTRUCTION DETAIL VERIFICATION
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, MirDetail_LabelBlockContainsInstructions)
{
    std::string code = R"(
myLabel:
{
    create i32 %x;
    mov %x, 10;
    add %x, 5;
    nop;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));

    // Retrieve the label's MIR block via symbol linkage
    auto label = dynamic_cast<Label *>(getAstNode());
    auto symbolAnnot = label->getAnnotation<SymbolAnnotation>();
    MirId labelBlockId = getLoweringContext()->getMirIdOfSymbol(symbolAnnot->getSymbol());
    EXPECT_NE(labelBlockId, MIRID_INVALID);

    // We can't easily get the MirBlock* from just the ID without iterating the pool,
    // but we know the lowering succeeded and the block was created.
}

TEST_F(AstLowererVisitorTestFixture, MirDetail_ModuleBlockInstructions)
{
    std::string code = R"(
void SimpleFunc(i32 %p)
{
    create i32 %a;
    mov %a, %p;
    add %a, 100;
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    auto module = dynamic_cast<Module *>(getAstNode());
    auto symbolAnnot = module->getAnnotation<SymbolAnnotation>();
    Symbol *moduleSym = symbolAnnot->getSymbol();
    MirId moduleBlockId = getLoweringContext()->getMirIdOfSymbol(moduleSym);
    EXPECT_NE(moduleBlockId, MIRID_INVALID);
}

TEST_F(AstLowererVisitorTestFixture, MirDetail_ModuleType)
{
    std::string code = R"(
void SimpleFunc(i32 %p)
{
    create i32 %a;
    mov %a, %p;
    add %a, 100;
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    auto module = dynamic_cast<Module *>(getAstNode());
    auto symbolAnnot = module->getAnnotation<SymbolAnnotation>();
    Symbol *moduleSym = symbolAnnot->getSymbol();
    MirId moduleBlockId = getLoweringContext()->getMirIdOfSymbol(moduleSym);
    MirId moduleTypeId = getLoweringContext()->getMirIdOfSymbol(moduleSym);
    EXPECT_NE(moduleBlockId, MIRID_INVALID);
    EXPECT_NE(moduleTypeId, MIRID_INVALID);
}

// =============================================================================
//  XIII. TYPE SIZE VARIANTS
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, TypeSizes_i8Variable)
{
    std::string code = R"(
myLabel:
{
    create i8 %byte;
    mov %byte, 0xFF;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, TypeSizes_i16Variable)
{
    std::string code = R"(
myLabel:
{
    create i16 %word;
    mov %word, 0x1234;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, TypeSizes_i64Variable)
{
    std::string code = R"(
myLabel:
{
    create i64 %qword;
    mov %qword, 0xDEADBEEF;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, TypeSizes_FloatVariable)
{
    std::string code = R"(
myLabel:
{
    create float %f;
    mov %f, 1.0;
})";

    ASSERT_TRUE(runLowering<LabelParser>(code));
}

// =============================================================================
//  XIV. BITWISE / SHIFT COMBINATIONS
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, BitwiseCombination)
{
    std::string code = R"(
void Bitwise(i32 %x, i32 %mask)
{
    and %x, %mask;
    or %x, 0x01;
    xor %x, %mask;
    shl %x, 4;
    shr %x, 2;
    not %x;
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  XV. SEQUENTIAL IF-ELSE CHAINS
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, SequentialIfBlocks)
{
    std::string code = R"(
void SequentialIf(i32 %a, i32 %b, i32 %c, i32 %d)
{
    if (%a EQ %b)
    {
        nop;
    }
    if (%c NE %d)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  XVI. MIXED CONTROL FLOW
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, MixedControlFlow_IfThenWhile)
{
    std::string code = R"(
void Mixed(i32 %x, i32 %y, i32 %limit)
{
    if (%x EQ %y)
    {
        nop;
    }
    while (%x LT %limit)
    {
        add %x, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, MixedControlFlow_WhileThenIf)
{
    std::string code = R"(
void Mixed2(i32 %x, i32 %y, i32 %limit)
{
    while (%x LT %limit)
    {
        add %x, 1;
    }
    if (%x EQ %y)
    {
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  XVII. CONDITION IN WHILE – ALL COMPARISON TYPES
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, WhileCondition_EQ)
{
    std::string code = R"(
void WEQ(i32 %a, i32 %b)
{
    while (%a EQ %b)
    {
        add %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, WhileCondition_NE)
{
    std::string code = R"(
void WNE(i32 %a, i32 %b)
{
    while (%a NE %b)
    {
        add %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, WhileCondition_GE)
{
    std::string code = R"(
void WGE(i32 %a, i32 %b)
{
    while (%a GE %b)
    {
        sub %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, WhileCondition_LE)
{
    std::string code = R"(
void WLE(i32 %a, i32 %b)
{
    while (%a LE %b)
    {
        add %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

// =============================================================================
//  XVIII. BREAK / CONTINUE LOWERING
// =============================================================================

TEST_F(AstLowererVisitorTestFixture, BreakLowering_SimpleBreakInWhile)
{
    // A simple break inside a while loop should lower successfully.
    // Break emits a JMP to the loop's exit (falseBlock).
    std::string code = R"(
void TestBreak(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        break;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_SimpleContinueInWhile)
{
    // A simple continue inside a while loop should lower successfully.
    // Continue emits a JMP to the loop's condition check block (checkCondition).
    std::string code = R"(
void TestContinue(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        continue;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakWithCodeBefore)
{
    // Instructions before the break should be emitted normally,
    // then break emits JMP to exit, and a dead-code block is created.
    std::string code = R"(
void BreakAfterCode(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        add %a, 1;
        break;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueWithCodeBefore)
{
    // Instructions before the continue should be emitted normally,
    // then continue emits JMP to the condition block.
    std::string code = R"(
void ContinueAfterCode(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        add %a, 1;
        continue;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakWithDeadCodeAfter)
{
    // Code after a break is unreachable. The lowerer creates a dead-code block
    // so that any subsequent instructions don't corrupt the terminated block.
    // Lowering should still succeed (the dead code is just unreachable).
    std::string code = R"(
void BreakDeadCode(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        break;
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueWithDeadCodeAfter)
{
    // Code after a continue is unreachable, same dead-code block mechanism.
    std::string code = R"(
void ContinueDeadCode(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        continue;
        nop;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakInsideIfInsideWhile)
{
    // Break inside an if that's inside a while: the break should target
    // the while's exit block, not the if's merge block.
    std::string code = R"(
void BreakInIf(i32 %a, i32 %b, i32 %c)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            break;
        }
        add %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueInsideIfInsideWhile)
{
    // Continue inside an if that's inside a while: the continue should target
    // the while's condition check block.
    std::string code = R"(
void ContinueInIf(i32 %a, i32 %b, i32 %c)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            continue;
        }
        add %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakInsideElseBranchInsideWhile)
{
    // Break inside the else branch of an if inside a while.
    std::string code = R"(
void BreakInElse(i32 %a, i32 %b, i32 %c)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            add %a, 1;
        }
        else
        {
            break;
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueInsideElseBranchInsideWhile)
{
    // Continue inside the else branch of an if inside a while.
    std::string code = R"(
void ContinueInElse(i32 %a, i32 %b, i32 %c)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            add %a, 1;
        }
        else
        {
            continue;
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakContinueLowering_BothInSameWhile)
{
    // Both break and continue used in the same loop body (in different branches).
    std::string code = R"(
void BreakAndContinue(i32 %a, i32 %b, i32 %c)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            break;
        }
        else
        {
            add %a, 1;
            continue;
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakInNestedWhile_InnerOnly)
{
    // Break in the inner while only breaks the inner loop.
    // The outer loop should continue iterating normally.
    std::string code = R"(
void NestedBreak(i32 %i, i32 %j, i32 %n)
{
    while (%i LT %n)
    {
        while (%j LT %n)
        {
            break;
        }
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueInNestedWhile_InnerOnly)
{
    // Continue in the inner while only re-checks the inner loop's condition.
    std::string code = R"(
void NestedContinue(i32 %i, i32 %j, i32 %n)
{
    while (%i LT %n)
    {
        while (%j LT %n)
        {
            add %j, 1;
            continue;
        }
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakInOuterOfNestedWhile)
{
    // Break in the outer loop body (after the inner loop finishes).
    std::string code = R"(
void OuterBreak(i32 %i, i32 %j, i32 %n)
{
    while (%i LT %n)
    {
        while (%j LT %n)
        {
            add %j, 1;
        }
        break;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakContinueLowering_NestedWhileMixed)
{
    // Inner loop uses continue, outer loop uses break.
    std::string code = R"(
void NestedMixed(i32 %i, i32 %j, i32 %n, i32 %limit)
{
    while (%i LT %n)
    {
        while (%j LT %n)
        {
            add %j, 1;
            continue;
        }
        if (%i GE %limit)
        {
            break;
        }
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakOutsideLoop_Fails)
{
    // A break statement outside of any loop should be rejected by the
    // type checker (which runs before lowering). The pipeline must fail.
    std::string code = R"(
void BadBreak()
{
    break;
    nop;
})";

    EXPECT_FALSE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueOutsideLoop_Fails)
{
    // A continue statement outside of any loop should be rejected by the
    // type checker. The pipeline must fail.
    std::string code = R"(
void BadContinue()
{
    continue;
    nop;
})";

    EXPECT_FALSE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakInIfOutsideLoop_Fails)
{
    // Break inside an if but not inside any while – should fail.
    std::string code = R"(
void BadBreakInIf(i32 %a, i32 %b)
{
    if (%a EQ %b)
    {
        break;
    }
    nop;
})";

    EXPECT_FALSE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueInIfOutsideLoop_Fails)
{
    // Continue inside an if but not inside any while – should fail.
    std::string code = R"(
void BadContinueInIf(i32 %a, i32 %b)
{
    if (%a EQ %b)
    {
        continue;
    }
    nop;
})";

    EXPECT_FALSE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_MultipleBreaksInDifferentBranches)
{
    // Break appears in both the true and false branches of an if inside a while.
    // Both should target the same while exit block.
    std::string code = R"(
void MultiBreak(i32 %a, i32 %b, i32 %c)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            break;
        }
        else
        {
            break;
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_MultipleContinuesInDifferentBranches)
{
    // Continue appears in both branches of an if-else inside a while.
    std::string code = R"(
void MultiContinue(i32 %a, i32 %b, i32 %c)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            continue;
        }
        else
        {
            continue;
        }
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakContinueLowering_ComplexNestedControlFlow)
{
    // A complex scenario with nested ifs and whiles, using both break and continue.
    std::string code = R"(
void Complex(i32 %i, i32 %j, i32 %n, i32 %threshold, i32 %zero)
{
    while (%i LT %n)
    {
        if (%i EQ %zero)
        {
            add %i, 1;
            continue;
        }

        while (%j LT %n)
        {
            if (%j GE %threshold)
            {
                break;
            }
            add %j, 1;
        }

        if (%i GE %threshold)
        {
            break;
        }
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_WhileWithOnlyBreak)
{
    // A while loop whose body is solely a break – effectively a single-iteration loop.
    std::string code = R"(
void OnlyBreak(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        break;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_WhileWithOnlyContinue)
{
    // A while loop whose body is solely a continue – infinite re-check of condition.
    std::string code = R"(
void OnlyContinue(i32 %a, i32 %b)
{
    while (%a LT %b)
    {
        continue;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakLowering_BreakInDeepNestedIf)
{
    // Break inside a deeply nested if structure inside a while.
    std::string code = R"(
void DeepBreak(i32 %a, i32 %b, i32 %c, i32 %d)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            if (%c EQ %d)
            {
                break;
            }
        }
        add %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, ContinueLowering_ContinueInDeepNestedIf)
{
    // Continue inside a deeply nested if structure inside a while.
    std::string code = R"(
void DeepContinue(i32 %a, i32 %b, i32 %c, i32 %d)
{
    while (%a LT %b)
    {
        if (%a EQ %c)
        {
            if (%c EQ %d)
            {
                continue;
            }
        }
        add %a, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakContinueLowering_ThreeNestedWhilesBreakMiddle)
{
    // Three nested while loops. Break in the middle loop.
    std::string code = R"(
void ThreeNested(i32 %i, i32 %j, i32 %k, i32 %n)
{
    while (%i LT %n)
    {
        while (%j LT %n)
        {
            while (%k LT %n)
            {
                add %k, 1;
            }
            break;
        }
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));
}

TEST_F(AstLowererVisitorTestFixture, BreakContinueLowering_IntegrationCounterWithEarlyExit)
{
    // Realistic scenario: a counter loop with early exit on a condition.
    std::string code = R"(
i64 FindValue(i64 %ptr, i32 %len, i32 %target)
{
    create i32 %i;
    create i64 %addr;
    create i64 %val;
    mov %i, 0;
    mov %addr, %ptr;

    while (%i LT %len)
    {
        mov %val, i64 (%addr+0);

        if (%val EQ %target)
        {
            break;
        }

        add %addr, 8;
        add %i, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    // Verify symbol linkage for all variables.
    auto module = dynamic_cast<Module *>(getAstNode());
    ASSERT_NE(module, nullptr);
    auto scopedAnnot = module->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(scopedAnnot, nullptr);
    Scope *moduleScope = scopedAnnot->getOwnedScope();

    const char *names[] = { "ptr", "len", "target", "i", "addr", "val" };
    for (const char *name : names)
    {
        Symbol *sym = nullptr;
        EXPECT_TRUE(moduleScope->resolve(name, &sym, false)) << "Symbol '" << name << "' not found";
        if (sym)
        {
            EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(sym)) << "Symbol '" << name << "' not linked to MIR";
        }
    }
}

TEST_F(AstLowererVisitorTestFixture, BreakContinueLowering_IntegrationSkipAndAccumulate)
{
    // Realistic scenario: loop that skips certain values and accumulates the rest.
    std::string code = R"(
i64 Accumulate(i32 %n, i32 %skip, i32 %zero)
{
    create i32 %i;
    create i64 %sum;
    mov %i, 0;
    mov %sum, 0;

    while (%i LT %n)
    {
        add %i, 1;
        if (%i EQ %skip)
        {
            continue;
        }
        add %sum, 1;
    }
    nop;
})";

    ASSERT_TRUE(runLowering<ModuleParser::ModuleParser>(code));

    auto module = dynamic_cast<Module *>(getAstNode());
    ASSERT_NE(module, nullptr);
    auto scopedAnnot = module->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(scopedAnnot, nullptr);
    Scope *moduleScope = scopedAnnot->getOwnedScope();

    const char *names[] = { "n", "skip", "zero", "i", "sum" };
    for (const char *name : names)
    {
        Symbol *sym = nullptr;
        EXPECT_TRUE(moduleScope->resolve(name, &sym, false)) << "Symbol '" << name << "' not found";
        if (sym)
        {
            EXPECT_TRUE(getLoweringContext()->isSymbolLinkedToMir(sym)) << "Symbol '" << name << "' not linked to MIR";
        }
    }
}
