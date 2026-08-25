#include "EzDslTestSuite.h"
#include "Ast/TargetDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/RegisterBankPass.h"

class RegisterBankPassTest : public DslTestSuiteAsGtest
{
  protected:
    std::optional<DSL::Ast::TargetDef::TargetDef> parseTarget(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("test.tdf", source);
        return ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    }
};

// ============================================================================
// 1. Happy Path & Hierarchy Topology Tests
// ============================================================================

TEST_F(RegisterBankPassTest, TestBasicRegisterBankAndClassRegistration)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0),
            rcx(, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_TRUE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));

    // 1. Verify Bank Symbol
    const Symbol *bankSym = table.getSymByName("GPR");
    ASSERT_NE(bankSym, nullptr);
    EXPECT_EQ(bankSym->getType(), SymbolType::RegisterBank);
    EXPECT_TRUE(bankSym->hasFlag(SymbolFlags::IsDefined));

    const auto *bankData = bankSym->getIf<Sema::Symbols::RegisterBankSymbol>();
    ASSERT_NE(bankData, nullptr);
    EXPECT_EQ(bankData->m_name, "GPR");
    ASSERT_EQ(bankData->m_classes.size(), 1);

    // 2. Verify Class Symbol
    const Symbol *classSym = table.getSymById(bankData->m_classes[0]);
    ASSERT_NE(classSym, nullptr);
    EXPECT_EQ(classSym->getType(), SymbolType::RegisterClass);

    const auto *classData = classSym->getIf<Sema::Symbols::RegisterClassSymbol>();
    ASSERT_NE(classData, nullptr);
    EXPECT_EQ(classData->m_name, "gpr64");
    EXPECT_EQ(classData->m_bankId, bankSym->getId());
    ASSERT_EQ(classData->m_registers.size(), 2);

    // 3. Verify Register Symbols
    const Symbol *raxSym = table.getSymByName("rax");
    ASSERT_NE(raxSym, nullptr);
    EXPECT_EQ(raxSym->getType(), SymbolType::Register);

    const auto *raxData = raxSym->getIf<Sema::Symbols::RegisterSymbol>();
    ASSERT_NE(raxData, nullptr);
    EXPECT_EQ(raxData->m_name, "rax");
    EXPECT_EQ(raxData->m_bitSize, 64);
    EXPECT_EQ(raxData->m_bitOffset, 0);
    EXPECT_FALSE(raxData->m_parentId.has_value());
    EXPECT_EQ(raxData->m_primaryClassId, classSym->getId());
}

TEST_F(RegisterBankPassTest, TestMultiTierSubRegisterHierarchyResolution)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0)
        );
        CLASS(gpr32,
            eax(rax, 32, 0)
        );
        CLASS(gpr16,
            ax(eax, 16, 0)
        );
        CLASS(gpr8,
            al(ax, 8, 0),
            ah(ax, 8, 8)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_TRUE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *raxSym = table.getSymByName("rax");
    const Symbol *eaxSym = table.getSymByName("eax");
    const Symbol *axSym = table.getSymByName("ax");
    const Symbol *alSym = table.getSymByName("al");
    const Symbol *ahSym = table.getSymByName("ah");

    ASSERT_NE(raxSym, nullptr);
    ASSERT_NE(eaxSym, nullptr);
    ASSERT_NE(axSym, nullptr);
    ASSERT_NE(alSym, nullptr);
    ASSERT_NE(ahSym, nullptr);

    const auto *raxData = raxSym->getIf<Sema::Symbols::RegisterSymbol>();
    const auto *eaxData = eaxSym->getIf<Sema::Symbols::RegisterSymbol>();
    const auto *axData = axSym->getIf<Sema::Symbols::RegisterSymbol>();
    const auto *alData = alSym->getIf<Sema::Symbols::RegisterSymbol>();
    const auto *ahData = ahSym->getIf<Sema::Symbols::RegisterSymbol>();

    // Verify parent links across multiple sub-register tiers
    EXPECT_FALSE(raxData->m_parentId.has_value());

    ASSERT_TRUE(eaxData->m_parentId.has_value());
    EXPECT_EQ(*eaxData->m_parentId, raxSym->getId());

    ASSERT_TRUE(axData->m_parentId.has_value());
    EXPECT_EQ(*axData->m_parentId, eaxSym->getId());

    ASSERT_TRUE(alData->m_parentId.has_value());
    EXPECT_EQ(*alData->m_parentId, axSym->getId());
    EXPECT_EQ(alData->m_bitOffset, 0);
    EXPECT_EQ(alData->m_bitSize, 8);

    ASSERT_TRUE(ahData->m_parentId.has_value());
    EXPECT_EQ(*ahData->m_parentId, axSym->getId());
    EXPECT_EQ(ahData->m_bitOffset, 8);
    EXPECT_EQ(ahData->m_bitSize, 8);
}

TEST_F(RegisterBankPassTest, TestOrderIndependentParentRegisterResolution)
{
    // Sub-register 'eax' declared in class BEFORE parent register 'rax'
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr32,
            eax(rax, 32, 0)
        );
        CLASS(gpr64,
            rax(, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_TRUE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *raxSym = table.getSymByName("rax");
    const Symbol *eaxSym = table.getSymByName("eax");

    ASSERT_NE(raxSym, nullptr);
    ASSERT_NE(eaxSym, nullptr);

    const auto *eaxData = eaxSym->getIf<Sema::Symbols::RegisterSymbol>();
    ASSERT_TRUE(eaxData->m_parentId.has_value());
    EXPECT_EQ(*eaxData->m_parentId, raxSym->getId());
}

TEST_F(RegisterBankPassTest, TestMultipleDistinctBanks)
{
    std::string test = R"dsl(
target MixedTarget {
    bank GPR {
        CLASS(gpr64,
            r0(, 64, 0),
            r1(, 64, 0)
        );
    };
    bank FPR {
        CLASS(fpr64,
            f0(, 64, 0),
            f1(, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_TRUE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));

    const Symbol *gprBank = table.getSymByName("GPR");
    const Symbol *fprBank = table.getSymByName("FPR");

    ASSERT_NE(gprBank, nullptr);
    ASSERT_NE(fprBank, nullptr);
    EXPECT_NE(gprBank->getId(), fprBank->getId());

    EXPECT_NE(table.getSymByName("r0"), nullptr);
    EXPECT_NE(table.getSymByName("f0"), nullptr);
}

// ============================================================================
// 2. Sub-Register Bit Range & Alignment Invariant Tests
// ============================================================================

TEST_F(RegisterBankPassTest, TestSubRegisterSizeGreaterThanParentFails)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0)
        );
        CLASS(gpr128,
            rax128(rax, 128, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestSubRegisterOffsetPlusSizeExceedsParentFails)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr16,
            ax(, 16, 0)
        );
        CLASS(gpr8,
            bad_byte(ax, 16, 8)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestSubRegisterOffsetPastParentEndFails)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0)
        );
        CLASS(gpr32,
            bad_eax(rax, 32, 64)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 3. Parent Resolution & Reference Invariant Tests
// ============================================================================

TEST_F(RegisterBankPassTest, TestUndefinedParentRegisterFails)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr32,
            eax(non_existent_rax, 32, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestParentIsBankNotRegisterFails)
{
    // Parent references a Bank name instead of a Register
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr32,
            eax(GPR, 32, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 4. Sub-Register Cycle Detection Invariant Tests
// ============================================================================

TEST_F(RegisterBankPassTest, TestSelfReferencingRegisterCycleFails)
{
    std::string test = R"dsl(
target BrokenTarget {
    bank GPR {
        CLASS(gpr64,
            rax(rax, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestDirectTwoNodeRegisterCycleFails)
{
    std::string test = R"dsl(
target BrokenTarget {
    bank GPR {
        CLASS(gpr64,
            r0(r1, 64, 0),
            r1(r0, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestMultiNodeRegisterCycleFails)
{
    std::string test = R"dsl(
target BrokenTarget {
    bank GPR {
        CLASS(gpr64,
            r0(r2, 64, 0),
            r1(r0, 64, 0),
            r2(r1, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestCycleAcrossDifferentRegisterClassesFails)
{
    std::string test = R"dsl(
target BrokenTarget {
    bank GPR {
        CLASS(c1,
            rax(rbx, 64, 0)
        );
        CLASS(c2,
            rbx(rax, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestCycleInSubTreeWithValidSiblingsFails)
{
    std::string test = R"dsl(
target MixedTarget {
    bank GPR {
        CLASS(valid_class,
            rcx(, 64, 0),
            ecx(rcx, 32, 0)
        );
        CLASS(cyclic_class,
            r0(r1, 64, 0),
            r1(r0, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

// ============================================================================
// 5. Duplicate Symbol & Collision Invariant Tests
// ============================================================================

TEST_F(RegisterBankPassTest, TestDuplicateRegisterBankNamesFail)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64, rax(, 64, 0));
    };
    bank GPR {
        CLASS(gpr32, eax(, 32, 0));
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestDuplicateRegisterClassNamesFail)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64, rax(, 64, 0));
        CLASS(gpr64, rbx(, 64, 0));
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestDuplicateRegisterNamesInSameClassFail)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0),
            rax(, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestDuplicateRegisterNamesAcrossClassesFail)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0)
        );
        CLASS(gpr32,
            rax(, 32, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());
    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}

TEST_F(RegisterBankPassTest, TestPreExistingSymbolCollisionFails)
{
    std::string test = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0)
        );
    };
};
)dsl";

    auto ast = parseTarget(test);
    ASSERT_TRUE(ast.has_value());

    SymbolTable table(getAllocator());

    // Pre-declare a symbol with the same name "rax" in root scope
    table.declareSym(nullptr, SymbolFlags::IsDefined, SymbolType::Type, Sema::Symbols::TypeSymbol{}, "rax");

    EXPECT_FALSE(RegisterBankPass::run(getDiagCollector(), &table, &*ast));
}