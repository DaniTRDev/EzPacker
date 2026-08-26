#include "EzDslTestSuite.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/CallingConvDefLang.h"
#include "Parser/ParseContext.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/CallingConvPass.h"

class CallingConvPassTest : public DslTestSuiteAsGtest
{
  protected:
    std::unique_ptr<SymbolTable> m_table;

    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();
        m_table = std::make_unique<SymbolTable>(getAllocator());

        setupTargetEnvironment();
        m_table->enterScope("TargetScope");
    }

    void registerType(std::string_view name,
                      uint32_t bitWidth,
                      DSL::Ast::TypeDef::TypeKind kind = DSL::Ast::TypeDef::TypeKind::Integer)
    {
        Sema::Symbols::TypeSymbol typeSym{ .m_name = name, .m_kind = kind, .m_bitWidth = bitWidth };
        m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Type, typeSym, name);
    }

    SymbolId registerRegisterClass(std::string_view name)
    {
        Sema::Symbols::RegisterClassSymbol classSym{ .m_name = name,
                                                     .m_bankId = InvalidSymbolId,
                                                     .m_registers = std::pmr::vector<SymbolId>{ getAllocator() } };
        return m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::RegisterClass, classSym, name);
    }

    void registerRegister(std::string_view name, SymbolId classSymId, uint32_t bitSize = 64)
    {
        Sema::Symbols::RegisterSymbol regSym{ .m_name = name,
                                              .m_parentId = std::nullopt,
                                              .m_bitSize = bitSize,
                                              .m_bitOffset = 0,
                                              .m_primaryClassId = classSymId };
        SymbolId regId = m_table->declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Register, regSym, name);

        Symbol *classSymbol = m_table->getSymById(classSymId);
        if (classSymbol)
        {
            if (auto *classData = classSymbol->getIf<Sema::Symbols::RegisterClassSymbol>())
            {
                classData->m_registers.push_back(regId);
            }
        }
    }

    void setupTargetEnvironment()
    {
        // 1. Primitive Scalar & Pointer Types
        registerType("i1", 1);
        registerType("i8", 8);
        registerType("i16", 16);
        registerType("i32", 32);
        registerType("i64", 64);
        registerType("ptr", 64);
        registerType("f32", 32, DSL::Ast::TypeDef::TypeKind::FloatingPoint);
        registerType("f64", 64, DSL::Ast::TypeDef::TypeKind::FloatingPoint);

        // 2. Hardware Register Classes
        SymbolId gprId = registerRegisterClass("GPR");
        SymbolId fprId = registerRegisterClass("FPR");

        // 3. General Purpose Registers (x86_64 style)
        registerRegister("rax", gprId);
        registerRegister("rcx", gprId);
        registerRegister("rdx", gprId);
        registerRegister("rbx", gprId);
        registerRegister("rsp", gprId);
        registerRegister("rbp", gprId);
        registerRegister("rsi", gprId);
        registerRegister("rdi", gprId);
        registerRegister("r8", gprId);
        registerRegister("r9", gprId);
        registerRegister("r10", gprId);
        registerRegister("r11", gprId);
        registerRegister("r12", gprId);
        registerRegister("r13", gprId);
        registerRegister("r14", gprId);
        registerRegister("r15", gprId);

        // 4. Floating Point Registers
        registerRegister("xmm0", fprId);
        registerRegister("xmm1", fprId);
        registerRegister("xmm2", fprId);
        registerRegister("xmm3", fprId);
        registerRegister("xmm4", fprId);
        registerRegister("xmm5", fprId);
        registerRegister("xmm6", fprId);
        registerRegister("xmm7", fprId);
    }

    std::optional<DSL::Ast::CallingConvDef::CallingConvDefFile> parseFile(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff(std::format("CallingConvPassTest_{}.ccdf", m_parsedNum), source);
        m_parsedNum++;

        return ctx
                .parse<DSL::Parser::CallingConvDef::CallingConvDefFile, DSL::Ast::CallingConvDef::CallingConvDefFile>();
    }

    bool runPass(const std::string &source)
    {
        auto ast = parseFile(source);
        if (!ast.has_value())
        {
            return false;
        }
        return CallingConvPass::run(getDiagCollector(), m_table.get(), &ast.value());
    }

  private:
    size_t m_parsedNum{ 0 };
};

// ============================================================================
// 1. Full Real-World Calling Convention Validation
// ============================================================================

TEST_F(CallingConvPassTest, TestSystemVAMD64CallingConvention)
{
    std::string code = R"dsl(
calling_conv SysV64 {
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
        TYPE(f32, f64) >> FLOAT;

        AGGREGATE {
            IF_SIZE_LE(16) >> INTEGER;
            IF_HOMOGENEOUS(FLOAT, MAX: 4) >> HFA;
            IF_NON_TRIVIAL >> MEMORY;
            DEFAULT >> MEMORY;
            CHUNK_SIZE(8);
            MERGE_PRECEDENCE >> MEMORY > INTEGER > FLOAT;
            ALLOC_POLICY(ALL_OR_NOTHING);
        };
    };

    PASS {
        INTEGER >> REG_SEQ(GPR:rdi, GPR:rsi, GPR:rdx, GPR:rcx, GPR:r8, GPR:r9) >> STACK(ALIGN: 8);
        FLOAT   >> REG_SEQ(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3, FPR:xmm4, FPR:xmm5, FPR:xmm6, FPR:xmm7) >> STACK(8);
        HFA     >> EXPAND_TO(FLOAT);
        MEMORY  >> STACK(16);
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

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SysV64");
    ASSERT_NE(sym, nullptr);
    EXPECT_EQ(sym->getType(), SymbolType::CallingConv);

    const auto *ccData = sym->getIf<Sema::Symbols::CallingConvSymbol>();
    ASSERT_NE(ccData, nullptr);

    // Layout configuration
    EXPECT_EQ(ccData->m_name, "SysV64");
    EXPECT_EQ(ccData->m_stackAlign, 16u);
    EXPECT_EQ(ccData->m_stackDirection, DSL::Ast::CallingConvDef::StackDirection::Down);
    EXPECT_EQ(ccData->m_stackCleanup, DSL::Ast::CallingConvDef::StackCleaner::Caller);
    EXPECT_EQ(ccData->m_shadowSpace, 0u);

    // Stack & Frame pointers
    EXPECT_NE(ccData->m_stackPointer.m_registerId, InvalidSymbolId);
    EXPECT_NE(ccData->m_framePointer.m_registerId, InvalidSymbolId);

    // Preservation lists
    EXPECT_EQ(ccData->m_calleeSaved.size(), 7);
    EXPECT_EQ(ccData->m_callerSaved.size(), 17);

    // Classification
    EXPECT_EQ(ccData->m_primitiveRules.size(), 8);
    ASSERT_TRUE(ccData->m_aggregateDef.has_value());
    EXPECT_EQ(ccData->m_aggregateDef->m_chunkSize, 8);
    EXPECT_EQ(ccData->m_aggregateDef->m_predicates.size(), 4);
    EXPECT_EQ(ccData->m_aggregateDef->m_mergePrecedence.size(), 3);

    // Dispatch rules
    EXPECT_EQ(ccData->m_passRules.size(), 4);
    EXPECT_EQ(ccData->m_returnRules.size(), 3);

    // SRET Configuration
    ASSERT_TRUE(ccData->m_sretConfig.has_value());
    EXPECT_TRUE(ccData->m_sretConfig->m_consumesArgSlot);
    ASSERT_TRUE(ccData->m_sretConfig->m_returnReg.has_value());
}

TEST_F(CallingConvPassTest, TestWindowsX64CallingConvention)
{
    std::string code = R"dsl(
calling_conv Win64 {
    STACK_ALIGN(16);
    STACK_DIRECTION(DOWN);
    STACK_CLEANUP(CALLER);
    SHADOW_SPACE(32);

    STACK_POINTER(GPR:rsp);
    FRAME_POINTER(GPR:rbp);

    CALLEE_SAVED(GPR:rbx, GPR:rbp, GPR:rdi, GPR:rsi, GPR:r12, GPR:r13, GPR:r14, GPR:r15,
                 FPR:xmm6, FPR:xmm7);
    CALLER_SAVED(GPR:rax, GPR:rcx, GPR:rdx, GPR:r8, GPR:r9, GPR:r10, GPR:r11,
                 FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3, FPR:xmm4, FPR:xmm5);

    CLASSIFY {
        TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;
        TYPE(f32, f64) >> FLOAT;

        AGGREGATE {
            IF_SIZE_IN(1, 2, 4, 8) >> INTEGER;
            DEFAULT >> BY_REF;
            ALLOC_POLICY(INDEPENDENT);
        };
    };

    PASS {
        INTEGER >> REG_SLOTS(GPR:rcx, GPR:rdx, GPR:r8, GPR:r9) >> STACK(ALIGN: 8);
        FLOAT   >> REG_SLOTS(FPR:xmm0, FPR:xmm1, FPR:xmm2, FPR:xmm3) >> STACK(8);
        BY_REF  >> PASS_AS_POINTER >> INTEGER;
    };

    RETURN {
        INTEGER >> REG_SEQ(GPR:rax);
        FLOAT   >> REG_SEQ(FPR:xmm0);

        SRET_CONFIG {
            PASS_IN_REG(GPR:rcx);
            CONSUMES_ARG_SLOT(true);
            RETURN_REG(GPR:rax);
        };
    };
};
)dsl";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("Win64");
    ASSERT_NE(sym, nullptr);

    const auto *ccData = sym->getIf<Sema::Symbols::CallingConvSymbol>();
    ASSERT_NE(ccData, nullptr);
    EXPECT_EQ(ccData->m_shadowSpace, 32u);
    ASSERT_EQ(ccData->m_passRules.size(), 3);
    EXPECT_EQ(ccData->m_passRules[0].m_action.m_regAssignKind, DSL::Ast::CallingConvDef::RegAssignKind::Slots);
}

// ============================================================================
// 2. Stack Layout & Directive Error Validation
// ============================================================================

TEST_F(CallingConvPassTest, TestErrorNonPowerOfTwoStackAlign)
{
    std::string code = R"dsl(
calling_conv InvalidAlign {
    STACK_ALIGN(12); // Invalid: not a power of 2
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorNegativeStackAlign)
{
    std::string code = R"dsl(
calling_conv InvalidAlign {
    STACK_ALIGN(-2);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorNegativeShadowSpace)
{
    std::string code = R"dsl(
calling_conv NegativeShadow {
    SHADOW_SPACE(-16);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

// ============================================================================
// 3. Register Resolution & Preservation Constraints
// ============================================================================

TEST_F(CallingConvPassTest, TestErrorUnknownRegisterClassInStackPointer)
{
    std::string code = R"dsl(
calling_conv UnknownClass {
    STACK_POINTER(UNKNOWN_CLASS:rsp);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorUnknownRegisterInFramePointer)
{
    std::string code = R"dsl(
calling_conv UnknownReg {
    FRAME_POINTER(GPR:non_existent_reg);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorRegisterNotMemberOfSpecifiedClass)
{
    // xmm0 is in FPR, not GPR
    std::string code = R"dsl(
calling_conv ClassMismatch {
    STACK_POINTER(GPR:xmm0);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorDuplicateCalleeSavedRegister)
{
    std::string code = R"dsl(
calling_conv DuplicateCallee {
    CALLEE_SAVED(GPR:rbx, GPR:r12, GPR:rbx);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorDuplicateCallerSavedRegister)
{
    std::string code = R"dsl(
calling_conv DuplicateCaller {
    CALLER_SAVED(GPR:rax, GPR:rcx, GPR:rax);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorCalleeAndCallerSavedRegisterOverlap)
{
    // rbx cannot be both CALLEE_SAVED and CALLER_SAVED
    std::string code = R"dsl(
calling_conv OverlappingPreservation {
    CALLEE_SAVED(GPR:rbx, GPR:r12);
    CALLER_SAVED(GPR:rax, GPR:rbx);
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

// ============================================================================
// 4. Type & Aggregate Classification Semantic Invariants
// ============================================================================

TEST_F(CallingConvPassTest, TestErrorUnknownTypeInTypeClassification)
{
    std::string code = R"dsl(
calling_conv UnknownTypeClassify {
    CLASSIFY {
        TYPE(i32, unknown_int_type) >> INTEGER;
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorDuplicateTypeClassification)
{
    // i32 is classified twice
    std::string code = R"dsl(
calling_conv DuplicateTypeClassify {
    CLASSIFY {
        TYPE(i32, i64) >> INTEGER;
        TYPE(i32) >> FLOAT;
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorNonPowerOfTwoChunkSize)
{
    std::string code = R"dsl(
calling_conv BadChunkSize {
    CLASSIFY {
        AGGREGATE {
            CHUNK_SIZE(6); // Invalid: not a power of 2
        };
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorNegativeAggregateSizeThreshold)
{
    std::string code = R"dsl(
calling_conv NegativeSizePred {
    CLASSIFY {
        AGGREGATE {
            IF_SIZE_GT(-8) >> MEMORY;
        };
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorNonPositiveHomogeneousMaxElements)
{
    std::string code = R"dsl(
calling_conv BadHomogeneousMax {
    CLASSIFY {
        AGGREGATE {
            IF_HOMOGENEOUS(FLOAT, MAX: 0) >> HFA;
        };
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

// ============================================================================
// 5. Dispatch & Lowering Action Semantic Invariants
// ============================================================================

TEST_F(CallingConvPassTest, TestErrorDuplicatePassDispatchRuleForAbiClass)
{
    std::string code = R"dsl(
calling_conv DuplicatePassRule {
    PASS {
        INTEGER >> REG_SEQ(GPR:rdi);
        INTEGER >> REG_SEQ(GPR:rsi);
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorDuplicateReturnDispatchRuleForAbiClass)
{
    std::string code = R"dsl(
calling_conv DuplicateReturnRule {
    RETURN {
        INTEGER >> REG_SEQ(GPR:rax);
        INTEGER >> REG_SEQ(GPR:rdx);
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorEmptyRegisterSequence)
{
    std::string code = R"dsl(
calling_conv EmptyRegSeq {
    PASS {
        INTEGER >> REG_SEQ();
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorNonPowerOfTwoStackPlacementAlignment)
{
    std::string code = R"dsl(
calling_conv BadStackPlacementAlign {
    PASS {
        INTEGER >> REG_SEQ(GPR:rdi) >> STACK(ALIGN: 10);
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorEmptyTargetClassInExpandTo)
{
    std::string code = R"dsl(
calling_conv EmptyExpandToClass {
    PASS {
        HFA >> EXPAND_TO();
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

// ============================================================================
// 6. SRET Configuration & Symbol Redefinition Tests
// ============================================================================

TEST_F(CallingConvPassTest, TestValidSretConfigWithNoneReturn)
{
    std::string code = R"dsl(
calling_conv SretNoneReturn {
    RETURN {
        SRET_CONFIG {
            PASS_IN_REG(GPR:rdi);
            CONSUMES_ARG_SLOT(false);
            RETURN_REG(NONE);
        };
    };
};
)dsl";

    ASSERT_TRUE(runPass(code));

    Symbol *sym = m_table->getSymByName("SretNoneReturn");
    ASSERT_NE(sym, nullptr);

    const auto *ccData = sym->getIf<Sema::Symbols::CallingConvSymbol>();
    ASSERT_NE(ccData, nullptr);
    ASSERT_TRUE(ccData->m_sretConfig.has_value());
    EXPECT_FALSE(ccData->m_sretConfig->m_consumesArgSlot);
    EXPECT_FALSE(ccData->m_sretConfig->m_returnReg.has_value());
}

TEST_F(CallingConvPassTest, TestErrorUnknownRegisterInSretConfig)
{
    std::string code = R"dsl(
calling_conv UnknownSretReg {
    RETURN {
        SRET_CONFIG {
            PASS_IN_REG(GPR:unknown_reg);
        };
    };
};
)dsl";

    EXPECT_FALSE(runPass(code));
}

TEST_F(CallingConvPassTest, TestErrorRedefinitionOfCallingConvention)
{
    std::string code = R"dsl(
calling_conv CDecl {
    STACK_ALIGN(16);
};
)dsl";

    ASSERT_TRUE(runPass(code));

    // Running again with identical symbol name in the same scope fails
    EXPECT_FALSE(runPass(code));
}