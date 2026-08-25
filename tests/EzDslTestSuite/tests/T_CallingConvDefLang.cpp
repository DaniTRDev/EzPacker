#include "EzDslTestSuite.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/CallingConvDefLang.h"
#include "Parser/ParseContext.h"
#include "SourceManager/SourceManager.h"

class CallingConvDefLangTest : public DslTestSuiteAsGtest
{
  public:
};

// ============================================================================
// 1. Registers & Stack Configuration
// ============================================================================

TEST_F(CallingConvDefLangTest, TestRegisterReference)
{
    std::string test = "GPR:rdi";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::CallingConvDef::RegisterRef, DSL::Ast::CallingConvDef::RegisterRef>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_className.m_node, "GPR");
    EXPECT_EQ(res->m_regName.m_node, "rdi");

    std::string fprTest = "FPR:xmm0";
    ParseContext fprCtx = createParseContextFromBuff("fprTest", fprTest);

    auto fprRes = fprCtx.parse<DSL::Parser::CallingConvDef::RegisterRef, DSL::Ast::CallingConvDef::RegisterRef>();
    ASSERT_TRUE(fprRes.has_value());
    EXPECT_EQ(fprRes->m_className.m_node, "FPR");
    EXPECT_EQ(fprRes->m_regName.m_node, "xmm0");
}

TEST_F(CallingConvDefLangTest, TestStackPlacement)
{
    std::string unaligned = "STACK";
    ParseContext unalignedCtx = createParseContextFromBuff("unaligned", unaligned);

    auto unalignedRes =
            unalignedCtx.parse<DSL::Parser::CallingConvDef::StackPlacement, DSL::Ast::CallingConvDef::StackPlacement>();
    ASSERT_TRUE(unalignedRes.has_value());
    EXPECT_FALSE(unalignedRes->m_alignment.has_value());

    std::string explicitAlign = "STACK(ALIGN: 16)";
    ParseContext explicitCtx = createParseContextFromBuff("explicitAlign", explicitAlign);

    auto explicitRes =
            explicitCtx.parse<DSL::Parser::CallingConvDef::StackPlacement, DSL::Ast::CallingConvDef::StackPlacement>();
    ASSERT_TRUE(explicitRes.has_value());
    ASSERT_TRUE(explicitRes->m_alignment.has_value());
    EXPECT_EQ(explicitRes->m_alignment->m_node, 16);

    std::string directAlign = "STACK(8)";
    ParseContext directCtx = createParseContextFromBuff("directAlign", directAlign);

    auto directRes =
            directCtx.parse<DSL::Parser::CallingConvDef::StackPlacement, DSL::Ast::CallingConvDef::StackPlacement>();
    ASSERT_TRUE(directRes.has_value());
    ASSERT_TRUE(directRes->m_alignment.has_value());
    EXPECT_EQ(directRes->m_alignment->m_node, 8);
}

// ============================================================================
// 2. Classification Stage
// ============================================================================

TEST_F(CallingConvDefLangTest, TestPrimitiveClassifyRule)
{
    std::string test = "TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::CallingConvDef::PrimitiveClassifyRule,
                         DSL::Ast::CallingConvDef::PrimitiveClassifyRule>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_types.size(), 6);
    EXPECT_EQ(res->m_types[0].m_node, "i1");
    EXPECT_EQ(res->m_types[1].m_node, "i8");
    EXPECT_EQ(res->m_types[5].m_node, "ptr");
    EXPECT_EQ(res->m_targetClass.m_node, "INTEGER");
}

TEST_F(CallingConvDefLangTest, TestAggregatePredicates)
{
    std::string sizeGt = "IF_SIZE_GT(16) >> MEMORY;";
    ParseContext sizeGtCtx = createParseContextFromBuff("sizeGt", sizeGt);

    auto sizeGtRes = sizeGtCtx.parse<DSL::Parser::CallingConvDef::AggregatePredicate,
                                     DSL::Ast::CallingConvDef::AggregatePredicate>();
    ASSERT_TRUE(sizeGtRes.has_value());
    EXPECT_EQ(sizeGtRes->m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::SizeGt);
    ASSERT_TRUE(sizeGtRes->m_size.has_value());
    EXPECT_EQ(sizeGtRes->m_size->m_node, 16);
    EXPECT_EQ(sizeGtRes->m_resultClass.m_node, "MEMORY");

    std::string sizeIn = "IF_SIZE_IN(1, 2, 4, 8) >> INTEGER;";
    ParseContext sizeInCtx = createParseContextFromBuff("sizeIn", sizeIn);

    auto sizeInRes = sizeInCtx.parse<DSL::Parser::CallingConvDef::AggregatePredicate,
                                     DSL::Ast::CallingConvDef::AggregatePredicate>();
    ASSERT_TRUE(sizeInRes.has_value());
    EXPECT_EQ(sizeInRes->m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::SizeIn);
    ASSERT_EQ(sizeInRes->m_sizes.size(), 4);
    EXPECT_EQ(sizeInRes->m_sizes[0].m_node, 1);
    EXPECT_EQ(sizeInRes->m_sizes[3].m_node, 8);
    EXPECT_EQ(sizeInRes->m_resultClass.m_node, "INTEGER");

    std::string hfa = "IF_HOMOGENEOUS(FLOAT, MAX: 4) >> HFA;";
    ParseContext hfaCtx = createParseContextFromBuff("hfa", hfa);

    auto hfaRes = hfaCtx.parse<DSL::Parser::CallingConvDef::AggregatePredicate,
                               DSL::Ast::CallingConvDef::AggregatePredicate>();
    ASSERT_TRUE(hfaRes.has_value());
    EXPECT_EQ(hfaRes->m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::Homogeneous);
    ASSERT_TRUE(hfaRes->m_homogeneousClass.has_value());
    EXPECT_EQ(hfaRes->m_homogeneousClass->m_node, "FLOAT");
    ASSERT_TRUE(hfaRes->m_maxElements.has_value());
    EXPECT_EQ(hfaRes->m_maxElements->m_node, 4);
    EXPECT_EQ(hfaRes->m_resultClass.m_node, "HFA");

    std::string unaligned = "IF_UNALIGNED >> MEMORY;";
    ParseContext unalignedCtx = createParseContextFromBuff("unaligned", unaligned);

    auto unalignedRes = unalignedCtx.parse<DSL::Parser::CallingConvDef::AggregatePredicate,
                                           DSL::Ast::CallingConvDef::AggregatePredicate>();
    ASSERT_TRUE(unalignedRes.has_value());
    EXPECT_EQ(unalignedRes->m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::Unaligned);
    EXPECT_EQ(unalignedRes->m_resultClass.m_node, "MEMORY");

    std::string def = "DEFAULT >> BY_REF;";
    ParseContext defCtx = createParseContextFromBuff("def", def);

    auto defRes = defCtx.parse<DSL::Parser::CallingConvDef::AggregatePredicate,
                               DSL::Ast::CallingConvDef::AggregatePredicate>();
    ASSERT_TRUE(defRes.has_value());
    EXPECT_EQ(defRes->m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::Default);
    EXPECT_EQ(defRes->m_resultClass.m_node, "BY_REF");
}

TEST_F(CallingConvDefLangTest, TestAggregateClassifyDef)
{
    std::string test = R"(
AGGREGATE {
    IF_SIZE_GT(16)   >> MEMORY;
    IF_UNALIGNED     >> MEMORY;
    IF_NON_TRIVIAL   >> MEMORY;

    CHUNK_SIZE(8);
    MERGE_PRECEDENCE >> MEMORY > INTEGER > FLOAT;
    ALLOC_POLICY(ALL_OR_NOTHING);
};
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::CallingConvDef::AggregateClassifyDef,
                         DSL::Ast::CallingConvDef::AggregateClassifyDef>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_predicates.size(), 3);
    EXPECT_EQ(res->m_predicates[0].m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::SizeGt);
    EXPECT_EQ(res->m_predicates[1].m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::Unaligned);
    EXPECT_EQ(res->m_predicates[2].m_kind, DSL::Ast::CallingConvDef::AggregatePredicateKind::NonTrivial);

    ASSERT_TRUE(res->m_chunkSize.has_value());
    EXPECT_EQ(res->m_chunkSize->m_node, 8);

    ASSERT_EQ(res->m_mergePrecedence.size(), 3);
    EXPECT_EQ(res->m_mergePrecedence[0].m_node, "MEMORY");
    EXPECT_EQ(res->m_mergePrecedence[1].m_node, "INTEGER");
    EXPECT_EQ(res->m_mergePrecedence[2].m_node, "FLOAT");

    EXPECT_EQ(res->m_allocPolicy, DSL::Ast::CallingConvDef::AllocPolicy::AllOrNothing);
}

TEST_F(CallingConvDefLangTest, TestClassifyBlock)
{
    std::string test = R"(
CLASSIFY {
    TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;
    TYPE(f32, f64)                  >> FLOAT;

    AGGREGATE {
        IF_SIZE_GT(16)   >> MEMORY;
        CHUNK_SIZE(8);
        MERGE_PRECEDENCE >> MEMORY > INTEGER > FLOAT;
    };
}
)";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::CallingConvDef::ClassifyBlock, DSL::Ast::CallingConvDef::ClassifyBlock>();
    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(res->m_primitiveRules.size(), 2);
    EXPECT_EQ(res->m_primitiveRules[0].m_targetClass.m_node, "INTEGER");
    EXPECT_EQ(res->m_primitiveRules[1].m_targetClass.m_node, "FLOAT");

    ASSERT_TRUE(res->m_aggregateDef.has_value());
    ASSERT_EQ(res->m_aggregateDef->m_predicates.size(), 1);
    ASSERT_TRUE(res->m_aggregateDef->m_chunkSize.has_value());
    EXPECT_EQ(res->m_aggregateDef->m_chunkSize->m_node, 8);
}

// ============================================================================
// 3. Dispatch & SRET Rules
// ============================================================================

TEST_F(CallingConvDefLangTest, TestSequentialDispatchRule)
{
    std::string test = "INTEGER >> REG_SEQ(GPR:rdi, GPR:rsi, GPR:rdx, GPR:rcx, GPR:r8, GPR:r9) >> STACK(ALIGN: 8);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::CallingConvDef::DispatchRule, DSL::Ast::CallingConvDef::DispatchRule>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_abiClass.m_node, "INTEGER");
    EXPECT_EQ(res->m_action.m_kind, DSL::Ast::CallingConvDef::LoweringActionKind::RegisterAssign);
    EXPECT_EQ(res->m_action.m_regAssignKind, DSL::Ast::CallingConvDef::RegAssignKind::Sequence);
    ASSERT_EQ(res->m_action.m_registers.size(), 6);
    EXPECT_EQ(res->m_action.m_registers[0].m_regName.m_node, "rdi");
    EXPECT_EQ(res->m_action.m_registers[5].m_regName.m_node, "r9");

    ASSERT_TRUE(res->m_action.m_stackFallback.has_value());
    ASSERT_TRUE(res->m_action.m_stackFallback->m_alignment.has_value());
    EXPECT_EQ(res->m_action.m_stackFallback->m_alignment->m_node, 8);
}

TEST_F(CallingConvDefLangTest, TestSlotsAndExpansionDispatchRules)
{
    std::string slotsTest = "FLOAT >> REG_SLOTS(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3) >> STACK;";
    ParseContext slotsCtx = createParseContextFromBuff("slotsTest", slotsTest);

    auto slotsRes = slotsCtx.parse<DSL::Parser::CallingConvDef::DispatchRule, DSL::Ast::CallingConvDef::DispatchRule>();
    ASSERT_TRUE(slotsRes.has_value());
    EXPECT_EQ(slotsRes->m_abiClass.m_node, "FLOAT");
    EXPECT_EQ(slotsRes->m_action.m_regAssignKind, DSL::Ast::CallingConvDef::RegAssignKind::Slots);
    ASSERT_EQ(slotsRes->m_action.m_registers.size(), 4);
    EXPECT_EQ(slotsRes->m_action.m_registers[0].m_regName.m_node, "xmm0");

    std::string expandTest = "HFA >> EXPAND_TO(FLOAT);";
    ParseContext expandCtx = createParseContextFromBuff("expandTest", expandTest);

    auto expandRes =
            expandCtx.parse<DSL::Parser::CallingConvDef::DispatchRule, DSL::Ast::CallingConvDef::DispatchRule>();
    ASSERT_TRUE(expandRes.has_value());
    EXPECT_EQ(expandRes->m_abiClass.m_node, "HFA");
    EXPECT_EQ(expandRes->m_action.m_kind, DSL::Ast::CallingConvDef::LoweringActionKind::ExpandTo);
    ASSERT_TRUE(expandRes->m_action.m_targetClass.has_value());
    EXPECT_EQ(expandRes->m_action.m_targetClass->m_node, "FLOAT");

    std::string byRefTest = "BY_REF >> PASS_AS_POINTER >> INTEGER;";
    ParseContext byRefCtx = createParseContextFromBuff("byRefTest", byRefTest);

    auto byRefRes = byRefCtx.parse<DSL::Parser::CallingConvDef::DispatchRule, DSL::Ast::CallingConvDef::DispatchRule>();
    ASSERT_TRUE(byRefRes.has_value());
    EXPECT_EQ(byRefRes->m_abiClass.m_node, "BY_REF");
    EXPECT_EQ(byRefRes->m_action.m_kind, DSL::Ast::CallingConvDef::LoweringActionKind::PassAsPointer);
    ASSERT_TRUE(byRefRes->m_action.m_targetClass.has_value());
    EXPECT_EQ(byRefRes->m_action.m_targetClass->m_node, "INTEGER");
}

TEST_F(CallingConvDefLangTest, TestSretConfig)
{
    std::string sysvSret = R"(
SRET_CONFIG {
    PASS_IN_REG(GPR:rdi);
    CONSUMES_ARG_SLOT(true);
    RETURN_REG(GPR:rax);
};
)";
    ParseContext sysvCtx = createParseContextFromBuff("sysvSret", sysvSret);

    auto sysvRes = sysvCtx.parse<DSL::Parser::CallingConvDef::SretConfig, DSL::Ast::CallingConvDef::SretConfig>();
    ASSERT_TRUE(sysvRes.has_value());
    EXPECT_EQ(sysvRes->m_passInReg.m_className.m_node, "GPR");
    EXPECT_EQ(sysvRes->m_passInReg.m_regName.m_node, "rdi");
    EXPECT_TRUE(sysvRes->m_consumesArgSlot);
    ASSERT_TRUE(sysvRes->m_returnReg.has_value());
    EXPECT_EQ(sysvRes->m_returnReg->m_regName.m_node, "rax");

    std::string armSret = R"(
SRET_CONFIG {
    PASS_IN_REG(GPR:x8);
    CONSUMES_ARG_SLOT(false);
    RETURN_REG(NONE);
};
)";
    ParseContext armCtx = createParseContextFromBuff("armSret", armSret);

    auto armRes = armCtx.parse<DSL::Parser::CallingConvDef::SretConfig, DSL::Ast::CallingConvDef::SretConfig>();
    ASSERT_TRUE(armRes.has_value());
    EXPECT_EQ(armRes->m_passInReg.m_regName.m_node, "x8");
    EXPECT_FALSE(armRes->m_consumesArgSlot);
    EXPECT_FALSE(armRes->m_returnReg.has_value());
}

// ============================================================================
// 4. Complete Translation Units (Full ABIs)
// ============================================================================

TEST_F(CallingConvDefLangTest, TestFullSystemVAMD64Definition)
{
    std::string test = R"dsl(
calling_conv SystemV_AMD64 {
    STACK_ALIGN(16);
    STACK_DIRECTION(DOWN);
    STACK_CLEANUP(CALLER);
    SHADOW_SPACE(0);

    STACK_POINTER(GPR:rsp);
    FRAME_POINTER(GPR:rbp);

    CALLEE_SAVED(GPR:rbx, GPR:rsp, GPR:rbp, GPR:r12, GPR:r13, GPR:r14, GPR:r15);
    CALLER_SAVED(GPR:rax, GPR:rcx, GPR:rdx, GPR:rsi, GPR:rdi, GPR:r8, GPR:r9, GPR:r10, GPR:r11,
                 FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3, FPR:xmm4, FPR:xmm5, FPR:xmm6, FPR:xmm7);

    CLASSIFY {
        TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;
        TYPE(f32, f64)                  >> FLOAT;

        AGGREGATE {
            IF_SIZE_GT(16)   >> MEMORY;
            IF_UNALIGNED     >> MEMORY;
            IF_NON_TRIVIAL   >> MEMORY;

            CHUNK_SIZE(8);
            MERGE_PRECEDENCE >> MEMORY > INTEGER > FLOAT;
            ALLOC_POLICY(ALL_OR_NOTHING);
        };
    };

    PASS {
        INTEGER >> REG_SEQ(GPR:rdi, GPR:rsi, GPR:rdx, GPR:rcx, GPR:r8, GPR:r9) >> STACK(ALIGN: 8);
        FLOAT   >> REG_SEQ(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3, FPR:xmm4, FPR:xmm5, FPR:xmm6, FPR:xmm7) >> STACK(ALIGN: 8);
        MEMORY  >> STACK(ALIGN: 8);
    };

    RETURN {
        INTEGER >> REG_SEQ(GPR:rax, GPR:rdx);
        FLOAT   >> REG_SEQ(FPR:xmm0, FPR:xmm1);
        MEMORY  >> SRET;

        SRET_CONFIG {
            PASS_IN_REG(GPR:rdi);
            CONSUMES_ARG_SLOT(true);
            RETURN_REG(GPR:rax);
        };
    };
};
)dsl";

    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res =
            ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile, DSL::Ast::CallingConvDef::CallingConvDefFile>();
    ASSERT_TRUE(res.has_value());

    EXPECT_EQ(res->m_name.m_node, "SystemV_AMD64");
    EXPECT_EQ(res->m_stackAlign.m_node, 16);
    EXPECT_EQ(res->m_stackDirection, DSL::Ast::CallingConvDef::StackDirection::Down);
    EXPECT_EQ(res->m_stackCleanup, DSL::Ast::CallingConvDef::StackCleaner::Caller);
    EXPECT_EQ(res->m_shadowSpace.m_node, 0);

    EXPECT_EQ(res->m_stackPointer.m_regName.m_node, "rsp");
    EXPECT_EQ(res->m_framePointer.m_regName.m_node, "rbp");

    ASSERT_EQ(res->m_calleeSaved.size(), 7);
    EXPECT_EQ(res->m_calleeSaved[0].m_regName.m_node, "rbx");
    ASSERT_EQ(res->m_callerSaved.size(), 17);
    EXPECT_EQ(res->m_callerSaved[0].m_regName.m_node, "rax");

    ASSERT_EQ(res->m_classify.m_primitiveRules.size(), 2);
    ASSERT_TRUE(res->m_classify.m_aggregateDef.has_value());
    ASSERT_EQ(res->m_classify.m_aggregateDef->m_predicates.size(), 3);
    EXPECT_EQ(res->m_classify.m_aggregateDef->m_chunkSize->m_node, 8);

    ASSERT_EQ(res->m_passRules.size(), 3);
    EXPECT_EQ(res->m_passRules[0].m_abiClass.m_node, "INTEGER");
    ASSERT_EQ(res->m_returnRules.size(), 3);
    EXPECT_EQ(res->m_returnRules[2].m_action.m_kind, DSL::Ast::CallingConvDef::LoweringActionKind::Sret);

    ASSERT_TRUE(res->m_sretConfig.has_value());
    EXPECT_EQ(res->m_sretConfig->m_passInReg.m_regName.m_node, "rdi");
    EXPECT_TRUE(res->m_sretConfig->m_consumesArgSlot);
    ASSERT_TRUE(res->m_sretConfig->m_returnReg.has_value());
    EXPECT_EQ(res->m_sretConfig->m_returnReg->m_regName.m_node, "rax");
}

TEST_F(CallingConvDefLangTest, TestFullAAPCS64Definition)
{
    std::string test = R"dsl(
calling_conv AAPCS64 {
    STACK_ALIGN(16);
    STACK_DIRECTION(DOWN);
    STACK_CLEANUP(CALLER);
    SHADOW_SPACE(0);

    STACK_POINTER(GPR:sp);
    FRAME_POINTER(GPR:x29);

    CALLEE_SAVED(GPR:x19, GPR:x20, GPR:x21, GPR:x22, GPR:x23, GPR:x24, GPR:x25, GPR:x26, GPR:x27, GPR:x28, GPR:x29, GPR:x30,
                 FPR:v8, FPR:v9, FPR:v10, FPR:v11, FPR:v12, FPR:v13, FPR:v14, FPR:v15);
    CALLER_SAVED(GPR:x0, GPR:x1, GPR:x2, GPR:x3, GPR:x4, GPR:x5, GPR:x6, GPR:x7,
                 GPR:x9, GPR:x10, GPR:x11, GPR:x12, GPR:x13, GPR:x14, GPR:x15, GPR:x16, GPR:x17, GPR:x18,
                 FPR:v0, FPR:v1, FPR:v2, FPR:v3, FPR:v4, FPR:v5, FPR:v6, FPR:v7);

    CLASSIFY {
        TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;
        TYPE(f32, f64)                  >> FLOAT;

        AGGREGATE {
            IF_HOMOGENEOUS(FLOAT, MAX: 4) >> HFA;
            IF_SIZE_LE(16)                >> INTEGER;
            DEFAULT                       >> BY_REF;
        };
    };

    PASS {
        INTEGER >> REG_SEQ(GPR:x0, GPR:x1, GPR:x2, GPR:x3, GPR:x4, GPR:x5, GPR:x6, GPR:x7) >> STACK(ALIGN: 8);
        FLOAT   >> REG_SEQ(FPR:v0, FPR:v1, FPR:v2, FPR:v3, FPR:v4, FPR:v5, FPR:v6, FPR:v7)   >> STACK(ALIGN: 8);
        HFA     >> EXPAND_TO(FLOAT);
        BY_REF  >> PASS_AS_POINTER >> INTEGER;
    };

    RETURN {
        INTEGER >> REG_SEQ(GPR:x0, GPR:x1);
        FLOAT   >> REG_SEQ(FPR:v0, FPR:v1, FPR:v2, FPR:v3);
        HFA     >> EXPAND_TO(FLOAT);
        BY_REF  >> SRET;

        SRET_CONFIG {
            PASS_IN_REG(GPR:x8);
            CONSUMES_ARG_SLOT(false);
            RETURN_REG(NONE);
        };
    };
};
)dsl";

    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res =
            ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile, DSL::Ast::CallingConvDef::CallingConvDefFile>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "AAPCS64");
    EXPECT_EQ(res->m_stackPointer.m_regName.m_node, "sp");
    EXPECT_EQ(res->m_framePointer.m_regName.m_node, "x29");

    ASSERT_EQ(res->m_passRules.size(), 4);
    EXPECT_EQ(res->m_passRules[2].m_action.m_kind, DSL::Ast::CallingConvDef::LoweringActionKind::ExpandTo);
    EXPECT_EQ(res->m_passRules[3].m_action.m_kind, DSL::Ast::CallingConvDef::LoweringActionKind::PassAsPointer);

    ASSERT_TRUE(res->m_sretConfig.has_value());
    EXPECT_EQ(res->m_sretConfig->m_passInReg.m_regName.m_node, "x8");
    EXPECT_FALSE(res->m_sretConfig->m_consumesArgSlot);
    EXPECT_FALSE(res->m_sretConfig->m_returnReg.has_value());
}

// ============================================================================
// 5. Negative & Error Parsing Tests
// ============================================================================

TEST_F(CallingConvDefLangTest, TestMissingSemicolonInDispatchRuleError)
{
    std::string test = "INTEGER >> REG_SEQ(GPR:rdi, GPR:rsi) >> STACK";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::CallingConvDef::DispatchRule, DSL::Ast::CallingConvDef::DispatchRule>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(CallingConvDefLangTest, TestInvalidStackDirectionError)
{
    std::string test = "STACK_DIRECTION(SIDEWAYS);";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::CallingConvDef::StackDirection, DSL::Ast::CallingConvDef::StackDirection>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(CallingConvDefLangTest, TestUnclosedCallingConvBodyError)
{
    std::string test = R"(
calling_conv BrokenConvention {
    STACK_ALIGN(16);
    STACK_POINTER(GPR:rsp);
)";

    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res =
            ctx.parse<DSL::Parser::CallingConvDef::CallingConvDefFile, DSL::Ast::CallingConvDef::CallingConvDefFile>();
    EXPECT_FALSE(res.has_value());
}