#include "EzDslCommon.h"
#include "Ast/TargetDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Diagnostics/DiagnosticLogger.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "SourceManager/SourceManager.h"
#include <gtest/gtest.h>

class TargetDefLangTest : public ::testing::Test
{
  public:
    DiagnosticCollector *getDiagCollector() { return m_diagCollector.get(); }

    size_t addSource(const std::string &source, const std::string &content)
    {
        return m_sourceManager->addSourceContent(source, content);
    }

    SourceManager *getSourceManager() { return m_sourceManager.get(); }

    void SetUp() override
    {
        m_sourceManager = std::make_shared<SourceManager>("", &m_resource);
        m_diagLogger = std::make_shared<DiagnosticLogger>(m_sourceManager.get());
        m_diagCollector = std::make_shared<DiagnosticCollector>();

        m_diagCollector->addListener(m_diagLogger.get());
    }

    void TearDown() override {}

  private:
    std::pmr::monotonic_buffer_resource m_resource;
    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<DiagnosticLogger> m_diagLogger;
    std::shared_ptr<SourceManager> m_sourceManager;
};

TEST_F(TargetDefLangTest, TestRootRegisterWithoutParent)
{
    std::string test = "rax(, 64, 0)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "rax");
    EXPECT_TRUE(res->m_parentName.m_node.empty());
    EXPECT_EQ(res->m_size.m_node, 64);
    EXPECT_EQ(res->m_offset.m_node, 0);
}

TEST_F(TargetDefLangTest, TestAliasedSubRegister)
{
    std::string test = "eax(rax, 32, 0)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "eax");
    EXPECT_EQ(res->m_parentName.m_node, "rax");
    EXPECT_EQ(res->m_size.m_node, 32);
    EXPECT_EQ(res->m_offset.m_node, 0);
}

TEST_F(TargetDefLangTest, TestHighByteSubRegisterWithOffset)
{
    std::string test = "ah(ax, 8, 8)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "ah");
    EXPECT_EQ(res->m_parentName.m_node, "ax");
    EXPECT_EQ(res->m_size.m_node, 8);
    EXPECT_EQ(res->m_offset.m_node, 8);
}

TEST_F(TargetDefLangTest, TestEmptyRegisterClass)
{
    std::string test = "CLASS(gpr64);";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterClass, DSL::Ast::TargetDef::TargetRegisterClass>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "gpr64");
    EXPECT_TRUE(res->m_registers.empty());
}

TEST_F(TargetDefLangTest, TestRegisterClassWithRegisters)
{
    std::string test = R"(
CLASS(gpr64,
    rax(, 64, 0),
    rcx(, 64, 0),
    rdx(, 64, 0)
);
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterClass, DSL::Ast::TargetDef::TargetRegisterClass>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "gpr64");
    ASSERT_EQ(res->m_registers.size(), 3);

    EXPECT_EQ(res->m_registers[0].m_name.m_node, "rax");
    EXPECT_TRUE(res->m_registers[0].m_parentName.m_node.empty());
    EXPECT_EQ(res->m_registers[1].m_name.m_node, "rcx");
    EXPECT_EQ(res->m_registers[2].m_name.m_node, "rdx");
}

TEST_F(TargetDefLangTest, TestRegisterBankMultipleClasses)
{
    std::string test = R"(
bank GPR {
    CLASS(gpr64,
        rax(, 64, 0),
        rbx(, 64, 0)
    );
    CLASS(gpr32,
        eax(rax, 32, 0),
        ebx(rbx, 32, 0)
    );
};
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterBank, DSL::Ast::TargetDef::TargetRegisterBank>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "GPR");
    ASSERT_EQ(res->m_classes.size(), 2);

    EXPECT_EQ(res->m_classes[0].m_name.m_node, "gpr64");
    ASSERT_EQ(res->m_classes[0].m_registers.size(), 2);
    EXPECT_EQ(res->m_classes[0].m_registers[0].m_name.m_node, "rax");

    EXPECT_EQ(res->m_classes[1].m_name.m_node, "gpr32");
    ASSERT_EQ(res->m_classes[1].m_registers.size(), 2);
    EXPECT_EQ(res->m_classes[1].m_registers[0].m_name.m_node, "eax");
    EXPECT_EQ(res->m_classes[1].m_registers[0].m_parentName.m_node, "rax");
}

TEST_F(TargetDefLangTest, TestMultipleInclusions)
{
    std::string test = R"(
target RISCV64 {
    include idef "instructions.idf";
    include isel "selection.isf";
    include sched "scheduling.scd";
}
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "RISCV64");
    ASSERT_EQ(res->m_inclusions.size(), 3);

    EXPECT_EQ(res->m_inclusions[0].m_inclusionType.m_node, "idef");
    EXPECT_EQ(res->m_inclusions[0].m_path.m_node, "instructions.idf");

    EXPECT_EQ(res->m_inclusions[1].m_inclusionType.m_node, "isel");
    EXPECT_EQ(res->m_inclusions[1].m_path.m_node, "selection.isf");

    EXPECT_EQ(res->m_inclusions[2].m_inclusionType.m_node, "sched");
    EXPECT_EQ(res->m_inclusions[2].m_path.m_node, "scheduling.scd");

    EXPECT_TRUE(res->m_regBanks.empty());
}

TEST_F(TargetDefLangTest, TestCompleteTargetDefinitionInterleaved)
{
    std::string test = R"(
target x86_64 {
    include idef "x86_insts.idf";

    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0),
            rcx(, 64, 0)
        );
        CLASS(gpr32,
            eax(rax, 32, 0),
            ecx(rcx, 32, 0)
        );
    };

    include isel "x86_patterns.isf";

    bank FPR {
        CLASS(fpr64,
            xmm0(, 64, 0),
            xmm1(, 64, 0)
        );
    };
};
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_name.m_node, "x86_64");

    // Inclusions validation
    ASSERT_EQ(res->m_inclusions.size(), 2);
    EXPECT_EQ(res->m_inclusions[0].m_inclusionType.m_node, "idef");
    EXPECT_EQ(res->m_inclusions[0].m_path.m_node, "x86_insts.idf");
    EXPECT_EQ(res->m_inclusions[1].m_inclusionType.m_node, "isel");
    EXPECT_EQ(res->m_inclusions[1].m_path.m_node, "x86_patterns.isf");

    // Register banks validation
    ASSERT_EQ(res->m_regBanks.size(), 2);
    EXPECT_EQ(res->m_regBanks[0].m_name.m_node, "GPR");
    ASSERT_EQ(res->m_regBanks[0].m_classes.size(), 2);
    EXPECT_EQ(res->m_regBanks[0].m_classes[0].m_name.m_node, "gpr64");
    EXPECT_EQ(res->m_regBanks[0].m_classes[1].m_name.m_node, "gpr32");

    EXPECT_EQ(res->m_regBanks[1].m_name.m_node, "FPR");
    ASSERT_EQ(res->m_regBanks[1].m_classes.size(), 1);
    EXPECT_EQ(res->m_regBanks[1].m_classes[0].m_name.m_node, "fpr64");
}

TEST_F(TargetDefLangTest, TestEmptyTargetBodyFails)
{
    std::string test = R"(
target MyTarget {
}
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(TargetDefLangTest, TestMalformedRegisterMissingOffset)
{
    std::string test = "rax(, 64)"; // Missing comma and offset parameter
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegister, DSL::Ast::TargetDef::TargetRegister>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(TargetDefLangTest, TestMalformedClassMissingSemicolon)
{
    std::string test = R"(
CLASS(gpr64,
    rax(, 64, 0)
)
)"; // Missing closing semicolon
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetRegisterClass, DSL::Ast::TargetDef::TargetRegisterClass>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(TargetDefLangTest, TestUnknownBodyItemInTarget)
{
    std::string test = R"(
target MyTarget {
    foo bar "file.idf";
}
)";
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    EXPECT_FALSE(res.has_value());
}

TEST_F(TargetDefLangTest, TestUnterminatedTargetDef)
{
    std::string test = R"(
target MyTarget {
    include idef "file.idf";
)"; // Missing closing brace '}'
    size_t sourceId = addSource("test", test);
    ParseContext ctx(getDiagCollector(), getSourceManager(), sourceId);

    auto res = ctx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    EXPECT_FALSE(res.has_value());
}