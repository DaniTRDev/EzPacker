#include "EzDslTestSuite.h"
#include "Ast/InstructionSelDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/InstructionSelDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

class InstSelDefLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. Addressing Mode Parameter Tests
// ============================================================================

TEST_F(InstSelDefLangTest, TestAddrModeParamSimple)
{
    std::string test = "GPR:base";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::AddrModeParam, DSL::Ast::InstSelDef::AddrModeParam>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_typeOrClass.m_node, "GPR");
    EXPECT_FALSE(res->m_typeParam.has_value());
    EXPECT_EQ(res->m_name.m_node, "base");
    EXPECT_FALSE(res->m_defaultValue.has_value());
}

TEST_F(InstSelDefLangTest, TestAddrModeParamWithDefault)
{
    std::string test = "simm12:offset = 0";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::AddrModeParam, DSL::Ast::InstSelDef::AddrModeParam>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_typeOrClass.m_node, "simm12");
    EXPECT_FALSE(res->m_typeParam.has_value());
    EXPECT_EQ(res->m_name.m_node, "offset");
    ASSERT_TRUE(res->m_defaultValue.has_value());
    EXPECT_EQ(res->m_defaultValue->m_node, 0);
}

TEST_F(InstSelDefLangTest, TestAddrModeParamParameterizedWithDefault)
{
    std::string test1 = "simm(i12):offset = 0";
    ParseContext ctx1 = createParseContextFromBuff("test1", test1);

    auto res1 = ctx1.parse<DSL::Parser::InstSelDef::AddrModeParam, DSL::Ast::InstSelDef::AddrModeParam>();
    ASSERT_TRUE(res1.has_value());
    EXPECT_EQ(res1->m_typeOrClass.m_node, "simm");
    ASSERT_TRUE(res1->m_typeParam.has_value());
    EXPECT_EQ(res1->m_typeParam->m_node, "i12");
    EXPECT_EQ(res1->m_name.m_node, "offset");
    ASSERT_TRUE(res1->m_defaultValue.has_value());
    EXPECT_EQ(res1->m_defaultValue->m_node, 0);

    std::string test2 = "imm(i32):disp = 16";
    ParseContext ctx2 = createParseContextFromBuff("test2", test2);

    auto res2 = ctx2.parse<DSL::Parser::InstSelDef::AddrModeParam, DSL::Ast::InstSelDef::AddrModeParam>();
    ASSERT_TRUE(res2.has_value());
    EXPECT_EQ(res2->m_typeOrClass.m_node, "imm");
    ASSERT_TRUE(res2->m_typeParam.has_value());
    EXPECT_EQ(res2->m_typeParam->m_node, "i32");
    EXPECT_EQ(res2->m_name.m_node, "disp");
    ASSERT_TRUE(res2->m_defaultValue.has_value());
    EXPECT_EQ(res2->m_defaultValue->m_node, 16);
}

// ============================================================================
// 2. Addressing Mode Variant & Declaration Tests
// ============================================================================

TEST_F(InstSelDefLangTest, TestAddrModeVariantParsing)
{
    std::string test = R"dsl(
variant OffsetAddr {
    match {
        ADDI $addr, GPR:$base, simm(i12):$offset;
    };
    when {
        hasOneUse($addr);
        immInRange($offset, -2048, 2047);
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::AddrModeVariantParser, DSL::Ast::InstSelDef::AddrModeVariant>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_variantName.m_node, "OffsetAddr");

    // Match block validation
    ASSERT_EQ(res->m_matchPatterns.size(), 1);
    EXPECT_EQ(res->m_matchPatterns[0].m_opcode.m_node, "ADDI");
    ASSERT_EQ(res->m_matchPatterns[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[0].m_name.m_node, "addr");
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[1].m_name.m_node, "base");
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_name.m_node, "offset");

    // When block validation
    ASSERT_EQ(res->m_predicates.size(), 2);
    EXPECT_EQ(res->m_predicates[0].m_predicateName.m_node, "hasOneUse");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_predicates[0].m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_predicates[0].m_arguments[0]).m_node, "addr");

    EXPECT_EQ(res->m_predicates[1].m_predicateName.m_node, "immInRange");
    ASSERT_EQ(res->m_predicates[1].m_arguments.size(), 3);
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_predicates[1].m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_predicates[1].m_arguments[0]).m_node, "offset");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::IntegerLiteral>(res->m_predicates[1].m_arguments[1]));
    EXPECT_EQ(std::get<DSL::Ast::Common::IntegerLiteral>(res->m_predicates[1].m_arguments[1]).m_node, -2048);
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::IntegerLiteral>(res->m_predicates[1].m_arguments[2]));
    EXPECT_EQ(std::get<DSL::Ast::Common::IntegerLiteral>(res->m_predicates[1].m_arguments[2]).m_node, 2047);
}

TEST_F(InstSelDefLangTest, TestAddrModeDefMultiVariantWithDefaults)
{
    std::string test = R"dsl(
addrmode AddrModeRegImm12(GPR:base, simm(i12):offset = 0) {
    variant OffsetAddr {
        match {
            ADDI $addr, GPR:$base, simm(i12):$offset;
        };
        when {
            hasOneUse($addr);
            immInRange($offset, -2048, 2047);
        };
    };
    variant BaseOnly {
        match {
            GPR:$base;
        };
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::AddrModeDefParser, DSL::Ast::InstSelDef::AddrModeDef>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "AddrModeRegImm12");

    // Parameters
    ASSERT_EQ(res->m_parameters.size(), 2);
    EXPECT_EQ(res->m_parameters[0].m_name.m_node, "base");
    EXPECT_FALSE(res->m_parameters[0].m_defaultValue.has_value());

    EXPECT_EQ(res->m_parameters[1].m_name.m_node, "offset");
    EXPECT_EQ(res->m_parameters[1].m_typeOrClass.m_node, "simm");
    ASSERT_TRUE(res->m_parameters[1].m_typeParam.has_value());
    EXPECT_EQ(res->m_parameters[1].m_typeParam->m_node, "i12");
    ASSERT_TRUE(res->m_parameters[1].m_defaultValue.has_value());
    EXPECT_EQ(res->m_parameters[1].m_defaultValue->m_node, 0);

    // Variants
    ASSERT_EQ(res->m_variants.size(), 2);
    EXPECT_EQ(res->m_variants[0].m_variantName.m_node, "OffsetAddr");
    EXPECT_EQ(res->m_variants[1].m_variantName.m_node, "BaseOnly");
    ASSERT_EQ(res->m_variants[1].m_matchPatterns.size(), 1);
    EXPECT_EQ(res->m_variants[1].m_matchPatterns[0].m_opcode.m_node, "GPR");
}

// ============================================================================
// 3. Instruction Selection Pattern Tests
// ============================================================================

TEST_F(InstSelDefLangTest, TestSimplePatternWithCost)
{
    std::string test = R"dsl(
pattern Select_ADDI {
    match {
        ADD i32:$dst, GPR:$rs1, simm(i12):$imm;
    };
    when {
        immInRange($imm, -2048, 2047);
    };
    emit {
        ADDI GPR:$dst, GPR:$rs1, $imm;
    };
    cost(1);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelPatternParser, DSL::Ast::InstSelDef::ISelPattern>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_patternName.m_node, "Select_ADDI");

    // Match block
    ASSERT_EQ(res->m_matchPatterns.size(), 1);
    EXPECT_EQ(res->m_matchPatterns[0].m_opcode.m_node, "ADD");
    ASSERT_EQ(res->m_matchPatterns[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[0].m_type->m_node, "i32");
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_name.m_node, "imm");

    // When block
    ASSERT_EQ(res->m_predicates.size(), 1);
    EXPECT_EQ(res->m_predicates[0].m_predicateName.m_node, "immInRange");

    // Emit block
    ASSERT_EQ(res->m_emitSequence.size(), 1);
    EXPECT_EQ(res->m_emitSequence[0].m_opcode.m_node, "ADDI");
    ASSERT_EQ(res->m_emitSequence[0].m_operands.size(), 3);
    EXPECT_EQ(res->m_emitSequence[0].m_operands[0].m_name.m_node, "dst");

    // Cost block
    ASSERT_TRUE(res->m_cost.has_value());
    EXPECT_EQ(res->m_cost->m_node, 1);
}

TEST_F(InstSelDefLangTest, TestMultiInstructionMatchAndEmit)
{
    std::string test = R"dsl(
pattern Select_SH2ADD {
    match {
        SHL i64:$tmp, GPR:$rs2, 2;
        ADD i64:$dst, GPR:$rs1, $tmp;
    };
    when {
        hasFeature(Zba);
        hasOneUse($tmp);
    };
    emit {
        SH2ADD GPR:$dst, GPR:$rs2, GPR:$rs1;
    };
    cost(1);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelPatternParser, DSL::Ast::InstSelDef::ISelPattern>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_patternName.m_node, "Select_SH2ADD");

    // 2 instructions in match
    ASSERT_EQ(res->m_matchPatterns.size(), 2);
    EXPECT_EQ(res->m_matchPatterns[0].m_opcode.m_node, "SHL");
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[2].m_immLiteral->m_node, 2);

    EXPECT_EQ(res->m_matchPatterns[1].m_opcode.m_node, "ADD");
    EXPECT_EQ(res->m_matchPatterns[1].m_operands[2].m_name.m_node, "tmp");

    // Predicates
    ASSERT_EQ(res->m_predicates.size(), 2);
    EXPECT_EQ(res->m_predicates[0].m_predicateName.m_node, "hasFeature");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_predicates[0].m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_predicates[0].m_arguments[0]).m_node, "Zba");

    EXPECT_EQ(res->m_predicates[1].m_predicateName.m_node, "hasOneUse");
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(res->m_predicates[1].m_arguments[0]));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(res->m_predicates[1].m_arguments[0]).m_node, "tmp");

    // Emit
    ASSERT_EQ(res->m_emitSequence.size(), 1);
    EXPECT_EQ(res->m_emitSequence[0].m_opcode.m_node, "SH2ADD");
}

TEST_F(InstSelDefLangTest, TestPatternUsingAddrModeAndTransforms)
{
    std::string test = R"dsl(
pattern Select_LW {
    match {
        LOAD i32:$dst, AddrModeRegImm12($base, $offset);
    };
    emit {
        LW GPR:$dst, GPR:$base, $offset;
    };
    cost(2);
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelPatternParser, DSL::Ast::InstSelDef::ISelPattern>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_patternName.m_node, "Select_LW");

    ASSERT_EQ(res->m_matchPatterns.size(), 1);
    EXPECT_EQ(res->m_matchPatterns[0].m_opcode.m_node, "LOAD");
    ASSERT_EQ(res->m_matchPatterns[0].m_operands.size(), 2);

    // Verify AddrMode invocation parses as CustomTransform operand
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[1].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[1].m_name.m_node, "AddrModeRegImm12");
    ASSERT_EQ(res->m_matchPatterns[0].m_operands[1].m_callArgs.size(), 2);
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[1].m_callArgs[0].m_node, "base");
    EXPECT_EQ(res->m_matchPatterns[0].m_operands[1].m_callArgs[1].m_node, "offset");

    ASSERT_EQ(res->m_emitSequence.size(), 1);
    EXPECT_EQ(res->m_emitSequence[0].m_opcode.m_node, "LW");
    ASSERT_TRUE(res->m_cost.has_value());
    EXPECT_EQ(res->m_cost.value().m_node, 2);
}

TEST_F(InstSelDefLangTest, TestPatternWithMultiInstructionEmitAndTransform)
{
    std::string test = R"dsl(
pattern Select_RotL {
    match {
        ROTL i32:$dst, GPR:$src, imm(i32):$amt;
    };
    emit {
        SLLI GPR:$tmp1, GPR:$src, $amt;
        SRLI GPR:$tmp2, GPR:$src, sub($amt);
        OR   GPR:$dst, GPR:$tmp1, GPR:$tmp2;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelPatternParser, DSL::Ast::InstSelDef::ISelPattern>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_patternName.m_node, "Select_RotL");
    EXPECT_FALSE(res->m_cost.has_value());

    ASSERT_EQ(res->m_emitSequence.size(), 3);
    EXPECT_EQ(res->m_emitSequence[0].m_opcode.m_node, "SLLI");
    EXPECT_EQ(res->m_emitSequence[1].m_opcode.m_node, "SRLI");
    EXPECT_EQ(res->m_emitSequence[2].m_opcode.m_node, "OR");

    // Check sub($amt) transform operand in SRLI
    ASSERT_EQ(res->m_emitSequence[1].m_operands.size(), 3);
    EXPECT_EQ(res->m_emitSequence[1].m_operands[2].m_kind, DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform);
    EXPECT_EQ(res->m_emitSequence[1].m_operands[2].m_name.m_node, "sub");
    ASSERT_EQ(res->m_emitSequence[1].m_operands[2].m_callArgs.size(), 1);
    EXPECT_EQ(res->m_emitSequence[1].m_operands[2].m_callArgs[0].m_node, "amt");
}

// ============================================================================
// 4. Full Translation Unit Tests
// ============================================================================

TEST_F(InstSelDefLangTest, TestFullTranslationUnit)
{
    std::string test = R"dsl(
addrmode AddrModeRegImm12(GPR:base, simm(i12):offset = 0) {
    variant OffsetAddr {
        match {
            ADDI $addr, GPR:$base, simm(i12):$offset;
        };
        when {
            hasOneUse($addr);
        };
    };
    variant BaseOnly {
        match {
            GPR:$base;
        };
    };
};

pattern Select_LW {
    match {
        LOAD i32:$dst, AddrModeRegImm12($base, $offset);
    };
    emit {
        LW GPR:$dst, GPR:$base, $offset;
    };
    cost(1);
};

pattern Select_SW {
    match {
        STORE GPR:$src, AddrModeRegImm12($base, $offset);
    };
    emit {
        SW GPR:$src, GPR:$base, $offset;
    };
    cost(1);
};
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelDefFileParser, DSL::Ast::InstSelDef::ISelDefFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_addrModes.size(), 1);
    ASSERT_EQ(res->m_patterns.size(), 2);

    EXPECT_EQ(res->m_addrModes[0].m_name.m_node, "AddrModeRegImm12");
    EXPECT_EQ(res->m_patterns[0].m_patternName.m_node, "Select_LW");
    EXPECT_EQ(res->m_patterns[1].m_patternName.m_node, "Select_SW");
}

// ============================================================================
// 5. Negative & Error Parsing Tests
// ============================================================================

TEST_F(InstSelDefLangTest, TestDisallowedPostfixImmediateInPatternError)
{
    std::string test = R"dsl(
pattern BadPattern {
    match {
        ADD i32:$dst, GPR:$rs1, $imm:imm;
    };
    emit {
        ADDI GPR:$dst, GPR:$rs1, $imm;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelPatternParser, DSL::Ast::InstSelDef::ISelPattern>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(InstSelDefLangTest, TestMissingSemicolonAfterBlockError)
{
    std::string test = R"dsl(
pattern BadPattern {
    match {
        ADD i32:$dst, GPR:$rs1, simm(12):$imm;
    }
    emit {
        ADDI GPR:$dst, GPR:$rs1, $imm;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelPatternParser, DSL::Ast::InstSelDef::ISelPattern>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(InstSelDefLangTest, TestMissingSemicolonAfterPatternInFileError)
{
    std::string test = R"dsl(
pattern BadPattern {
    match {
        ADD i32:$dst, GPR:$rs1, simm(12):$imm;
    };
    emit {
        ADDI GPR:$dst, GPR:$rs1, $imm;
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelDefFileParser, DSL::Ast::InstSelDef::ISelDefFile>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(InstSelDefLangTest, TestMissingParamDefaultValueError)
{
    std::string test = R"dsl(
addrmode BadAddrMode(GPR:base, simm(12):offset =) {
    variant BaseOnly {
        match { GPR:$base; };
    };
}
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::AddrModeDefParser, DSL::Ast::InstSelDef::AddrModeDef>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(InstSelDefLangTest, TestUnterminatedPatternBodyError)
{
    std::string test = R"dsl(
pattern IncompletePattern {
    match {
        ADD i32:$dst, GPR:$rs1, simm(12):$imm;
    };
    emit {
        ADDI GPR:$dst, GPR:$rs1, $imm;
    };
)dsl";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::InstSelDef::ISelPatternParser, DSL::Ast::InstSelDef::ISelPattern>();
    EXPECT_FALSE(res.has_value());
}