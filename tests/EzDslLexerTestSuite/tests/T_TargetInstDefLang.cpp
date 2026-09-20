#include "EzDslLexerTestSuite.h"
#include "Ast/EncodingDefLangAst.h"
#include "Ast/TargetInstDefLangAst.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetInstDefLang.h"

namespace
{
// Returns the value of the first directive with the given key, or nullptr.
const DSL::Ast::Encoding::Value *findDirectiveValue(const DSL::Ast::Encoding::EncodingDecl &encoding,
                                                    std::string_view key)
{
    for (const auto &directive : encoding.m_directives)
    {
        if (directive.m_key.m_node == key)
        {
            return &directive.m_value;
        }
    }
    return nullptr;
}
} // namespace

/**
 * Test fixture for the target instruction definition (.idf) dialect parser.
 */
class TargetInstDefLangTest : public DslLexerTestSuiteAsGtest
{
};

/**
 * Verifies parsing an input-direction operand (class:name IN).
 */
TEST_F(TargetInstDefLangTest, TestTargetOperandIn)
{
    std::string test = "GPR32:src IN";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetOperand, DSL::Ast::TargetInstDef::TargetOperandDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_regClassOrType.m_node, "GPR32");
    EXPECT_EQ(res->m_name.m_node, "src");
    EXPECT_EQ(res->m_direction, DSL::Ast::TargetInstDef::OperandDirection::In);
}

/**
 * Verifies parsing an output-direction operand (class:name OUT).
 */
TEST_F(TargetInstDefLangTest, TestTargetOperandOut)
{
    std::string test = "GPR64:dst OUT";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetOperand, DSL::Ast::TargetInstDef::TargetOperandDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_regClassOrType.m_node, "GPR64");
    EXPECT_EQ(res->m_name.m_node, "dst");
    EXPECT_EQ(res->m_direction, DSL::Ast::TargetInstDef::OperandDirection::Out);
}

/**
 * Verifies parsing a read/write operand (class:name INOUT).
 */
TEST_F(TargetInstDefLangTest, TestTargetOperandInOut)
{
    std::string test = "Mem32:addr INOUT";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetOperand, DSL::Ast::TargetInstDef::TargetOperandDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_regClassOrType.m_node, "Mem32");
    EXPECT_EQ(res->m_name.m_node, "addr");
    EXPECT_EQ(res->m_direction, DSL::Ast::TargetInstDef::OperandDirection::InOut);
}

/**
 * Verifies a target instruction declaration parses its operands plus the
 * MNEMONIC, FLAGS, IMPLICIT_DEFS, and IMPLICIT_USES attributes.
 */
TEST_F(TargetInstDefLangTest, TestTargetInstructionDeclaration)
{
    std::string test = R"(
        target_inst ADD32rr(GPR32:dst OUT, GPR32:src1 IN, GPR32:src2 IN) {
            MNEMONIC("addl");
            FLAGS(IsCommutative);
            IMPLICIT_DEFS(EFLAGS);
            IMPLICIT_USES(EAX, EDX);
        };
    )";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetInstDecl, DSL::Ast::TargetInstDef::TargetInstDecl>();
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->m_instName.m_node, "ADD32rr");
    ASSERT_EQ(res->m_operands.size(), 3);
    EXPECT_EQ(res->m_operands[0].m_name.m_node, "dst");
    EXPECT_EQ(res->m_operands[0].m_direction, DSL::Ast::TargetInstDef::OperandDirection::Out);
    EXPECT_EQ(res->m_operands[1].m_name.m_node, "src1");
    EXPECT_EQ(res->m_operands[2].m_name.m_node, "src2");

    ASSERT_TRUE(res->m_mnemonic.has_value());
    EXPECT_EQ(res->m_mnemonic->m_node, "addl");

    ASSERT_EQ(res->m_flags.size(), 1);
    EXPECT_EQ(res->m_flags[0].m_node, "IsCommutative");

    ASSERT_EQ(res->m_implicitDefs.size(), 1);
    EXPECT_EQ(res->m_implicitDefs[0].m_node, "EFLAGS");

    ASSERT_EQ(res->m_implicitUses.size(), 2);
    EXPECT_EQ(res->m_implicitUses[0].m_node, "EAX");
    EXPECT_EQ(res->m_implicitUses[1].m_node, "EDX");
}

/**
 * Verifies a full target instruction file parses the target name and multiple
 * instruction declarations, including one with no operands.
 */
TEST_F(TargetInstDefLangTest, TestTargetInstructionFile)
{
    std::string test = R"(
        target AMD64;

        target_inst NOP() {
            MNEMONIC("nop");
        };

        target_inst ADD32rm(GPR32:dst OUT, GPR32:src IN, Mem32:addr IN) {
            MNEMONIC("addl");
            FLAGS(ReadsMemory);
            IMPLICIT_DEFS(EFLAGS);
        };
    )";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetInstFile, DSL::Ast::TargetInstDef::TargetInstFile>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res->m_targetName.has_value());
    EXPECT_EQ(res->m_targetName->m_node, "AMD64");

    ASSERT_EQ(res->m_instructions.size(), 2);
    EXPECT_EQ(res->m_instructions[0].m_instName.m_node, "NOP");
    EXPECT_EQ(res->m_instructions[0].m_operands.size(), 0);

    EXPECT_EQ(res->m_instructions[1].m_instName.m_node, "ADD32rm");
    EXPECT_EQ(res->m_instructions[1].m_operands.size(), 3);
}

/**
 * Verifies the generic ENCODING block parses into arch-neutral directives: an
 * identifier form, a byte-list opcode, an integer digit and operand bindings.
 */
TEST_F(TargetInstDefLangTest, TestGenericEncodingDirectives)
{
    std::string test = R"(
        target_inst ADD32rr(GPR32:dst OUT, GPR32:src1 IN, GPR32:src2 IN) {
            MNEMONIC("addl");
            ENCODING {
                form: rr;
                opcode: [0x01];
                opcode_digit: 0;
                operands { src2 => reg; dst => rm_reg; };
                coalesce: src1;
            };
        };
    )";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetInstDecl, DSL::Ast::TargetInstDef::TargetInstDecl>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res->m_encoding.has_value());
    const auto &enc = res->m_encoding.value();
    EXPECT_FALSE(enc.m_backend.has_value());
    ASSERT_EQ(enc.m_directives.size(), 5);

    const auto *form = findDirectiveValue(enc, "form");
    ASSERT_NE(form, nullptr);
    ASSERT_TRUE(std::holds_alternative<DSL::Ast::Common::Identifier>(*form));
    EXPECT_EQ(std::get<DSL::Ast::Common::Identifier>(*form).m_node, "rr");

    const auto *opcode = findDirectiveValue(enc, "opcode");
    ASSERT_NE(opcode, nullptr);
    ASSERT_TRUE(std::holds_alternative<std::pmr::vector<uint8_t>>(*opcode));
    ASSERT_EQ(std::get<std::pmr::vector<uint8_t>>(*opcode).size(), 1u);
    EXPECT_EQ(std::get<std::pmr::vector<uint8_t>>(*opcode)[0], 0x01);

    const auto *digit = findDirectiveValue(enc, "opcode_digit");
    ASSERT_NE(digit, nullptr);
    ASSERT_TRUE(std::holds_alternative<int64_t>(*digit));
    EXPECT_EQ(std::get<int64_t>(*digit), 0);

    const auto *operands = findDirectiveValue(enc, "operands");
    ASSERT_NE(operands, nullptr);
    ASSERT_TRUE(std::holds_alternative<std::pmr::vector<DSL::Ast::Encoding::OperandBinding>>(*operands));
    const auto &bindings = std::get<std::pmr::vector<DSL::Ast::Encoding::OperandBinding>>(*operands);
    ASSERT_EQ(bindings.size(), 2u);
    EXPECT_EQ(bindings[0].m_operand.m_node, "src2");
    EXPECT_EQ(bindings[0].m_field.m_node, "reg");
    EXPECT_EQ(bindings[1].m_operand.m_node, "dst");
    EXPECT_EQ(bindings[1].m_field.m_node, "rm_reg");
}

/**
 * Verifies the optional explicit backend selector parses into `m_backend`.
 */
TEST_F(TargetInstDefLangTest, TestGenericEncodingBackendSelector)
{
    std::string test = R"(
        target_inst FOO(GPR32:dst OUT) {
            ENCODING [stub] { bits: 3; };
        };
    )";
    ParseContext ctx = createParseContextFromBuff("test", test);

    auto res = ctx.parse<DSL::Parser::TargetInstDef::TargetInstDecl, DSL::Ast::TargetInstDef::TargetInstDecl>();
    ASSERT_TRUE(res.has_value());
    ASSERT_TRUE(res->m_encoding.has_value());
    ASSERT_TRUE(res->m_encoding->m_backend.has_value());
    EXPECT_EQ(res->m_encoding->m_backend->m_node, "stub");

    const auto *bits = findDirectiveValue(res->m_encoding.value(), "bits");
    ASSERT_NE(bits, nullptr);
    ASSERT_TRUE(std::holds_alternative<int64_t>(*bits));
    EXPECT_EQ(std::get<int64_t>(*bits), 3);
}
