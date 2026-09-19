#include "EzDslCliTestSuite.h"

#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Parser/IrInstructionDefLang.h"
#include "Parser/LegalizeActionDefLang.h"
#include "Parser/LegalizeRuleDefLang.h"
#include "Parser/TypeDefLang.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/IrSymbols.h"
#include "Sema/Symbols/TypeSymbols.h"

#include <sstream>

using namespace Cli;

/**
 * Fixture for the InfoDumper text/JSON rendering helpers.
 */
class InfoDumperTest : public EzDslCliTestSuiteAsGtest
{
};

// ============================================================================
// 1. JSON String Escaping
// ============================================================================

TEST_F(InfoDumperTest, EscapeJsonSpecialCharacters)
{
    EXPECT_EQ(InfoDumper::escapeJson("simple"), "simple");
    EXPECT_EQ(InfoDumper::escapeJson("quotes: \"hello\""), "quotes: \\\"hello\\\"");
    EXPECT_EQ(InfoDumper::escapeJson("backslash: \\path"), "backslash: \\\\path");
    EXPECT_EQ(InfoDumper::escapeJson("control:\n\t\r\b\f"), "control:\\n\\t\\r\\b\\f");

    // Non-printable control char < 0x20 (e.g., ASCII 1)
    std::string unprintable;
    unprintable.push_back('\x01');
    EXPECT_EQ(InfoDumper::escapeJson(unprintable), "\\u0001");
}

// ============================================================================
// 2. Dump General File Info
// ============================================================================

TEST_F(InfoDumperTest, DumpGeneralInfoTextAndJson)
{
    GeneralFileInfo info;
    info.inputPath = "types.tyf";
    info.fileSizeBytes = 1024;
    info.dialect = LanguageDialect::TypeDef;
    info.dialectName = "TypeDef (.tyf)";
    info.constructCount = 12;
    info.generatorName = "CppMirTypeTableGenerator";
    info.workingMode = "Full (.h and .cpp)";
    info.outputDirectory = "out/types";
    info.outputs = { { .role = "header", .path = "out/types/MirTypeTable.h", .exists = false },
                     { .role = "source", .path = "out/types/MirTypeTable.cpp", .exists = true } };

    // Text format
    std::ostringstream textOut;
    InfoDumper::dumpGeneralInfo(info, OutputFormat::Text, textOut);
    std::string textStr = textOut.str();
    EXPECT_TRUE(textStr.find("EzDSL File Information") != std::string::npos);
    EXPECT_TRUE(textStr.find("Input File:        types.tyf") != std::string::npos);
    EXPECT_TRUE(textStr.find("1024 bytes") != std::string::npos);
    EXPECT_TRUE(textStr.find("Constructs Found:  12") != std::string::npos);
    EXPECT_TRUE(textStr.find("CppMirTypeTableGenerator") != std::string::npos);
    EXPECT_TRUE(textStr.find("[header] out/types/MirTypeTable.h (exists: no)") != std::string::npos);
    EXPECT_TRUE(textStr.find("[source] out/types/MirTypeTable.cpp (exists: yes)") != std::string::npos);

    // JSON format
    std::ostringstream jsonOut;
    InfoDumper::dumpGeneralInfo(info, OutputFormat::Json, jsonOut);
    std::string jsonStr = jsonOut.str();
    EXPECT_TRUE(jsonStr.find("\"input\": \"types.tyf\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"size_bytes\": 1024") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"dialect\": \"TypeDef (.tyf)\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"construct_count\": 12") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"generator\": \"CppMirTypeTableGenerator\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"exists\": false") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"exists\": true") != std::string::npos);
}

// ============================================================================
// 3. Dump Output Files
// ============================================================================

TEST_F(InfoDumperTest, DumpOutputFilesTextAndJson)
{
    std::vector<OutputFileInfo> files = {
        { .role = "header", .path = "MirInstructionSetDefs.h", .exists = true },
    };

    std::ostringstream textOut;
    InfoDumper::dumpOutputFiles(files, OutputFormat::Text, textOut);
    EXPECT_TRUE(textOut.str().find("[header] MirInstructionSetDefs.h") != std::string::npos);

    std::ostringstream jsonOut;
    InfoDumper::dumpOutputFiles(files, OutputFormat::Json, jsonOut);
    EXPECT_TRUE(jsonOut.str().find("\"role\": \"header\"") != std::string::npos);
    EXPECT_TRUE(jsonOut.str().find("\"path\": \"MirInstructionSetDefs.h\"") != std::string::npos);
    EXPECT_TRUE(jsonOut.str().find("\"exists\": true") != std::string::npos);
}

// ============================================================================
// 4. Dump TypeDef AST
// ============================================================================

TEST_F(InfoDumperTest, DumpTypeDefAstTextAndJson)
{
    std::string source = "integer i32(32); float f64(64); void v();";
    auto ctx = createParseContextFromBuff("test.tyf", source);
    auto ast = ctx.parse<DSL::Parser::TypeDef::TypeDefFile, DSL::Ast::TypeDef::TypeDefFile>();
    ASSERT_TRUE(ast.has_value());

    std::ostringstream textOut;
    InfoDumper::dumpTypeDefAst(*ast, OutputFormat::Text, textOut);
    std::string textStr = textOut.str();
    EXPECT_TRUE(textStr.find("TypeDef AST Summary (3 types)") != std::string::npos);
    EXPECT_TRUE(textStr.find("i32") != std::string::npos);
    EXPECT_TRUE(textStr.find("Integer") != std::string::npos);
    EXPECT_TRUE(textStr.find("f64") != std::string::npos);
    EXPECT_TRUE(textStr.find("FloatingPoint") != std::string::npos);

    std::ostringstream jsonOut;
    InfoDumper::dumpTypeDefAst(*ast, OutputFormat::Json, jsonOut);
    std::string jsonStr = jsonOut.str();
    EXPECT_TRUE(jsonStr.find("\"name\": \"i32\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"kind\": \"Integer\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"bit_width\": 32") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"name\": \"v\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"kind\": \"Void\"") != std::string::npos);
}

// ============================================================================
// 5. Dump IrInstDef AST
// ============================================================================

TEST_F(InfoDumperTest, DumpIrInstDefAstTextAndJson)
{
    std::string source = R"(
        ir_inst Add ( Register:dst OUT, Register:lhs IN, Register:rhs IN ) {
            CATEGORY(Arithmetic);
            TIER(HighLevel);
            FLAGS(IsCommutative, SizeMatch);
        };
    )";

    auto ctx = createParseContextFromBuff("test.irdf", source);
    auto ast = ctx.parse<DSL::Parser::IrInstDef::IrInstDefFile, DSL::Ast::IrInstDef::IrInstDefFile>();
    ASSERT_TRUE(ast.has_value());

    std::ostringstream textOut;
    InfoDumper::dumpIrInstDefAst(*ast, OutputFormat::Text, textOut);
    std::string textStr = textOut.str();
    EXPECT_TRUE(textStr.find("Instruction: Add") != std::string::npos);
    EXPECT_TRUE(textStr.find("Arithmetic") != std::string::npos);
    EXPECT_TRUE(textStr.find("HighLevel") != std::string::npos);
    EXPECT_TRUE(textStr.find("IsCommutative") != std::string::npos);
    EXPECT_TRUE(textStr.find("OUT") != std::string::npos);

    std::ostringstream jsonOut;
    InfoDumper::dumpIrInstDefAst(*ast, OutputFormat::Json, jsonOut);
    std::string jsonStr = jsonOut.str();
    EXPECT_TRUE(jsonStr.find("\"name\": \"Add\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"category\": \"Arithmetic\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"tier\": \"HighLevel\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"IsCommutative\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"direction\": \"OUT\"") != std::string::npos);
}

// ============================================================================
// 6. Dump Symbols
// ============================================================================

TEST_F(InfoDumperTest, DumpSymbolsTextAndJson)
{
    // Populate symbols in SymbolTable
    Symbols::TypeSymbol typeSym{ .m_name = "i32",
                                 .m_kind = DSL::Ast::TypeDef::TypeKind::Integer,
                                 .m_bitWidth = 32,
                                 .m_alignment = 32,
                                 .m_compactId = 1 };
    getSymbolTable()->declareSym(nullptr, SymbolType::Type, std::move(typeSym), "i32");

    Symbols::IrInstructionSymbol instSym{ .m_name = "Ret",
                                          .m_category = DSL::Ast::IrInstDef::IrInstCategory::ControlFlow,
                                          .m_tier = DSL::Ast::IrInstDef::IrInstTier::HighLevel,
                                          .m_flags = DSL::Ast::IrInstDef::IrInstFlag::IsReturn };
    getSymbolTable()->declareSym(nullptr, SymbolType::IrInstruction, std::move(instSym), "Ret");

    std::ostringstream textOut;
    InfoDumper::dumpSymbols(*getSymbolTable(), OutputFormat::Text, textOut);
    std::string textStr = textOut.str();
    EXPECT_TRUE(textStr.find("Symbol Table Dump") != std::string::npos);
    EXPECT_TRUE(textStr.find("[Type]") != std::string::npos);
    EXPECT_TRUE(textStr.find("i32") != std::string::npos);
    EXPECT_TRUE(textStr.find("[Instruction]") != std::string::npos);
    EXPECT_TRUE(textStr.find("Ret") != std::string::npos);

    std::ostringstream jsonOut;
    InfoDumper::dumpSymbols(*getSymbolTable(), OutputFormat::Json, jsonOut);
    std::string jsonStr = jsonOut.str();
    EXPECT_TRUE(jsonStr.find("\"symbols\": [") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"name\": \"i32\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"type\": \"Type\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"bit_width\": 32") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"name\": \"Ret\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"type\": \"IrInstruction\"") != std::string::npos);
    EXPECT_TRUE(jsonStr.find("\"category\": \"ControlFlow\"") != std::string::npos);
}
