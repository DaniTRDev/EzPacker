#include "SymbolDefinitionVisitorTestFixture.h"

TEST_F(SymbolDefinitionVisitorTestFixture, TestCreateInstruction_Valid)
{
    std::string code = "create i32 %myVar;";
    EXPECT_TRUE(runVisitor<InstructionParser::InstructionParser>(code));

    auto instr = std::dynamic_pointer_cast<Instruction>(getAstNode());
    ASSERT_NE(instr, nullptr);

    auto operands = instr->getOperands();
    ASSERT_EQ(operands.size(), 1);

    auto var = std::dynamic_pointer_cast<Variable>(operands[0]);
    ASSERT_NE(var, nullptr);

    auto symbolAnnotation = var->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(symbolAnnotation, nullptr);

    auto symbol = symbolAnnotation->getSymbol();
    ASSERT_NE(symbol, nullptr);

    EXPECT_EQ(symbol->getName(), "myVar");
    EXPECT_EQ(symbol->getType(), SymbolType::LocalVariable);
    EXPECT_EQ(symbol->getSymbolDataType()->getUnderlyingType(), UnderlyingType::Integer);
}

TEST_F(SymbolDefinitionVisitorTestFixture, TestCreateInstruction_InvalidOperandCount)
{
    std::string code = "create i32 %myVar, i32 %otherVar;";
    // Parsing might succeed, but visitor should fail
    EXPECT_FALSE(runVisitor<InstructionParser::InstructionParser>(code));
}

TEST_F(SymbolDefinitionVisitorTestFixture, TestCreateInstruction_InvalidOperandType)
{
    std::string code = "create 123;";
    // Parsing might succeed (as immediate), but visitor expects variable
    EXPECT_FALSE(runVisitor<InstructionParser::InstructionParser>(code));
}

TEST_F(SymbolDefinitionVisitorTestFixture, TestCreateInstruction_Redefinition)
{
    std::string code = R"(
myLabel:
{
    create float %myVar;
    create float %myVar;
})";
    EXPECT_FALSE(runVisitor<LabelParser>(code));
}

TEST_F(SymbolDefinitionVisitorTestFixture, TestLabel_Valid)
{
    std::string code = R"(
myLabel:
{
    create float %localInLabel;
    create float %myVar;
    add i8 %myVar, 1;
    add %myVar2, i64 (1234);
    add %myVar3, i64 (%base+0);
    add %myVar4, i64 (%base+0xFEEF);
    nop;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code));

    auto label = std::dynamic_pointer_cast<Label>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scopedAnnotation = label->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(scopedAnnotation, nullptr);

    auto symbol = scopedAnnotation->getSymbol();
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->getName(), "myLabel");
    EXPECT_EQ(symbol->getType(), SymbolType::Label);

    auto scope = scopedAnnotation->getOwnedScope();
    ASSERT_NE(scope, nullptr);

    // Check if symbol inside label exists in the inner scope
    std::shared_ptr<Symbol> innerSymbol;
    EXPECT_TRUE(scope->resolve("localInLabel", &innerSymbol, false));
    ASSERT_NE(innerSymbol, nullptr);
    EXPECT_EQ(innerSymbol->getType(), SymbolType::LocalVariable);
}

TEST_F(SymbolDefinitionVisitorTestFixture, TestModule_Valid)
{
    std::string code = R"(
i64 MyModule(i32 %param1)
{
    create float %localInModule;
    add i8 %myVar, 1;
    add %myVar2, i64 (1234);
    add %myVar3, i64 (%base+0);
    add %myVar4, i64 (%base+0xFEEF);
    nop;
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code));

    auto module = std::dynamic_pointer_cast<Module>(getAstNode());
    ASSERT_NE(module, nullptr);
    
    auto scopedAnnotation = module->getAnnotation<ScopedSymbolAnnotation>();
    ASSERT_NE(scopedAnnotation, nullptr);

    auto symbol = scopedAnnotation->getSymbol();
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->getName(), "MyModule");
    EXPECT_EQ(symbol->getType(), SymbolType::Module);

    auto scope = scopedAnnotation->getOwnedScope();
    ASSERT_NE(scope, nullptr);

    // Check parameter symbol
    std::shared_ptr<Symbol> paramSymbol;
    EXPECT_TRUE(scope->resolve("param1", &paramSymbol, false));
    ASSERT_NE(paramSymbol, nullptr);
    EXPECT_EQ(paramSymbol->getType(), SymbolType::ModuleParameter);

    // Check local variable symbol
    std::shared_ptr<Symbol> localSymbol;
    EXPECT_TRUE(scope->resolve("localInModule", &localSymbol, false));
    ASSERT_NE(localSymbol, nullptr);
    EXPECT_EQ(localSymbol->getType(), SymbolType::LocalVariable);
}
