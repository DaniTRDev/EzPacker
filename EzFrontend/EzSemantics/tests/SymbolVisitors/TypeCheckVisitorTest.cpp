#include "TypeCheckVisitorTestFixture.h"

TEST_F(TypeCheckVisitorTestFixture, TestNoCastNeeded)
{
    // Define a variable of type i32 and use it as i32.
    std::string code = R"(
myLabel:
{
    create i32 %myVar;
    mov %myVar, 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scope = label->getCodeScope();
    ASSERT_NE(scope, nullptr);

    // Get the 'mov' instruction (second expression in label)
    auto expressions = scope->getExpressions();
    ASSERT_NE(expressions, nullptr);
    ASSERT_GE(expressions->m_numElems, 2);

    auto instr = expressions->get<Instruction>(1);
    ASSERT_NE(instr, nullptr);

    auto instrExprs = instr->getExpressions();
    ASSERT_NE(instrExprs, nullptr);

    auto var = instrExprs->get<Variable>(0);
    ASSERT_NE(var, nullptr);

    auto annotation = var->getAnnotation<SymbolAnnotation>();
    ASSERT_NE(annotation, nullptr);

    // It should remain a SymbolAnnotation, not upgraded to TypeCastAnnotation if types match
    EXPECT_EQ(std::string(annotation->getAnnotationName()), "SymbolAnnotation");
}

TEST_F(TypeCheckVisitorTestFixture, TestImplicitCast_Expansion)
{
    // i32 -> i64 (Expansion)
    std::string code = R"(
myLabel:
{
    create i32 %myVar;
    mov i64 %myVar, 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scope = label->getCodeScope();
    ASSERT_NE(scope, nullptr);

    auto expressions = scope->getExpressions();
    ASSERT_NE(expressions, nullptr);
    ASSERT_GE(expressions->m_numElems, 2);

    auto instr = expressions->get<Instruction>(1);
    ASSERT_NE(instr, nullptr);

    auto instrExprs = instr->getExpressions();
    ASSERT_NE(instrExprs, nullptr);

    auto var = instrExprs->get<Variable>(0);
    ASSERT_NE(var, nullptr);

    auto annotation = var->getAnnotation<TypeCastAnnotation>();
    ASSERT_NE(annotation, nullptr);

    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    EXPECT_TRUE(annotation->isExpansion());
    EXPECT_FALSE(annotation->isTruncation());
}

TEST_F(TypeCheckVisitorTestFixture, TestImplicitCast_Truncation)
{
    // i64 -> i32 (Truncation)
    std::string code = R"(
myLabel:
{
    create i64 %myVar;
    mov i32 %myVar, 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scope = label->getCodeScope();
    ASSERT_NE(scope, nullptr);

    auto expressions = scope->getExpressions();
    ASSERT_NE(expressions, nullptr);
    ASSERT_GE(expressions->m_numElems, 2);

    auto instr = expressions->get<Instruction>(1);
    ASSERT_NE(instr, nullptr);

    auto instrExprs = instr->getExpressions();
    ASSERT_NE(instrExprs, nullptr);

    auto var = instrExprs->get<Variable>(0);
    ASSERT_NE(var, nullptr);

    auto annotation = var->getAnnotation<TypeCastAnnotation>();
    ASSERT_NE(annotation, nullptr);

    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    EXPECT_TRUE(annotation->isTruncation());
    EXPECT_FALSE(annotation->isExpansion());
}

TEST_F(TypeCheckVisitorTestFixture, TestInvalidCast_StringToInt)
{
    std::string code = R"(
myLabel:
{
    create string %myStr;
    mov i32 %myStr, 123;
})";

    EXPECT_FALSE(runVisitor<LabelParser>(code, true));
}

TEST_F(TypeCheckVisitorTestFixture, TestInvalidCast_IntToString)
{
    std::string code = R"(
myLabel:
{
    create i32 %myInt;
    mov string %myInt, "hello";
})";

    EXPECT_FALSE(runVisitor<LabelParser>(code, true));
}

TEST_F(TypeCheckVisitorTestFixture, TestMemoryOperand_SourceCast)
{
    std::string code = R"(
myLabel:
{
    create i32 %baseVar;
    mov i32 (i64 %baseVar+0), i64 %baseVar;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scope = label->getCodeScope();
    ASSERT_NE(scope, nullptr);

    auto expressions = scope->getExpressions();
    ASSERT_NE(expressions, nullptr);
    ASSERT_GE(expressions->m_numElems, 2);

    auto instr = expressions->get<Instruction>(1);
    ASSERT_NE(instr, nullptr);

    auto instrExprs = instr->getExpressions();
    ASSERT_NE(instrExprs, nullptr);

    auto memOperand = instrExprs->get<BaseDisplacementMemory>(0);
    ASSERT_NE(memOperand, nullptr);

    auto baseVar = memOperand->getBase();
    ASSERT_NE(baseVar, nullptr);

    auto annotation = baseVar->getAnnotation<TypeCastAnnotation>();
    ASSERT_NE(annotation, nullptr);

    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    EXPECT_TRUE(annotation->isExpansion());
}

TEST_F(TypeCheckVisitorTestFixture, TestMemoryOperand_IndexCast)
{
    std::string code = R"(
myLabel:
{
    create i64 %indexVar;
    # Cast i64 symbol to i32 usage in index
    mov i32 (, i32 %indexVar, 4), 123;
})";

    EXPECT_TRUE(runVisitor<LabelParser>(code, true));

    auto label = dynamic_cast<Label *>(getAstNode());
    ASSERT_NE(label, nullptr);

    auto scope = label->getCodeScope();
    ASSERT_NE(scope, nullptr);

    auto expressions = scope->getExpressions();
    ASSERT_NE(expressions, nullptr);
    ASSERT_GE(expressions->m_numElems, 2);

    // Fixed: Index changed from 2 to 1 (comments don't count as instructions)
    auto instr = expressions->get<Instruction>(1);
    ASSERT_NE(instr, nullptr);

    auto instrExprs = instr->getExpressions();
    ASSERT_NE(instrExprs, nullptr);

    auto memOp = instrExprs->get<IndexScaleMemory>(0);
    ASSERT_NE(memOp, nullptr);

    auto indexVar = dynamic_cast<Variable *>(memOp->getIndex());
    ASSERT_NE(indexVar, nullptr);

    auto annotation = indexVar->getAnnotation<TypeCastAnnotation>();
    ASSERT_NE(annotation, nullptr);

    EXPECT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    EXPECT_TRUE(annotation->isTruncation());
}

TEST_F(TypeCheckVisitorTestFixture, TestModuleScope)
{
    // Test casting within a module function
    std::string code = R"(
void MyFunc()
{
    create i32 %local;
    mov i64 %local, i32 100;
})";

    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));

    auto module = dynamic_cast<Module *>(getAstNode());
    ASSERT_NE(module, nullptr);

    auto body = module->getBody();
    ASSERT_NE(body, nullptr);

    auto expressions = body->getExpressions();
    ASSERT_NE(expressions, nullptr);
    ASSERT_GE(expressions->m_numElems, 2);

    auto instr = expressions->get<Instruction>(1);
    ASSERT_NE(instr, nullptr);

    auto var = instr->getExpressions()->get<Variable>(0);
    ASSERT_NE(var, nullptr);

    auto annotation = var->getAnnotation<TypeCastAnnotation>();
    ASSERT_NE(annotation, nullptr);

    ASSERT_EQ(std::string(annotation->getAnnotationName()), "TypeCastAnnotation");
    EXPECT_TRUE(annotation->isExpansion());
}

TEST_F(TypeCheckVisitorTestFixture, TestBreakOutsideLoop)
{
    std::string code = R"(
void MyFunc()
{
    break;
})";
    EXPECT_FALSE(runVisitor<ModuleParser::ModuleParser>(code, true));
}

TEST_F(TypeCheckVisitorTestFixture, TestContinueOutsideLoop)
{
    std::string code = R"(
void MyFunc()
{
    continue;
})";
    EXPECT_FALSE(runVisitor<ModuleParser::ModuleParser>(code, true));
}

TEST_F(TypeCheckVisitorTestFixture, TestBreakInsideLoop)
{
    std::string code = R"(
void MyFunc()
{
    create i32 %local;
    while(%local EQ %local)
    {
        break;
    }
})";
    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));
}

TEST_F(TypeCheckVisitorTestFixture, TestContinueInsideLoop)
{
    std::string code = R"(
void MyFunc()
{
    create i32 %local;
    while(%local EQ %local)
    {
        continue;
    }
})";
    EXPECT_TRUE(runVisitor<ModuleParser::ModuleParser>(code, true));
}