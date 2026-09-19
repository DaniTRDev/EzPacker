#include "EzDslCodeGeneratorsTestSuite.h"
#include "Ast/TargetInstDefLangAst.h"
#include "CodeGenerators/CppTargetEncodingGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetInstDefLang.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/TargetInstPass.h"

#include <filesystem>

using namespace CodeGenerators;

class CppTargetEncodingGeneratorTest : public EzDslCodeGeneratorsTestSuiteAsGtest
{
  protected:
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

    CppTargetEncodingGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "AMD64");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "AMD64EncodingTable.h";
    ASSERT_TRUE(std::filesystem::exists(headerPath));

    std::string header = readFileContent(headerPath);

    EXPECT_NE(header.find("#include \"TableGen/EncodingDesc.h\""), std::string::npos);
    EXPECT_NE(header.find("namespace EzCodeEmitter::TableGen::AMD64"), std::string::npos);
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
