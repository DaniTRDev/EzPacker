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
#include "EzTargetsRiscV64Dsl.h"

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
    std::string_view includeHeader() const override { return "Stub/StubEncodingDesc.h"; }
    std::string_view namespaceName() const override { return "Stub::Encoding"; }
    std::string_view arrayType() const override { return "StubDesc"; }
    std::string
    row(const Symbols::TargetInstructionSymbol & /*sym*/, DiagnosticCollector * /*diag*/ = nullptr) const override
    {
        return "StubDesc{}";
    }
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

    EXPECT_NE(header.find("#include \"Encoding/X86_64EncodingDesc.h\""), std::string::npos);
    EXPECT_NE(header.find("namespace EzTargets::X86_64"), std::string::npos);
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

// Generates RISC-V encoding descriptors and verifies R, I, S, B, U, J forms and opcode/funct fields.
TEST_F(CppTargetEncodingGeneratorTest, TestRiscVEncodingTableGeneration)
{
    EzTargets::RiscV64::registerDsl();

    std::string idfSource = R"(
        target RiscV64;

        target_inst ADD64rr(GPR64:dst OUT, GPR64:src1 IN, GPR64:src2 IN) {
            MNEMONIC("add");
            FLAGS(IsCommutative);
            ENCODING {
                form: r_type;
                opcode: [0x33];
                funct3: 0x0;
                funct7: 0x00;
                operands { dst => rd; src1 => rs1; src2 => rs2; };
            };
        };

        target_inst ADDI64ri(GPR64:dst OUT, GPR64:src1 IN, i64:imm IN) {
            MNEMONIC("addi");
            ENCODING {
                form: i_type;
                opcode: [0x13];
                funct3: 0x0;
                operands { dst => rd; src1 => rs1; imm => imm12_signed; };
            };
        };

        target_inst SD(GPR64:src IN, GPR64:base IN, i64:offset IN) {
            MNEMONIC("sd");
            ENCODING {
                form: s_type;
                opcode: [0x23];
                funct3: 0x3;
                operands { src => rs2; base => rs1; offset => imm12_store; };
            };
        };

        target_inst BEQ(GPR64:src1 IN, GPR64:src2 IN, i64:target IN) {
            MNEMONIC("beq");
            ENCODING {
                form: b_type;
                opcode: [0x63];
                funct3: 0x0;
                operands { src1 => rs1; src2 => rs2; target => imm12_branch; };
            };
        };

        target_inst LUI(GPR64:dst OUT, i64:imm IN) {
            MNEMONIC("lui");
            ENCODING {
                form: u_type;
                opcode: [0x37];
                operands { dst => rd; imm => imm20_upper; };
            };
        };

        target_inst JAL(GPR64:dst OUT, i64:target IN) {
            MNEMONIC("jal");
            ENCODING {
                form: j_type;
                opcode: [0x6F];
                operands { dst => rd; target => imm20_jump; };
            };
        };
    )";

    ASSERT_TRUE(parseAndRunPass(idfSource));

    CppEncodingTableGenerator generator(getDiagCollector(), getSymbolTable(), m_testTempDir, "RiscV64");
    ASSERT_TRUE(generator.run());

    auto headerPath = m_testTempDir / "RiscV64EncodingTable.h";
    ASSERT_TRUE(std::filesystem::exists(headerPath));

    std::string header = readFileContent(headerPath);

    EXPECT_NE(header.find("#include \"Encoding/RiscV64EncodingDesc.h\""), std::string::npos);
    EXPECT_NE(header.find("namespace EzTargets::RiscV64"), std::string::npos);
    EXPECT_NE(header.find("inline constexpr EncodingDesc s_encodings[]"), std::string::npos);
    EXPECT_NE(header.find("inline constexpr std::size_t s_encodingCount = 6;"), std::string::npos);

    // ADD64rr: RType, opcode 0x33, funct3 0x00, funct7 0x00
    EXPECT_NE(header.find("EncForm::RType"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Rd"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Rs1"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Rs2"), std::string::npos);

    // ADDI64ri: IType, opcode 0x13
    EXPECT_NE(header.find("EncForm::IType"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Imm12Signed"), std::string::npos);

    // SD: SType, opcode 0x23, funct3 0x03
    EXPECT_NE(header.find("EncForm::SType"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Imm12Store"), std::string::npos);

    // BEQ: BType, opcode 0x63, relocOperand should be 2 (target)
    EXPECT_NE(header.find("EncForm::BType"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Imm12Branch"), std::string::npos);
    EXPECT_NE(header.find(".m_relocOperand = 2"), std::string::npos);

    // LUI: UType, opcode 0x37
    EXPECT_NE(header.find("EncForm::UType"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Imm20Upper"), std::string::npos);

    // JAL: JType, opcode 0x6F
    EXPECT_NE(header.find("EncForm::JType"), std::string::npos);
    EXPECT_NE(header.find("EncSlotKind::Imm20Jump"), std::string::npos);
}

// Rejects a RISC-V encoding with an unknown field or duplicate register slot.
TEST_F(CppTargetEncodingGeneratorTest, TestRiscVInvalidEncodingIsRejected)
{
    EzTargets::RiscV64::registerDsl();

    std::string idfSource = R"(
        target RiscV64;
        target_inst BAD(GPR64:dst OUT, GPR64:src IN) {
            MNEMONIC("bad");
            ENCODING {
                form: r_type;
                opcode: [0x33];
                operands { dst => rd; src => rd; };
            };
        };
    )";

    EXPECT_FALSE(parseAndRunPass(idfSource));
}
