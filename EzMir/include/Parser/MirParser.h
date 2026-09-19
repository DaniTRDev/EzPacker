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

/**
 * Toggles controlling parser strictness and accepted input.
 */
struct MirParserOptions
{
    bool verifySsa{ true };               // When true, enforce single-definition/use SSA constraints while parsing.
    bool allowTargetInstructions{ true }; // When true, accept target-specific instruction syntax.
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
    /**
     * Parses a type expression into its AST node.
     */
    Ast::MirAstType *parseAstType(Parser::MirLexer &lexer, MirParserContext &pCtx);
    /**
     * Parses an optional constant initializer expression for a global.
     */
    std::optional<Ast::MirAstConstantInit> parseConstantInit(Parser::MirLexer &lexer, MirParserContext &pCtx);

    /**
     * Dispatches a single top-level declaration and appends it to module.
     */
    bool parseTopLevelDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    /**
     * Parses a "target" directive selecting the backend.
     */
    bool parseTargetDirective(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    /**
     * Parses a global variable declaration.
     */
    bool parseGlobalVarDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    /**
     * Parses a function prototype ("declare").
     */
    bool parseFunctionDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    /**
     * Parses a function definition body.
     */
    bool parseFunctionDef(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module);
    /**
     * Parses one labeled basic block and its instructions into func.
     */
    bool parseBasicBlock(Parser::MirLexer &lexer, MirParserContext &pCtx, MirFunction *func);
    /**
     * Parses a single instruction statement and inserts it into block.
     */
    MirInstruction *parseInstructionStatement(Parser::MirLexer &lexer, MirParserContext &pCtx, MirBlock *block);

    /**
     * Parses one instruction operand, recording a forward fixup when the referenced symbol is unknown.
     */
    MirOperand *parseOperand(Parser::MirLexer &lexer,
                             MirParserContext &pCtx,
                             MirInstruction *targetInst,
                             size_t operandIdx,
                             MirType *expectedType);

    /**
     * Parses a bracketed memory addressing operand of the form [base + index*scale + disp].
     */
    MirMemory *
    parseMemoryOperand(Parser::MirLexer &lexer, MirParserContext &pCtx, MirType *memType, SourceReference *startRef);

    /**
     * Consumes the next token, requiring it to be of the given kind; reports errorMsg otherwise.
     */
    bool matchToken(Parser::MirLexer &lexer, Parser::MirTokenKind kind, std::string_view errorMsg);

  private:
    MirBuilderContext *m_ctx{ nullptr };    // Context receiving constructed MIR entities.
    DiagnosticCollector *m_diag{ nullptr }; // Collector for parser error diagnostics.
    MirParserOptions m_options;             // Parser strictness/feature toggles.
};

} // namespace EzMir

#endif // EZMIR_MIR_PARSER_H
