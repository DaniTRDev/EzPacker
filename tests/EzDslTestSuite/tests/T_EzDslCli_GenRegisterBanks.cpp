#include "EzDslTestSuite.h"
#include "Ast/TargetDefLangAst.h"
#include "CodeGenerators/CppTargetBankGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/RegisterBankPass.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <thread>

/**
 * Test fixture for target register bank, register class, and register model code generator (CppTargetBankGenerator).
 * Verifies code generation, namespace scoping, overlap matrix synthesis, sub/super-register tables,
 * working mode isolation, write-if-changed timestamp caching, and null safety.
 */
class TargetBankGeneratorTest : public DslTestSuiteAsGtest
{
  protected:
    /**
     * Initializes test environment and creates a temporary sandbox directory for generated code.
     */
    void SetUp() override
    {
        DslTestSuiteAsGtest::SetUp();

        m_tempDir =
                std::filesystem::temp_directory_path() / ("ezdsl_bank_test_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(m_tempDir);

        getDiagCollector()->trace("TargetBankGeneratorTest", "Testing dir at: {}", m_tempDir.string());
    }

    /**
     * Cleans up the temporary sandbox directory and tears down the test environment.
     */
    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(m_tempDir, ec);

        DslTestSuiteAsGtest::TearDown();
    }

    /**
     * Reads and returns the entire contents of a file on disk as a string.
     */
    std::string readFile(const std::filesystem::path &filePath) const
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return {};
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::filesystem::path m_tempDir;
};

// ============================================================================
// 1. Full Register Bank Generation & Content Verification
// ============================================================================

/**
 * Verifies end-to-end code generation of <Target>RegisterBanks.h and <Target>RegisterBanks.cpp
 * for a target architecture with multi-tier register hierarchies (rax -> eax -> ax -> al/ah),
 * checking target namespace encapsulation, TargetReg enum, overlap matrix, sub-registers, and model methods.
 */
TEST_F(TargetBankGeneratorTest, GeneratesRegisterBanksWithSubRegisterHierarchies)
{
    std::string dslContent = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0),
            rcx(, 64, 0)
        );
        CLASS(gpr32,
            eax(rax, 32, 0),
            ecx(rcx, 32, 0)
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

    // 1. Parse .tdf DSL
    ParseContext parseCtx = createParseContextFromBuff("x86_64.tdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(ast.has_value());

    // 2. Run RegisterBankPass
    SymbolTable symTable(getAllocator());
    RegisterBankPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    // 3. Generate Header and Source Files
    bool success = CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                               &symTable,
                                                               m_tempDir,
                                                               "x86_64",
                                                               CodeGenerators::TargetBankGenWorkingMode::Full);
    ASSERT_TRUE(success);

    auto headerPath = m_tempDir / "x86_64RegisterBanks.h";
    auto sourcePath = m_tempDir / "x86_64RegisterBanks.cpp";

    ASSERT_TRUE(std::filesystem::exists(headerPath));
    ASSERT_TRUE(std::filesystem::exists(sourcePath));

    std::string headerContent = readFile(headerPath);
    std::string sourceContent = readFile(sourcePath);

    // Verify namespace encapsulation
    EXPECT_NE(headerContent.find("namespace x86_64"), std::string::npos);
    EXPECT_NE(sourceContent.find("namespace x86_64"), std::string::npos);

    // Verify TargetReg enum definition
    EXPECT_NE(headerContent.find("enum class TargetReg : uint16_t"), std::string::npos);
    EXPECT_NE(headerContent.find("NoRegister = 0,"), std::string::npos);
    EXPECT_NE(headerContent.find("rax,"), std::string::npos);
    EXPECT_NE(headerContent.find("eax,"), std::string::npos);
    EXPECT_NE(headerContent.find("ax,"), std::string::npos);
    EXPECT_NE(headerContent.find("al,"), std::string::npos);
    EXPECT_NE(headerContent.find("ah,"), std::string::npos);
    EXPECT_NE(headerContent.find("rcx,"), std::string::npos);
    EXPECT_NE(headerContent.find("ecx,"), std::string::npos);
    EXPECT_NE(headerContent.find("TARGET_REG_COUNT"), std::string::npos);

    // Verify function declarations in header
    EXPECT_NE(headerContent.find("const char *GetRegisterName(TargetReg reg);"), std::string::npos);
    EXPECT_NE(headerContent.find("TargetReg GetRegisterByName(std::string_view name);"), std::string::npos);
    EXPECT_NE(headerContent.find("bool TargetRegistersOverlap(TargetReg regA, TargetReg regB);"), std::string::npos);
    EXPECT_NE(headerContent.find("const std::vector<TargetReg> &GetSubRegisters(TargetReg reg);"), std::string::npos);
    EXPECT_NE(headerContent.find("const std::vector<TargetReg> &GetSuperRegisters(TargetReg reg);"), std::string::npos);

    // Verify Model Class declarations
    EXPECT_NE(headerContent.find("class x86_64RegisterBanks"), std::string::npos);
    EXPECT_NE(headerContent.find("void initialize();"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterBank *getBank(std::string_view name) const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterClass *getClass(std::string_view name) const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterDescriptor *getRegisterDescriptor(TargetReg reg) const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterRef getRegisterRef(TargetReg reg) const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterBank *getBank_GPR() const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterClass *getClass_gpr64() const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterClass *getClass_gpr32() const;"), std::string::npos);
    EXPECT_NE(headerContent.find("std::pmr::vector<MirRegisterBank *> CreateRegisterBanks(std::pmr::memory_resource *alloc);"),
              std::string::npos);

    // Verify Source implementations
    EXPECT_NE(sourceContent.find("s_OverlapTable"), std::string::npos);
    EXPECT_NE(sourceContent.find("s_SubRegs"), std::string::npos);
    EXPECT_NE(sourceContent.find("s_SuperRegs"), std::string::npos);
    EXPECT_NE(sourceContent.find("const char *GetRegisterName(TargetReg reg)"), std::string::npos);
    EXPECT_NE(sourceContent.find("TargetReg GetRegisterByName(std::string_view name)"), std::string::npos);
    EXPECT_NE(sourceContent.find("bool TargetRegistersOverlap(TargetReg regA, TargetReg regB)"), std::string::npos);
    EXPECT_NE(sourceContent.find("void x86_64RegisterBanks::initialize()"), std::string::npos);
    EXPECT_NE(sourceContent.find("cls_gpr64->addRegister(\"rax\", 64, 0, {});"), std::string::npos);
    EXPECT_NE(sourceContent.find("cls_gpr8->addRegister(\"al\", 8, 0, {});"), std::string::npos);
    EXPECT_NE(sourceContent.find("cls_gpr8->addRegister(\"ah\", 8, 8, {});"), std::string::npos);
}

// ============================================================================
// 2. Multi-Bank Target Architecture Generation
// ============================================================================

/**
 * Verifies code generation for an architecture with multiple distinct register banks (e.g. GPR and FPR).
 */
TEST_F(TargetBankGeneratorTest, GeneratesMultiBankTarget)
{
    std::string dslContent = R"dsl(
target AMD64 {
    bank GPR {
        CLASS(GPR64,
            r0(, 64, 0),
            r1(, 64, 0)
        );
    };
    bank FPR {
        CLASS(FPR64,
            f0(, 64, 0),
            f1(, 64, 0)
        );
    };
};
)dsl";

    ParseContext parseCtx = createParseContextFromBuff("amd64.tdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(ast.has_value());

    SymbolTable symTable(getAllocator());
    RegisterBankPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    bool success = CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                               &symTable,
                                                               m_tempDir,
                                                               "AMD64",
                                                               CodeGenerators::TargetBankGenWorkingMode::Full);
    ASSERT_TRUE(success);

    auto headerPath = m_tempDir / "AMD64RegisterBanks.h";
    ASSERT_TRUE(std::filesystem::exists(headerPath));

    std::string headerContent = readFile(headerPath);
    EXPECT_NE(headerContent.find("namespace AMD64"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterBank *getBank_GPR() const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterBank *getBank_FPR() const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterClass *getClass_GPR64() const;"), std::string::npos);
    EXPECT_NE(headerContent.find("MirRegisterClass *getClass_FPR64() const;"), std::string::npos);
}

// ============================================================================
// 3. Working Mode Isolation (Header-Only / Source-Only)
// ============================================================================

/**
 * Verifies Header-Only generation mode, creating only <Target>RegisterBanks.h.
 */
TEST_F(TargetBankGeneratorTest, GeneratesHeaderOnlyWhenRequested)
{
    std::string dslContent = R"dsl(
target RiscV64 {
    bank GPR {
        CLASS(GPR64,
            x0(, 64, 0)
        );
    };
};
)dsl";

    ParseContext parseCtx = createParseContextFromBuff("riscv.tdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(ast.has_value());

    SymbolTable symTable(getAllocator());
    RegisterBankPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    bool success = CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                               &symTable,
                                                               m_tempDir,
                                                               "RiscV64",
                                                               CodeGenerators::TargetBankGenWorkingMode::Header);
    ASSERT_TRUE(success);

    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "RiscV64RegisterBanks.h"));
    EXPECT_FALSE(std::filesystem::exists(m_tempDir / "RiscV64RegisterBanks.cpp"));
}

/**
 * Verifies Source-Only generation mode, creating only <Target>RegisterBanks.cpp.
 */
TEST_F(TargetBankGeneratorTest, GeneratesSourceOnlyWhenRequested)
{
    std::string dslContent = R"dsl(
target RiscV64 {
    bank GPR {
        CLASS(GPR64,
            x0(, 64, 0)
        );
    };
};
)dsl";

    ParseContext parseCtx = createParseContextFromBuff("riscv.tdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(ast.has_value());

    SymbolTable symTable(getAllocator());
    RegisterBankPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    bool success = CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                               &symTable,
                                                               m_tempDir,
                                                               "RiscV64",
                                                               CodeGenerators::TargetBankGenWorkingMode::Source);
    ASSERT_TRUE(success);

    EXPECT_FALSE(std::filesystem::exists(m_tempDir / "RiscV64RegisterBanks.h"));
    EXPECT_TRUE(std::filesystem::exists(m_tempDir / "RiscV64RegisterBanks.cpp"));
}

// ============================================================================
// 4. Direct File Path Output Specification
// ============================================================================

/**
 * Verifies code generation when targeting an explicit file path rather than a directory.
 */
TEST_F(TargetBankGeneratorTest, GeneratesToExplicitFilePath)
{
    std::string dslContent = R"dsl(
target ARM64 {
    bank GPR {
        CLASS(GPR64,
            x0(, 64, 0)
        );
    };
};
)dsl";

    ParseContext parseCtx = createParseContextFromBuff("arm64.tdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(ast.has_value());

    SymbolTable symTable(getAllocator());
    RegisterBankPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    auto customHeaderPath = m_tempDir / "CustomArmRegs.h";
    bool success = CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                               &symTable,
                                                               customHeaderPath,
                                                               "ARM64",
                                                               CodeGenerators::TargetBankGenWorkingMode::Full);
    ASSERT_TRUE(success);
    ASSERT_TRUE(std::filesystem::exists(customHeaderPath));
    ASSERT_TRUE(std::filesystem::exists(m_tempDir / "CustomArmRegs.cpp"));
}

// ============================================================================
// 5. Incremental Build: Write-If-Changed Verification
// ============================================================================

/**
 * Verifies write-if-changed optimization ensuring timestamps remain untouched when contents are identical.
 */
TEST_F(TargetBankGeneratorTest, PreservesTimestampWhenContentIsUnchanged)
{
    std::string dslContent = R"dsl(
target x86_64 {
    bank GPR {
        CLASS(gpr64,
            rax(, 64, 0)
        );
    };
};
)dsl";

    ParseContext parseCtx = createParseContextFromBuff("x86_64.tdf", dslContent);
    auto ast = parseCtx.parse<DSL::Parser::TargetDef::TargetDef, DSL::Ast::TargetDef::TargetDef>();
    ASSERT_TRUE(ast.has_value());

    SymbolTable symTable(getAllocator());
    RegisterBankPass pass;
    ASSERT_TRUE(pass.run(getDiagCollector(), &symTable, &*ast));

    auto targetPath = m_tempDir / "x86_64RegisterBanks.h";

    // First generation
    ASSERT_TRUE(CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                            &symTable,
                                                            m_tempDir,
                                                            "x86_64",
                                                            CodeGenerators::TargetBankGenWorkingMode::Full));
    ASSERT_TRUE(std::filesystem::exists(targetPath));
    auto initialTimestamp = std::filesystem::last_write_time(targetPath);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Second generation
    ASSERT_TRUE(CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                            &symTable,
                                                            m_tempDir,
                                                            "x86_64",
                                                            CodeGenerators::TargetBankGenWorkingMode::Full));
    auto secondTimestamp = std::filesystem::last_write_time(targetPath);

    EXPECT_EQ(initialTimestamp, secondTimestamp);
}

// ============================================================================
// 6. Error Handling & Null Safety Tests
// ============================================================================

/**
 * Verifies that code generator handles an empty symbol table gracefully.
 */
TEST_F(TargetBankGeneratorTest, HandlesEmptySymbolTableGracefully)
{
    SymbolTable emptyTable(getAllocator());

    bool success = CodeGenerators::GenerateTargetRegisterBanks(getDiagCollector(),
                                                               &emptyTable,
                                                               m_tempDir,
                                                               "EmptyTarget",
                                                               CodeGenerators::TargetBankGenWorkingMode::Full);
    ASSERT_TRUE(success);

    auto headerPath = m_tempDir / "EmptyTargetRegisterBanks.h";
    ASSERT_TRUE(std::filesystem::exists(headerPath));

    std::string content = readFile(headerPath);
    EXPECT_NE(content.find("namespace EmptyTarget"), std::string::npos);
    EXPECT_NE(content.find("TARGET_REG_COUNT"), std::string::npos);
}

/**
 * Verifies that the generator rejects null pointers for diagnostics or symbol table inputs.
 */
TEST_F(TargetBankGeneratorTest, FailsGracefullyOnNullInputs)
{
    DiagnosticCollector collector;
    SymbolTable symTable(getAllocator());

    EXPECT_FALSE(CodeGenerators::GenerateTargetRegisterBanks(&collector, nullptr, m_tempDir));
    EXPECT_FALSE(CodeGenerators::GenerateTargetRegisterBanks(nullptr, &symTable, m_tempDir));
}
