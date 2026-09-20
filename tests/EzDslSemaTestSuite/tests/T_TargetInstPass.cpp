#include "EzDslSemaTestSuite.h"
#include "Ast/TargetInstDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "Parser/TargetInstDefLang.h"
#include "Sema/Encoding/EncodingDialect.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/TargetInstPass.h"

namespace
{

/**
 * Test-only dialect proving that a new ISA needs only a dialect implementation:
 * it accepts any non-empty encoding without touching shared parser/sema code.
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

} // namespace

/**
 * Fixture for exercising the target-instruction semantic pass and encoding dialect selection.
 */
class TargetInstPassTest : public EzDslSemaTestSuiteAsGtest
{
  protected:
    // Parses an .idf source and runs the target-instruction semantic pass.
    bool parseAndRunPass(const std::string &source)
    {
        ParseContext ctx = createParseContextFromBuff("target_inst_pass_test", source);
        auto ast = ctx.parse<DSL::Parser::TargetInstDef::TargetInstFile, DSL::Ast::TargetInstDef::TargetInstFile>();
        if (!ast.has_value())
        {
            return false;
        }
        return TargetInstPass::run(getDiagCollector(), getSymbolTable(), &ast.value());
    }
};

// The x86-64 dialect is selected by the file's target name and validates a well-formed encoding.
TEST_F(TargetInstPassTest, SelectsX86_64DialectFromTargetName)
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
    )";

    EXPECT_TRUE(parseAndRunPass(idfSource));
}

// An explicitly requested but unregistered backend is rejected with a diagnostic.
TEST_F(TargetInstPassTest, RejectsUnknownBackend)
{
    std::string idfSource = R"(
        target AMD64;
        target_inst FOO(GPR32:dst OUT) {
            MNEMONIC("foo");
            ENCODING [does_not_exist] { bits: 1; };
        };
    )";

    EXPECT_FALSE(parseAndRunPass(idfSource));
}

// A stub dialect registered by the test validates its own encoding vocabulary.
TEST_F(TargetInstPassTest, StubDialectValidatesItsOwnEncoding)
{
    static StubEncodingDialect stubDialect;
    Sema::Encoding::registerEncodingDialect("stub", &stubDialect);

    std::string idfSource = R"(
        target STUB;
        target_inst BAR(GPR32:dst OUT) {
            MNEMONIC("bar");
            ENCODING [stub] { bits: 3; };
        };
    )";

    EXPECT_TRUE(parseAndRunPass(idfSource));
}

// The x86-64 dialect rejects an encoding that omits the required condition code for Jcc.
TEST_F(TargetInstPassTest, X86_64DialectRejectsJccWithoutCondition)
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
