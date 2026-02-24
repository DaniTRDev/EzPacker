#include "TypeCastVisitorTestFixture.h"

TEST_F(TypeCastVisitorTestFixture, TestNoCastNeeded)
{
    // Define a variable of type i32 and use it as i32.
    std::string code = R"(
myLabel:
{
    create i32 %myVar;
    mov %myVar, 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = std::dynamic_pointer_cast<Label>(getAstNode());
    ASSERT_NE(label, nullptr);

    // Get the 'mov' instruction (second expression in label)
    auto expressions = label->getExpressions();
    ASSERT_GE(expressions.size(), 2);

    auto it = expressions.begin();
    std::advance(it, 1);
    auto instr = std::dynamic_pointer_cast<Instruction>(it->second);
    ASSERT_NE(instr, nullptr);

    auto var = std::dynamic_pointer_cast<Variable>(instr->getOperands()[0]);
    auto annotation = var->getAnnotation<SymbolAnnotation>();

    // It should remain a SymbolAnnotation, not upgraded to TypeCastAnnotation if types match
    EXPECT_EQ(std::string(annotation->getAnnotationName()), "SymbolAnnotation");
}

TEST_F(TypeCastVisitorTestFixture, TestImplicitCast_Expansion)
{
    // i32 -> i64 (Expansion)
    std::string code = R"(
myLabel:
{
    create i32 %myVar;
    mov i64 %myVar, 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = std::dynamic_pointer_cast<Label>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto expressions = label->getExpressions();
    ASSERT_GE(expressions.size(), 2);

    auto it = expressions.begin();
    std::advance(it, 1);
    auto instr = std::dynamic_pointer_cast<Instruction>(it->second);
    ASSERT_NE(instr, nullptr);

    auto var = std::dynamic_pointer_cast<Variable>(instr->getOperands()[0]);
    auto annotation = var->getAnnotation<TypeCastAnnotation>();

    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    auto typeCastAnnotation = std::dynamic_pointer_cast<TypeCastAnnotation>(annotation);
    EXPECT_TRUE(typeCastAnnotation->isExpansion());
    EXPECT_FALSE(typeCastAnnotation->isTruncation());
}

TEST_F(TypeCastVisitorTestFixture, TestImplicitCast_Truncation)
{
    // i64 -> i32 (Truncation)
    std::string code = R"(
myLabel:
{
    create i64 %myVar;
    mov i32 %myVar, 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = std::dynamic_pointer_cast<Label>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto expressions = label->getExpressions();
    ASSERT_GE(expressions.size(), 2);

    auto it = expressions.begin();
    std::advance(it, 1);
    auto instr = std::dynamic_pointer_cast<Instruction>(it->second);
    ASSERT_NE(instr, nullptr);

    auto var = std::dynamic_pointer_cast<Variable>(instr->getOperands()[0]);
    auto annotation = var->getAnnotation<TypeCastAnnotation>();

    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    auto typeCastAnnotation = std::dynamic_pointer_cast<TypeCastAnnotation>(annotation);
    EXPECT_TRUE(typeCastAnnotation->isTruncation());
    EXPECT_FALSE(typeCastAnnotation->isExpansion());
}

TEST_F(TypeCastVisitorTestFixture, TestInvalidCast_StringToInt)
{
    std::string code = R"(
myLabel:
{
    create string %myStr;
    mov i32 %myStr, 123;
})";

    EXPECT_FALSE(runVisitor<LabelParser>(code, true));
}

TEST_F(TypeCastVisitorTestFixture, TestInvalidCast_IntToString)
{
    std::string code = R"(
myLabel:
{
    create i32 %myInt;
    mov string %myInt, "hello";
})";

    EXPECT_FALSE(runVisitor<LabelParser>(code, true));
}

TEST_F(TypeCastVisitorTestFixture, TestMemoryOperand_SourceCast)
{
    std::string code = R"(
myLabel:
{
    create i32 %baseVar;
    mov i32 (i64 %baseVar+0), i64 %baseVar;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = std::dynamic_pointer_cast<Label>(getAstNode());
    auto expressions = label->getExpressions();
    ASSERT_GE(expressions.size(), 2);

    auto it = expressions.begin();
    std::advance(it, 1);
    auto instr = std::dynamic_pointer_cast<Instruction>(it->second);
    
    auto baseVar = std::dynamic_pointer_cast<Variable>(instr->getOperands()[1]);
    ASSERT_NE(baseVar, nullptr);

    auto annotation = baseVar->getAnnotation<TypeCastAnnotation>();
    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    auto typeCast = std::dynamic_pointer_cast<TypeCastAnnotation>(annotation);
    EXPECT_TRUE(typeCast->isExpansion());
}

TEST_F(TypeCastVisitorTestFixture, TestMemoryOperand_IndexCast)
{
    std::string code = R"(
myLabel:
{
    create i64 %indexVar;
    # Cast i64 symbol to i32 usage in index
    mov i32 (, i32 %indexVar, 4), 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = std::dynamic_pointer_cast<Label>(getAstNode());
    auto expressions = label->getExpressions();
    ASSERT_GE(expressions.size(), 2);

    auto it = expressions.begin();
    std::advance(it, 1);
    auto instr = std::dynamic_pointer_cast<Instruction>(it->second);

    auto memOp = std::dynamic_pointer_cast<IndexScaleMemory>(instr->getOperands()[0]);
    ASSERT_NE(memOp, nullptr);

    auto indexVar = std::dynamic_pointer_cast<Variable>(memOp->getIndex());
    ASSERT_NE(indexVar, nullptr);

    auto annotation = indexVar->getAnnotation<TypeCastAnnotation>();
    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    auto typeCast = std::dynamic_pointer_cast<TypeCastAnnotation>(annotation);
    EXPECT_TRUE(typeCast->isTruncation());
}

TEST_F(TypeCastVisitorTestFixture, TestModuleScope)
{
    // Test casting within a module function
    std::string code = R"(
void MyFunc()
{
    create i32 %local;
    mov i64 %local, 100;
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));

    auto module = std::dynamic_pointer_cast<Module>(getAstNode());
    ASSERT_NE(module, nullptr);

    auto body = module->getBody();
    auto expressions = body->getExpressions();
    ASSERT_GE(expressions.size(), 2);

    auto it = expressions.begin();
    std::advance(it, 1);
    auto instr = std::dynamic_pointer_cast<Instruction>(it->second);
    ASSERT_NE(instr, nullptr);

    auto var = std::dynamic_pointer_cast<Variable>(instr->getOperands()[0]);
    auto annotation = var->getAnnotation<TypeCastAnnotation>();

    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    auto typeCast = std::dynamic_pointer_cast<TypeCastAnnotation>(annotation);
    EXPECT_TRUE(typeCast->isExpansion());
}
