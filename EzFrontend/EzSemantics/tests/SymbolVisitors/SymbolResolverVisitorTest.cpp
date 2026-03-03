#include "SymbolResolverVisitorTestFixture.h"

TEST_F(SymbolResolverVisitorTestFixture, TestResolveVariable_Defined)
{
    // We create a variable first, then try to use it in another instruction.
    // Since we are testing unit level, we can simulate this by manually adding a symbol to the context
    // and then parsing an instruction that uses it.

    Symbol *symbol;
    getSemanticContext()->createSymbol(nullptr,
                                       SymbolType::LocalVariable,
                                       &symbol,
                                       TypeTable::getType("i32").get(),
                                       "myVar");

    std::string code = "mov %myVar, 123;";
    // We don't run definition visitor here because 'mov' doesn't define symbols, it uses them.
    // And we already manually defined 'myVar'.
    EXPECT_TRUE(runVisitor<InstructionParser::InstructionParser>(code, false));

    auto instr = dynamic_cast<Instruction *>(getAstNode());
    ASSERT_NE(instr, nullptr);

    auto operands = instr->getExpressions();
    ASSERT_EQ(operands->m_numElems, 2);

    auto var = operands->get<Variable>(0);
    ASSERT_NE(var, nullptr);

    auto symbolAnnotation = var->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symbolAnnotation, nullptr);

    EXPECT_EQ(symbolAnnotation->getSymbol()->getId(), symbol->getId());
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveVariable_Undefined)
{
    std::string code = "mov %undefinedVar, 123;";
    EXPECT_FALSE(runVisitor<InstructionParser::InstructionParser>(code, false));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveLabel_InternalScope)
{
    // Test that symbols defined inside a label are resolvable within that label
    std::string code = R"(
myLabel:
{
    create i32 %localInLabel;
    mov %localInLabel, 10;
})";

    // Here we run definition visitor first to create the label scope and the 'localInLabel' symbol.
    // Then resolver visitor should be able to resolve 'localInLabel' in the 'mov' instruction.
    EXPECT_TRUE(runVisitor<LabelParser>(code, true));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveModule_InternalScope)
{
    // Test that symbols defined inside a module (params and locals) are resolvable
    std::string code = R"(
i64 MyModule(i32 %param1, i64 %base)
{
    create float %localInModule;
    create i8 %localInModule2;
    add %localInModule, 1;
    add %param1, i64 (1234);
    add %param1, i64 (%base+0);
    add %localInModule2, i64 (%base+0xFEEF);
    nop;
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveModule_If)
{
    // Test that symbols defined inside a module (params and locals) are resolvable
    std::string code = R"(
i64 MyModule(i32 %param1, i64 %base)
{
    if (%param1 EQ %base)
    {
        create i64 %myIfVar1;
        create i64 %myIfVar2;
    }
    else if (%param1 NE %base)
    {
        create i64 %myElseIfVar1;
        create i64 %myElseIfVar2;
    }
    else
    {
        create i64 %myIfElseVar1;
        create i64 %myIfElseVar2;
    }
    create float %localInModule;
    create i8 %localInModule2;
    add %localInModule, 1;
    add %param1, i64 (1234);
    add %param1, i64 (%base+0);
    add %localInModule2, i64 (%base+0xFEEF);
    nop;
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveModule_While)
{
    // Test that symbols defined inside a module (params and locals) are resolvable
    std::string code = R"(
i64 MyModule(i32 %param1, i64 %base)
{
    while(%param1 EQ %base)
    {
        create i64 %myWhileVar;
        create i64 %myWhileVar2;
    }
    create float %localInModule;
    create i8 %localInModule2;
    add %localInModule, 1;
    add %param1, i64 (1234);
    add %param1, i64 (%base+0);
    add %localInModule2, i64 (%base+0xFEEF);
    nop;
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveModule_BigModule)
{
    // Test that symbols defined inside a module (params and locals) are resolvable
    std::string code = R"(
i64 CalculateChecksum(i64 %bufferPtr, i32 %length, i64 %key)
{
    # 1. Variable Declaration
    create i64 %runningSum;
    create i32 %counter;
    create i64 %currentAddr;
    create i8  %byteVal;
    create i64 %tempCalc;

    # 2. Initialization
    mov %runningSum, 0;
    mov %counter, 0;
    
    # Copy the base pointer so we can modify it
    mov %currentAddr, %bufferPtr;

    # 3. Start of Loop
    label_loop_start:
    {
        # Compare counter vs length. If counter >= length, jump to end.
        cmp %counter, %length;
        bge %label_loop_end;

        # 4. Memory Access
        # Load a single byte from the current address offset by 0
        # Using your syntax: i8 (%currentAddr)
        mov %byteVal, i8 (%currentAddr);

        # 5. Logic Operations (Simple Encrypt/Hash logic)
        # XOR the byte with a fixed mask (FF)
        xor %byteVal, 0xFF;
        
        # Add a "salt" from the argument %key to the byte
        # We cast the byte to i64 first to match the key size
        mov %tempCalc, i64 (%byteVal+0);
        add %tempCalc, %key;

        # 6. Accumulate
        # Add result to our running total
        add %runningSum, %tempCalc;

        # 7. Pointer Arithmetic
        # Advance the current address by 1 byte
        add %currentAddr, 1;
        
        # Increment loop counter
        add %counter, 1;

        # Jump back to start
        jmp %label_loop_start;
    }
    label_loop_end:
    {
        add %runningSum, i64 (%currentAddr+0x10);
        nop;
        ret %runningSum;
    }
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveMemoryOperand_Base)
{
    Symbol *symbol;
    getSemanticContext()->createSymbol(nullptr,
                                       SymbolType::LocalVariable,
                                       &symbol,
                                       TypeTable::getType("i64").get(),
                                       "baseVar");

    std::string code = "mov i32 (%baseVar + 10), 123;";
    EXPECT_TRUE(runVisitor<InstructionParser::InstructionParser>(code, false));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveMemoryOperand_Index)
{
    Symbol *symbol;
    getSemanticContext()->createSymbol(nullptr,
                                       SymbolType::LocalVariable,
                                       &symbol,
                                       TypeTable::getType("i64").get(),
                                       "indexVar");

    std::string code = "mov i32 (, %indexVar, 4), 123;";
    EXPECT_TRUE(runVisitor<InstructionParser::InstructionParser>(code, false));
}

TEST_F(SymbolResolverVisitorTestFixture, TestResolveMemoryOperand_Undefined)
{
    std::string code = "mov i32 (%baseVar + 10), 123;";
    EXPECT_FALSE(runVisitor<InstructionParser::InstructionParser>(code, false));
}
