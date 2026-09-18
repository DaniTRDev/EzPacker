#ifndef EZMIR_MIR_PARSER_H
#define EZMIR_MIR_PARSER_H

#include "EzMirCommon.h"
#include "Parser/MirLexer.h"
#include "Parser/MirParserContext.h"
#include <memory_resource>
#include <optional>
#include <string_view>

class MirBuilderContext;
class DiagnosticCollector;
class MirFunction;
class MirBlock;
class MirInstruction;
class MirOperand;
class MirType;
class MirMemory;

namespace EzMir
{

struct MirParserOptions
{
    bool verifySsa{ true };
    bool allowTargetInstructions{ true };
};

/**
 * High-level parser for Textual Machine Intermediate Representation (.mir).
 * Materializes in-memory MirModule, MirFunction, MirBlock, MirInstruction, and MirOperand graphs.
 */
class MirParser
{
  public:
    explicit MirParser(MirBuilderContext *ctx,
                       DiagnosticCollector *diagCollector = nullptr,
                       MirParserOptions options = {});

    /**
     * Parses a complete MIR module from a source string buffer.
     * Populates ctx with functions, types, and global variables.
     */
    bool parseModule(std::string_view source, std::string_view bufferName = "input.mir");

    /**
     * Parses a single MIR function from a string buffer and attaches it to ctx.
     */
    MirFunction *parseFunction(std::string_view source);

    /**
     * Parses a single MIR instruction and inserts it at the current builder point in targetBlock.
     */
    MirInstruction *parseInstruction(std::string_view source, MirBlock *targetBlock);

  private:
    Ast::MirAstType *parseAstType(Parser::MirLexer &lexer, MirParserContext &pCtx);
    std::optional<Ast::MirAstConstantInit> parseConstantInit(Parser::MirLexer &lexer, MirParserContext &pCtx);

    bool parseTopLevelDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    bool parseTargetDirective(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    bool parseGlobalVarDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    bool parseFunctionDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    bool parseFunctionDef(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    bool parseBasicBlock(Parser::MirLexer &lexer, MirParserContext &pCtx, MirFunction *func);
    MirInstruction *parseInstructionStatement(Parser::MirLexer &lexer, MirParserContext &pCtx, MirBlock *block);

    MirOperand *parseOperand(Parser::MirLexer &lexer,
                             MirParserContext &pCtx,
                             MirInstruction *targetInst,
                             size_t operandIdx,
                             MirType *expectedType);

    MirMemory *parseMemoryOperand(Parser::MirLexer &lexer,
                                  MirParserContext &pCtx,
                                  MirType *memType,
                                  SourceReference *startRef);

    bool matchToken(Parser::MirLexer &lexer, Parser::MirTokenKind kind, std::string_view errorMsg);

  private:
    MirBuilderContext *m_ctx{ nullptr };
    DiagnosticCollector *m_diag{ nullptr };
    MirParserOptions m_options;
};

} // namespace EzMir

#endif // EZMIR_MIR_PARSER_H
