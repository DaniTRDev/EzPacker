#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/TargetInstDefLangAst.h"
#include "CodeGenerators/CppEncodingTableGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetInstDefLang.h"
#include "Sema/Encoding/EncodingDialect.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetSymbols.h"
#include "SemaPasses/TargetInstPass.h"

#include <filesystem>

using namespace CodeGenerators;

namespace
{

/**
 * Test-only dialect/backend pair demonstrating that a new ISA requires only a dialect
 * plus a codegen backend, with no edits to the generic parser, sema or generator.
 */
class StubEncodingDialect : public Sema::Encoding::EncodingDialect
{
  public:
    std::string_view name() const override { return "stub"; }

    bool validate(const DSL::Ast::Encoding::EncodingDecl &encoding,
                  const DSL::Ast::TargetInstDef::TargetInstDecl & /*inst*/,
                  DiagnosticCollector * /*diag*/) override
    {
        return !encoding.m_directives.empty();
    }
};

class StubEncodingBackend : public EncodingCodegenBackend
{
  public:
    std::string includeHeader() const override { return "Stub/StubEncodingDesc.h"; }
    std::string namespaceName() const override { return "Stub::Encoding"; }
    std::string arrayType() const override { return "StubDesc"; }
    std::string row(const Symbols::TargetInstructionSymbol & /*sym*/) const override { return "StubDesc{}"; }
};

} // namespace

/**
 * Fixture for generating table-driven encoding descriptors from target-instruction (.idf) sources.
 */
class CppTargetEncodingGeneratorTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
  protected:
    // Parses target-instruction source and runs the target-instruction semantic pass.
    bool parseAndRunPass(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("target_enc_test", source);
        auto ast = ctx.parse<DSL::Parser::TargetInstDef::TargetInstFile, DSL::Ast::TargetInstDef::TargetInstFile>();
        if (!ast.has_value())
        {
            return false;
        }

        return TargetInstPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    }
};

// Generates encoding descriptors and verifies RR/RM forms, REX/SSE fields, coalescing, and lookup helpers.
TEST_F(CppTargetEncodingGeneratorTest, TestEncodingTableGeneration)
{
    std::string idfSource = R"(
        target AMD64;

        target_inst ADD64rr(GPR64:dst OUT, GPR64:src1 IN, GPR64:src2 IN) {
            MNEMONIC("addq");
            ENCODING {
                form: rr;
                opcode: [0x01];
                rex_w: true;
                operands { src2 => reg; dst => rm_reg; };
                coalesce: src1;
                size: dst;
            };
        };

        target_inst LOAD32(GPR32:dst OUT, Mem32:addr IN) {
            MNEMONIC("movl");
            ENCODING {
                form: rm;
                opcode: [0x8B];
                operands { dst => reg; addr => rm_mem; };
                size: dst;
                sse_prefix: F3;
                sse_opcode: [0x0F, 0x10];
            };
        };

        target_inst RET() {
            MNEMONIC("ret");
            ENCODING { form: ret; opcode: [0xC3]; };
        };

        target_inst PLAIN(GPR32:dst OUT) {
            MNEMONIC("plain");
        };
    )";

    ASSERT_TRUE(parseAndRunPass(idfSource));

    CppEncodingTableGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "AMD64EncodingTable.h";
    ASSERT_TRUE(std::filesystem::exists(headerPath));

    std::string header = readFileContent(headerPath);

    EXPECT_NE(header.find("#include \"X86_64/Encoding/X86_64EncodingDesc.h\""), std::string::npos);
    EXPECT_NE(header.find("namespace EzCodeEmitter::X86_64"), std::string::npos);
    EXPECT_NE(header.find("inline constexpr EncodingDesc s_encodings[]"), std::string::npos);
    EXPECT_NE(header.find("inline constexpr std::size_t s_encodingCount = 4;"), std::string::npos);

    // ADD64rr: register-register with REX.W, digit sentinel, coalesce and reg/rm slots.
    EXPECT_NE(header.find("EncForm::Rr"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Reg, 2"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::RmReg, 0"), std::string::npos);
    EXPECT_NE(header.find(".m_rexW = 1"), std::string::npos);
    EXPECT_NE(header.find(".m_coalesceSrc = 1"), std::string::npos);

    // LOAD32: memory operand plus SSE variant.
    EXPECT_NE(header.find("EncForm::Rm"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::RmMem, 1"), std::string::npos);
    EXPECT_NE(header.find(".m_hasSseVariant = true"), std::string::npos);
    EXPECT_NE(header.find(".m_ssePrefixes = 8"), std::string::npos);

    // Instructions without an ENCODING block still occupy a table slot.
    EXPECT_NE(header.find("EncodingDesc{}"), std::string::npos);

    EXPECT_NE(header.find("getEncodingDesc(std::size_t id)"), std::string::npos);
    EXPECT_NE(header.find("findEncodingDesc(const char *name)"), std::string::npos);
    EXPECT_NE(header.find("\"PLAIN\""), std::string::npos);
}

// Rejects an encoding that binds an operand absent from the instruction signature.
TEST_F(CppTargetEncodingGeneratorTest, TestInvalidEncodingIsRejected)
{
    // The binding references an operand that does not exist in the signature.
    std::string idfSource = R"(
        target AMD64;
        target_inst BAD(GPR32:dst OUT) {
            MNEMONIC("bad");
            ENCODING {
                form: rr;
                opcode: [0x01];
                operands { missing => reg; };
            };
        };
    )";

    EXPECT_FALSE(parseAndRunPass(idfSource));
}

// Rejects a jcc encoding that omits the required condition code.
TEST_F(CppTargetEncodingGeneratorTest, TestJccRequiresConditionCode)
{
    std::string idfSource = R"(
        target AMD64;
        target_inst BADJCC(i64:target IN) {
            MNEMONIC("badjcc");
            ENCODING {
                form: jcc;
                opcode: [0x0F, 0x80];
                operands { target => rel32; };
            };
        };
    )";

    EXPECT_FALSE(parseAndRunPass(idfSource));
}

// A test-only second ISA (dialect + backend) generates its own table without touching shared code.
TEST_F(CppTargetEncodingGeneratorTest, StubDialectAndBackendGenerateOwnTable)
{
    static StubEncodingDialect stubDialect;
    static StubEncodingBackend stubBackend;
    Sema::Encoding::registerEncodingDialect("stub", &stubDialect);
    registerEncodingBackend("stubisa", &stubBackend);

    std::string idfSource = R"(
        target STUBISA;
        target_inst BAR(GPR32:dst OUT) {
            MNEMONIC("bar");
            ENCODING [stub] { bits: 3; };
        };
    )";
    ASSERT_TRUE(parseAndRunPass(idfSource));

    CppEncodingTableGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "stubisa");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "stubisaEncodingTable.h";
    ASSERT_TRUE(std::filesystem::exists(headerPath));
    std::string header = readFileContent(headerPath);

    EXPECT_NE(header.find("#include \"Stub/StubEncodingDesc.h\""), std::string::npos);
    EXPECT_NE(header.find("namespace Stub::Encoding"), std::string::npos);
    EXPECT_NE(header.find("inline constexpr StubDesc s_encodings[]"), std::string::npos);
    EXPECT_NE(header.find("StubDesc{},"), std::string::npos);
    EXPECT_NE(header.find("inline const StubDesc *findEncodingDesc(const char *name)"), std::string::npos);
}

// An unknown target with no registered backend fails generation rather than emitting a table.
TEST_F(CppTargetEncodingGeneratorTest, UnknownTargetBackendFails)
{
    CppEncodingTableGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "not_a_target");
    EXPECT_FALSE(generator.run());
}
