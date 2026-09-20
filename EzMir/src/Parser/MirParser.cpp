#include "Parser/MirParser.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "GlobalVar/MirGlobalVar.h"
#include "GlobalVar/MirGlobalVarBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

namespace EzMir
{

namespace
{

/**
 * Builds an integer operand from a raw literal value at the destination type's width, translating
 * FlexInt overflow/errors into a parser diagnostic and a recorded parse error.
 */
MirInteger *buildIntSafe(MirOperandBuilder &opBuilder,
                         MirType *type,
                         int64_t rawVal,
                         SourceReference *ref,
                         DiagnosticCollector *diag,
                         MirParserContext &pCtx)
{
    if (!type)
    {
        return nullptr;
    }
    size_t bitWidth = type->getTotalSizeInBits();
    try
    {
        FlexInt val = (rawVal >= 0) ? FlexInt(static_cast<uint64_t>(rawVal), bitWidth) : FlexInt(rawVal, bitWidth);
        return opBuilder.buildInt(type, val, ref);
    }
    catch (const std::exception &ex)
    {
        if (diag)
        {
            diag->error("MirParser", "Integer literal overflow or error: {}", ex.what()) << ref;
        }
        pCtx.recordError();
        return nullptr;
    }
}

} // anonymous namespace

/**
 * Creates the parser, defaulting the diagnostic collector to the builder context's collector and
 * optionally adopting a caller-owned source manager.
 */
MirParser::MirParser(MirBuilderContext *ctx,
                     DiagnosticCollector *diagCollector,
                     MirParserOptions options,
                     GenericSourceManager *sourceManager) :
    m_ctx(ctx), m_diag(diagCollector ? diagCollector : (ctx ? ctx->getDiagCollector() : nullptr)), m_options(options),
    m_sourceMgr(sourceManager)
{
}

/**
 * Requires the next token to be of the given kind; on mismatch reports errorMsg and returns false.
 */
bool MirParser::matchToken(Parser::MirLexer &lexer, Parser::MirTokenKind kind, std::string_view errorMsg)
{
    const auto &tok = lexer.peekToken();
    if (tok.m_kind != kind)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "{} (got '{}')", errorMsg, tok.m_text) << tok.m_ref;
        }
        return false;
    }
    lexer.nextToken();
    return true;
}

/**
 * Parses a type expression into its AST form: "ptr[<type>]", "[N x type]", "void", "token",
 * primitive iN/fN types and bare identifier type names.
 */
Ast::MirAstType *MirParser::parseAstType(Parser::MirLexer &lexer, MirParserContext &pCtx)
{
    const auto &tok = lexer.peekToken();
    auto *mr = pCtx.getArena();
    std::pmr::polymorphic_allocator<Ast::MirAstType> alloc(mr);

    // Pointer type: ptr or ptr<Type>
    if (tok.m_kind == Parser::MirTokenKind::TypePtr)
    {
        lexer.nextToken();
        auto *astType = alloc.new_object<Ast::MirAstType>(mr);
        astType->m_kind = Ast::TypeKind::Pointer;
        astType->m_name = "ptr";
        astType->m_ref = tok.m_ref;

        if (lexer.peekToken().m_kind == Parser::MirTokenKind::LAngle)
        {
            lexer.nextToken(); // Consume '<'
            astType->m_subType = parseAstType(lexer, pCtx);
            if (!matchToken(lexer, Parser::MirTokenKind::RAngle, "Expected '>' after pointer pointee type"))
            {
                return nullptr;
            }
        }
        return astType;
    }

    // Array type: [14 x i8]
    if (tok.m_kind == Parser::MirTokenKind::LBracket)
    {
        lexer.nextToken(); // Consume '['
        auto *astType = alloc.new_object<Ast::MirAstType>(mr);
        astType->m_kind = Ast::TypeKind::Array;
        astType->m_ref = tok.m_ref;

        const auto &sizeTok = lexer.peekToken();
        if (sizeTok.m_kind != Parser::MirTokenKind::IntegerLiteral)
        {
            if (m_diag)
            {
                m_diag->error("MirParser", "Expected array size integer in array type") << sizeTok.m_ref;
            }
            return nullptr;
        }
        astType->m_arraySize = static_cast<size_t>(sizeTok.m_intVal);
        lexer.nextToken();

        // Match 'x' (identifier with text 'x')
        const auto &xTok = lexer.peekToken();
        if (xTok.m_kind != Parser::MirTokenKind::Identifier || xTok.m_text != "x")
        {
            if (m_diag)
            {
                m_diag->error("MirParser", "Expected 'x' in array type definition") << xTok.m_ref;
            }
            return nullptr;
        }
        lexer.nextToken();

        astType->m_subType = parseAstType(lexer, pCtx);
        if (!matchToken(lexer, Parser::MirTokenKind::RBracket, "Expected ']' at end of array type"))
        {
            return nullptr;
        }
        return astType;
    }

    // Void type
    if (tok.m_kind == Parser::MirTokenKind::TypeVoid)
    {
        lexer.nextToken();
        auto *astType = alloc.new_object<Ast::MirAstType>(mr);
        astType->m_kind = Ast::TypeKind::Void;
        astType->m_name = "void";
        astType->m_ref = tok.m_ref;
        return astType;
    }

    // Token type
    if (tok.m_kind == Parser::MirTokenKind::TypeToken)
    {
        lexer.nextToken();
        auto *astType = alloc.new_object<Ast::MirAstType>(mr);
        astType->m_kind = Ast::TypeKind::Token;
        astType->m_name = tok.m_text;
        astType->m_ref = tok.m_ref;
        return astType;
    }

    // Primitive integer / float types: i1..i256, f32..f128
    if (tok.m_kind >= Parser::MirTokenKind::TypeI1 && tok.m_kind <= Parser::MirTokenKind::TypeF128)
    {
        lexer.nextToken();
        auto *astType = alloc.new_object<Ast::MirAstType>(mr);
        astType->m_kind = Ast::TypeKind::Primitive;
        astType->m_name = tok.m_text;
        astType->m_ref = tok.m_ref;
        return astType;
    }

    // Unknown or identifier type
    if (tok.m_kind == Parser::MirTokenKind::Identifier)
    {
        lexer.nextToken();
        auto *astType = alloc.new_object<Ast::MirAstType>(mr);
        astType->m_kind = Ast::TypeKind::Primitive;
        astType->m_name = tok.m_text;
        astType->m_ref = tok.m_ref;
        return astType;
    }

    return nullptr;
}

/**
 * Parses an optional constant initializer, accepting integer/float/string literals, bracketed array
 * constants, and zero-initializer spellings (with an optional type prefix consumed first). Returns
 * nullopt when the next token does not begin an initializer.
 */
std::optional<Ast::MirAstConstantInit> MirParser::parseConstantInit(Parser::MirLexer &lexer, MirParserContext &pCtx)
{
    // Optional type prefix: e.g. i64 100, f32 1.5
    if ((lexer.peekToken().m_kind >= Parser::MirTokenKind::TypeI1 &&
         lexer.peekToken().m_kind <= Parser::MirTokenKind::TypeToken) ||
        lexer.peekToken().m_kind == Parser::MirTokenKind::TypePtr)
    {
        lexer.nextToken(); // Consume type prefix
    }

    const auto &tok = lexer.peekToken();
    auto *mr = pCtx.getArena();

    if (tok.m_kind == Parser::MirTokenKind::IntegerLiteral)
    {
        lexer.nextToken();
        Ast::MirAstConstantInit init(mr);
        init.m_kind = Ast::ConstantKind::Integer;
        init.m_intVal = tok.m_intVal;
        init.m_ref = tok.m_ref;
        return init;
    }

    if (tok.m_kind == Parser::MirTokenKind::FloatLiteral)
    {
        lexer.nextToken();
        Ast::MirAstConstantInit init(mr);
        init.m_kind = Ast::ConstantKind::Float;
        init.m_floatVal = tok.m_floatVal;
        init.m_ref = tok.m_ref;
        return init;
    }

    if (tok.m_kind == Parser::MirTokenKind::StringLiteral)
    {
        lexer.nextToken();
        Ast::MirAstConstantInit init(mr);
        init.m_kind = Ast::ConstantKind::String;
        init.m_strVal = tok.m_strVal;
        init.m_ref = tok.m_ref;
        return init;
    }

    // Array constant: [ c1, c2, ... ]
    if (tok.m_kind == Parser::MirTokenKind::LBracket)
    {
        lexer.nextToken();
        Ast::MirAstConstantInit init(mr);
        init.m_kind = Ast::ConstantKind::Array;
        init.m_ref = tok.m_ref;

        while (lexer.peekToken().m_kind != Parser::MirTokenKind::RBracket &&
               lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
        {
            auto elem = parseConstantInit(lexer, pCtx);
            if (elem)
            {
                init.m_elements.push_back(std::move(*elem));
            }
            if (lexer.peekToken().m_kind == Parser::MirTokenKind::Comma)
            {
                lexer.nextToken();
            }
        }
        matchToken(lexer, Parser::MirTokenKind::RBracket, "Expected ']' at end of array constant");
        return init;
    }

    // Zero-initializer identifier or <zeroinit>
    if (tok.m_kind == Parser::MirTokenKind::Identifier && (tok.m_text == "zeroinitializer" || tok.m_text == "zeroinit"))
    {
        lexer.nextToken();
        Ast::MirAstConstantInit init(mr);
        init.m_kind = Ast::ConstantKind::ZeroInit;
        init.m_ref = tok.m_ref;
        return init;
    }

    if (tok.m_kind == Parser::MirTokenKind::LAngle)
    {
        lexer.nextToken();
        const auto &idTok = lexer.nextToken();
        matchToken(lexer, Parser::MirTokenKind::RAngle, "Expected '>' after zeroinit");
        Ast::MirAstConstantInit init(mr);
        init.m_kind = Ast::ConstantKind::ZeroInit;
        init.m_ref = idTok.m_ref;
        return init;
    }

    return std::nullopt;
}

/**
 * Parses a target directive (target = triple;) and stores it on the module.
 */
bool MirParser::parseTargetDirective(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module)
{
    lexer.nextToken(); // Consume 'target'
    if (!matchToken(lexer, Parser::MirTokenKind::Equal, "Expected '=' after 'target'"))
    {
        return false;
    }

    const auto &strTok = lexer.peekToken();
    if (strTok.m_kind != Parser::MirTokenKind::StringLiteral)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Expected target triple string literal") << strTok.m_ref;
        }
        return false;
    }
    lexer.nextToken();

    if (!matchToken(lexer, Parser::MirTokenKind::Semicolon, "Expected ';' after target directive"))
    {
        return false;
    }

    Ast::MirAstTargetDirective targetDir(pCtx.getArena());
    targetDir.m_target = strTok.m_strVal;
    targetDir.m_ref = strTok.m_ref;
    module.m_target = std::move(targetDir);
    return true;
}

/**
 * Parses a global variable declaration (@name = [linkage] [const|var] type [= init];), builds the
 * MirGlobalVar, registers it in the context and the parser symbol table.
 */
bool MirParser::parseGlobalVarDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule & /*module*/)
{
    const auto nameTok = lexer.nextToken(); // Consume GlobalName (@...)
    std::string_view gvarName = nameTok.m_strVal;

    if (!matchToken(lexer, Parser::MirTokenKind::Equal, "Expected '=' after global variable name"))
    {
        return false;
    }

    // Linkage (external, internal, weak)
    MirGlobalVarLinkage linkage = MirGlobalVarLinkage::Internal;
    const auto &linkTok = lexer.peekToken();
    if (linkTok.m_kind == Parser::MirTokenKind::KwExternal)
    {
        linkage = MirGlobalVarLinkage::External;
        lexer.nextToken();
    }
    else if (linkTok.m_kind == Parser::MirTokenKind::KwInternal)
    {
        linkage = MirGlobalVarLinkage::Internal;
        lexer.nextToken();
    }
    else if (linkTok.m_kind == Parser::MirTokenKind::KwWeak)
    {
        linkage = MirGlobalVarLinkage::Weak;
        lexer.nextToken();
    }

    // Mutability (const, var)
    bool isConst = true;
    const auto &mutTok = lexer.peekToken();
    if (mutTok.m_kind == Parser::MirTokenKind::KwConst)
    {
        isConst = true;
        lexer.nextToken();
    }
    else if (mutTok.m_kind == Parser::MirTokenKind::KwVar)
    {
        isConst = false;
        lexer.nextToken();
    }

    // Type
    Ast::MirAstType *astType = parseAstType(lexer, pCtx);
    if (!astType)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Expected type in global variable declaration") << nameTok.m_ref;
        }
        return false;
    }
    MirType *type = pCtx.resolveType(astType);
    if (!type)
    {
        return false;
    }

    // Optional initializer: '=' ConstantInit OR direct ConstantInit (e.g. "Hello, World!\0A\00")
    std::optional<Ast::MirAstConstantInit> initOpt;
    if (lexer.peekToken().m_kind == Parser::MirTokenKind::Equal)
    {
        lexer.nextToken(); // Consume '='
        initOpt = parseConstantInit(lexer, pCtx);
    }
    else if (lexer.peekToken().m_kind == Parser::MirTokenKind::StringLiteral ||
             lexer.peekToken().m_kind == Parser::MirTokenKind::IntegerLiteral ||
             lexer.peekToken().m_kind == Parser::MirTokenKind::FloatLiteral ||
             lexer.peekToken().m_kind == Parser::MirTokenKind::LBracket)
    {
        initOpt = parseConstantInit(lexer, pCtx);
    }

    if (!matchToken(lexer, Parser::MirTokenKind::Semicolon, "Expected ';' after global variable declaration"))
    {
        pCtx.recordError();
        return false;
    }

    // Materialize MirGlobalVar
    MirGlobalVarBuilder gvBuilder(m_ctx);
    gvBuilder.setConstant(isConst);

    MirOperandBuilder opBuilder(m_ctx);
    if (initOpt.has_value())
    {
        if (initOpt->m_kind == Ast::ConstantKind::Integer)
        {
            MirInteger *imm = buildIntSafe(opBuilder, type, initOpt->m_intVal, initOpt->m_ref, m_diag, pCtx);
            if (imm)
            {
                gvBuilder.setInitializer(imm);
            }
        }
        else if (initOpt->m_kind == Ast::ConstantKind::Float)
        {
            try
            {
                gvBuilder.setInitializer(opBuilder.buildFloat(type, FlexFloat(initOpt->m_floatVal), initOpt->m_ref));
            }
            catch (const std::exception &ex)
            {
                if (m_diag)
                {
                    m_diag->error("MirParser", "Float literal error: {}", ex.what()) << initOpt->m_ref;
                }
                pCtx.recordError();
            }
        }
    }

    MirGlobalVar *gvar = gvBuilder.build(linkage, type, std::pmr::string(gvarName, pCtx.getArena()), nameTok.m_ref);
    if (m_ctx)
    {
        m_ctx->appendGlobalVar(gvar);
    }
    pCtx.declareGlobal(gvarName, gvar);
    return true;
}

/**
 * Parses a function prototype (declare @name(params) -> ret;), building a MirFunction with no body
 * and registering it in the parser symbol table.
 */
bool MirParser::parseFunctionDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule & /*module*/)
{
    lexer.nextToken(); // Consume 'declare'

    const auto nameTok = lexer.nextToken();
    if (nameTok.m_kind != Parser::MirTokenKind::GlobalName)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Expected function symbol name '@...' after 'declare'") << nameTok.m_ref;
        }
        return false;
    }
    std::string_view fnName = nameTok.m_strVal;

    if (!matchToken(lexer, Parser::MirTokenKind::LParen, "Expected '(' after function name"))
    {
        return false;
    }

    std::vector<MirType *> paramTypes;
    while (lexer.peekToken().m_kind != Parser::MirTokenKind::RParen &&
           lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
    {
        if (lexer.peekToken().m_kind == Parser::MirTokenKind::Ellipsis)
        {
            lexer.nextToken(); // Variadic args
            break;
        }

        Ast::MirAstType *astType = parseAstType(lexer, pCtx);
        if (astType)
        {
            if (MirType *t = pCtx.resolveType(astType))
            {
                paramTypes.push_back(t);
            }
        }

        if (lexer.peekToken().m_kind == Parser::MirTokenKind::Comma)
        {
            lexer.nextToken();
        }
    }

    if (!matchToken(lexer, Parser::MirTokenKind::RParen, "Expected ')' after parameter type list"))
    {
        return false;
    }

    if (!matchToken(lexer, Parser::MirTokenKind::Arrow, "Expected '->' after parameter list"))
    {
        return false;
    }

    Ast::MirAstType *retAstType = parseAstType(lexer, pCtx);
    MirType *retType = pCtx.resolveType(retAstType);
    if (!retType && m_ctx)
    {
        retType = m_ctx->getTypeTable()->_void();
    }

    if (!matchToken(lexer, Parser::MirTokenKind::Semicolon, "Expected ';' after function declaration"))
    {
        return false;
    }

    MirFunctionBuilder funcBuilder(m_ctx);
    MirFunction *func = funcBuilder.build(retType, {}, fnName, nullptr, nameTok.m_ref);
    pCtx.declareFunction(fnName, func);
    return true;
}

/**
 * Parses a function definition (fn @name(params) -> ret [attrs] { blocks }): builds the function
 * and its parameter registers, enters its scope and parses each basic block until the closing brace.
 */
bool MirParser::parseFunctionDef(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule & /*module*/)
{
    const auto fnTok = lexer.nextToken(); // Consume 'fn'

    const auto nameTok = lexer.nextToken();
    if (nameTok.m_kind != Parser::MirTokenKind::GlobalName)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Expected function name '@...' after 'fn'") << nameTok.m_ref;
        }
        return false;
    }
    std::string_view fnName = nameTok.m_strVal;

    if (!matchToken(lexer, Parser::MirTokenKind::LParen, "Expected '(' after function name"))
    {
        return false;
    }

    MirOperandBuilder opBuilder(m_ctx);
    std::vector<MirRegister *> params;

    while (lexer.peekToken().m_kind != Parser::MirTokenKind::RParen &&
           lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
    {
        Ast::MirAstType *astType = parseAstType(lexer, pCtx);
        MirType *paramType = pCtx.resolveType(astType);
        if (!paramType && m_ctx)
        {
            paramType = m_ctx->getTypeTable()->i64();
        }

        const auto &paramNameTok = lexer.nextToken();
        std::string_view paramName = paramNameTok.m_strVal;
        if (paramName.empty())
        {
            paramName = paramNameTok.m_text;
        }

        MirRegister *paramReg = opBuilder.buildVReg(paramType, paramName, paramNameTok.m_ref);
        params.push_back(paramReg);

        if (lexer.peekToken().m_kind == Parser::MirTokenKind::Comma)
        {
            lexer.nextToken();
        }
    }

    if (!matchToken(lexer, Parser::MirTokenKind::RParen, "Expected ')' after function parameters"))
    {
        return false;
    }

    if (!matchToken(lexer, Parser::MirTokenKind::Arrow, "Expected '->' after function header"))
    {
        return false;
    }

    Ast::MirAstType *retAstType = parseAstType(lexer, pCtx);
    MirType *retType = pCtx.resolveType(retAstType);
    if (!retType && m_ctx)
    {
        retType = m_ctx->getTypeTable()->_void();
    }

    // Optional attributes list: [attr1="val", ...]
    if (lexer.peekToken().m_kind == Parser::MirTokenKind::LBracket)
    {
        lexer.nextToken(); // Consume '['
        while (lexer.peekToken().m_kind != Parser::MirTokenKind::RBracket &&
               lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
        {
            lexer.nextToken(); // Consume attribute element
            if (lexer.peekToken().m_kind == Parser::MirTokenKind::Comma)
            {
                lexer.nextToken();
            }
        }
        matchToken(lexer, Parser::MirTokenKind::RBracket, "Expected ']' at end of attribute list");
    }

    if (!matchToken(lexer, Parser::MirTokenKind::LBrace, "Expected '{' to begin function body"))
    {
        return false;
    }

    MirFunctionBuilder funcBuilder(m_ctx);
    MirFunction *func = funcBuilder.build(retType, {}, fnName, nullptr, nameTok.m_ref);
    for (MirRegister *paramReg : params)
    {
        funcBuilder.addParam(func, paramReg);
    }

    pCtx.declareFunction(fnName, func);
    pCtx.enterFunction(func);

    while (lexer.peekToken().m_kind != Parser::MirTokenKind::RBrace &&
           lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
    {
        if (lexer.peekToken().m_kind == Parser::MirTokenKind::Semicolon)
        {
            lexer.nextToken();
            continue;
        }

        if (!parseBasicBlock(lexer, pCtx, func))
        {
            pCtx.exitFunction();
            return false;
        }
    }

    if (!matchToken(lexer, Parser::MirTokenKind::RBrace, "Expected '}' at end of function body"))
    {
        pCtx.exitFunction();
        return false;
    }

    pCtx.exitFunction();
    return true;
}

/**
 * Parses one labeled basic block (label: instructions...) by declaring/reusing the block and
 * parsing instruction statements until the next label, the function's closing brace or EOF.
 */
bool MirParser::parseBasicBlock(Parser::MirLexer &lexer, MirParserContext &pCtx, MirFunction *func)
{
    const auto labelTok = lexer.nextToken();
    std::string_view rawName = labelTok.m_strVal;
    if (rawName.empty())
    {
        rawName = labelTok.m_text;
    }
    if (rawName.starts_with("%"))
    {
        rawName = rawName.substr(1);
    }

    if (!matchToken(lexer, Parser::MirTokenKind::Colon, "Expected ':' after basic block label"))
    {
        return false;
    }

    MirBlock *block = pCtx.declareBlock(rawName, labelTok.m_ref);
    if (!block)
    {
        block = pCtx.getOrCreateBlock(rawName, labelTok.m_ref);
    }

    // Parse instructions until next block label, closing brace '}', or EOF
    while (lexer.peekToken().m_kind != Parser::MirTokenKind::RBrace &&
           lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
    {
        if (lexer.peekToken().m_kind == Parser::MirTokenKind::Semicolon)
        {
            lexer.nextToken();
            continue;
        }

        // A local name or identifier followed by ':' begins the next basic block label.
        const auto &peekTok = lexer.peekToken();
        if ((peekTok.m_kind == Parser::MirTokenKind::LocalName || peekTok.m_kind == Parser::MirTokenKind::Identifier) &&
            lexer.peekToken(1).m_kind == Parser::MirTokenKind::Colon)
        {
            const auto labelNameTok = lexer.nextToken();
            std::string_view nextBlockName =
                    labelNameTok.m_strVal.empty() ? labelNameTok.m_text : labelNameTok.m_strVal;
            if (nextBlockName.starts_with("%"))
            {
                nextBlockName = nextBlockName.substr(1);
            }
            lexer.nextToken(); // Consume ':'
            block = pCtx.declareBlock(nextBlockName, labelNameTok.m_ref);
            continue;
        }

        MirInstruction *inst = parseInstructionStatement(lexer, pCtx, block);
        if (!inst)
        {
            return false;
        }
    }

    return true;
}

/**
 * Parses a single instruction statement, supporting both assignment form (dst = opcode type ops;)
 * and prefix form (opcode type ops;), then appends it to block. Returns nullptr on error or when
 * the next token ends the block.
 */
MirInstruction *MirParser::parseInstructionStatement(Parser::MirLexer &lexer, MirParserContext &pCtx, MirBlock *block)
{
    const auto &firstTok = lexer.peekToken();
    if (firstTok.m_kind == Parser::MirTokenKind::EndOfFile || firstTok.m_kind == Parser::MirTokenKind::RBrace)
    {
        return nullptr;
    }

    std::vector<MirOperand *> operands;
    std::string_view opcodeName;
    SourceReference *instRef = firstTok.m_ref;
    MirType *instType = nullptr;

    if (firstTok.m_kind == Parser::MirTokenKind::LocalName)
    {
        auto dstTok = lexer.nextToken();
        if (!matchToken(lexer, Parser::MirTokenKind::Equal, "Expected '=' in instruction assignment"))
        {
            return nullptr;
        }

        const auto opTok = lexer.nextToken();
        opcodeName = opTok.m_text;
        instRef = opTok.m_ref;

        if (lexer.peekToken().m_kind >= Parser::MirTokenKind::TypeI1 &&
            lexer.peekToken().m_kind <= Parser::MirTokenKind::TypeToken)
        {
            instType = pCtx.resolveType(parseAstType(lexer, pCtx));
        }

        MirRegister *dstReg = pCtx.getOrCreateRegister(dstTok.m_text, instType, dstTok.m_ref);
        operands.push_back(dstReg);
    }
    else
    {
        auto opTok = lexer.nextToken();
        opcodeName = opTok.m_text;
        instRef = opTok.m_ref;

        if (lexer.peekToken().m_kind >= Parser::MirTokenKind::TypeI1 &&
            lexer.peekToken().m_kind <= Parser::MirTokenKind::TypeToken)
        {
            instType = pCtx.resolveType(parseAstType(lexer, pCtx));
        }
    }

    MirInstructionOpCode opCode = getOpCodeFromStr(opcodeName);
    if (opCode == static_cast<MirInstructionOpCode>(0))
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Unknown instruction opcode '{}'", opcodeName) << instRef;
        }
        pCtx.recordError();
        return nullptr;
    }

    bool operandError = false;
    while (lexer.peekToken().m_kind != Parser::MirTokenKind::Semicolon &&
           lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
    {
        MirOperand *op = parseOperand(lexer, pCtx, nullptr, operands.size(), instType);
        if (!op)
        {
            operandError = true;
            break;
        }
        operands.push_back(op);
        if (lexer.peekToken().m_kind == Parser::MirTokenKind::Comma)
        {
            lexer.nextToken();
        }
    }

    if (operandError || !matchToken(lexer, Parser::MirTokenKind::Semicolon, "Expected ';' after instruction"))
    {
        pCtx.recordError();
        return nullptr;
    }

    MirInstructionBuilder instBuilder(m_ctx, block, InsertionType::Append);
    return instBuilder.build(opCode, instRef, operands);
}

/**
 * Parses the body of a bracketed memory operand (base [+ index[*scale]] [+/- disp]) into a
 * MirMemory, defaulting the value type to i64 when none was given.
 */
MirMemory *MirParser::parseMemoryOperand(Parser::MirLexer &lexer,
                                         MirParserContext &pCtx,
                                         MirType *memType,
                                         SourceReference *startRef)
{
    // Inside '['
    if (lexer.peekToken().m_kind == Parser::MirTokenKind::TypePtr)
    {
        lexer.nextToken(); // Consume optional 'ptr'
    }

    const auto baseTok = lexer.nextToken();
    MirType *ptrType = m_ctx->getTypeTable()->getPtr(m_ctx->getTypeTable()->i8());
    MirRegister *baseReg = pCtx.getOrCreateRegister(baseTok.m_text, ptrType, baseTok.m_ref);

    MirRegister *indexReg = nullptr;
    uint8_t scale = 1;
    int64_t disp = 0;

    while (lexer.peekToken().m_kind == Parser::MirTokenKind::Plus ||
           lexer.peekToken().m_kind == Parser::MirTokenKind::Minus)
    {
        bool isPlus = (lexer.peekToken().m_kind == Parser::MirTokenKind::Plus);
        lexer.nextToken(); // Consume '+' or '-'

        const auto &next = lexer.peekToken();
        if (next.m_kind == Parser::MirTokenKind::LocalName)
        {
            auto idxTok = lexer.nextToken();
            indexReg = pCtx.getOrCreateRegister(idxTok.m_text, m_ctx->getTypeTable()->i64(), idxTok.m_ref);
            if (lexer.peekToken().m_kind == Parser::MirTokenKind::Star)
            {
                lexer.nextToken(); // Consume '*'
                const auto &scaleTok = lexer.nextToken();
                scale = static_cast<uint8_t>(scaleTok.m_intVal);
            }
        }
        else if (next.m_kind == Parser::MirTokenKind::IntegerLiteral)
        {
            auto immTok = lexer.nextToken();
            disp += isPlus ? immTok.m_intVal : -immTok.m_intVal;
        }
    }

    if (!matchToken(lexer, Parser::MirTokenKind::RBracket, "Expected ']' at end of memory operand"))
    {
        pCtx.recordError();
        return nullptr;
    }

    MirOperandBuilder opBuilder(m_ctx);
    if (!memType)
    {
        memType = m_ctx->getTypeTable()->i64();
    }

    FlexInt displVal(disp, 64);
    return opBuilder.buildMem(memType, baseReg, displVal, indexReg, scale, startRef);
}

/**
 * Parses one instruction operand, dispatching on the token: label references, bracketed memory or
 * PHI pairs, @global/@function/runtime symbols, %stack slots, %register class bindings, integer
 * and float immediates, and explicit type prefixes. Unresolved symbols are queued as forward
 * references.
 */
MirOperand *MirParser::parseOperand(Parser::MirLexer &lexer,
                                    MirParserContext &pCtx,
                                    MirInstruction *targetInst,
                                    size_t operandIdx,
                                    MirType *expectedType)
{
    const auto &tok = lexer.peekToken();
    MirOperandBuilder opBuilder(m_ctx);

    // Label reference: label %block_name
    if (tok.m_kind == Parser::MirTokenKind::KwLabel)
    {
        lexer.nextToken(); // Consume 'label'
        const auto lblTok = lexer.nextToken();
        std::string_view name = lblTok.m_strVal.empty() ? lblTok.m_text : lblTok.m_strVal;
        if (name.starts_with("%"))
        {
            name = name.substr(1);
        }
        MirBlock *blk = pCtx.getOrCreateBlock(name, lblTok.m_ref);
        return opBuilder.buildRef(blk, lblTok.m_ref);
    }

    // Memory or Phi incoming pair: [ ... ]
    if (tok.m_kind == Parser::MirTokenKind::LBracket)
    {
        auto startRef = tok.m_ref;
        lexer.nextToken(); // Consume '['

        // Check if PHI pair: [%v3, label %then_block] or [%v3, %then_block]
        const auto &firstItem = lexer.peekToken();
        if (firstItem.m_kind == Parser::MirTokenKind::LocalName ||
            firstItem.m_kind == Parser::MirTokenKind::IntegerLiteral ||
            firstItem.m_kind == Parser::MirTokenKind::FloatLiteral)
        {
            auto valTok = lexer.nextToken();
            if (lexer.peekToken().m_kind == Parser::MirTokenKind::Comma)
            {
                lexer.nextToken(); // Consume ','
                if (lexer.peekToken().m_kind == Parser::MirTokenKind::KwLabel)
                {
                    lexer.nextToken(); // Consume 'label'
                }
                auto blkTok = lexer.nextToken();
                if (!matchToken(lexer, Parser::MirTokenKind::RBracket, "Expected ']' at end of phi incoming pair"))
                {
                    pCtx.recordError();
                    return nullptr;
                }

                std::string_view bName = blkTok.m_strVal.empty() ? blkTok.m_text : blkTok.m_strVal;
                if (bName.starts_with("%"))
                {
                    bName = bName.substr(1);
                }
                pCtx.getOrCreateBlock(bName, blkTok.m_ref);

                // Build operand for incoming value
                if (valTok.m_kind == Parser::MirTokenKind::LocalName)
                {
                    return pCtx.getOrCreateRegister(valTok.m_text, expectedType, valTok.m_ref);
                }
                else if (valTok.m_kind == Parser::MirTokenKind::IntegerLiteral)
                {
                    MirType *immType = (expectedType && expectedType->getKind() == MirTypeKind::Integer)
                            ? expectedType
                            : m_ctx->getTypeTable()->i64();
                    return buildIntSafe(opBuilder, immType, valTok.m_intVal, valTok.m_ref, m_diag, pCtx);
                }
            }

            // Not a PHI pair: it is a memory operand whose base was already consumed.
            // Push the base token back and let the shared memory-operand parser handle the rest.
            lexer.pushBack(std::move(valTok));
            return parseMemoryOperand(lexer, pCtx, expectedType, startRef);
        }

        return parseMemoryOperand(lexer, pCtx, expectedType, startRef);
    }

    // Global reference / Function / Runtime symbol: @...
    if (tok.m_kind == Parser::MirTokenKind::GlobalName)
    {
        auto gTok = lexer.nextToken();
        std::string_view symName = gTok.m_strVal.empty() ? gTok.m_text : gTok.m_strVal;

        int64_t offset = 0;
        if (lexer.peekToken().m_kind == Parser::MirTokenKind::Plus)
        {
            lexer.nextToken();
            const auto &offTok = lexer.nextToken();
            offset = offTok.m_intVal;
        }

        if (MirGlobalVar *gv = pCtx.resolveGlobal(symName, gTok.m_ref))
        {
            return opBuilder.buildRef(gv, static_cast<size_t>(offset), gTok.m_ref);
        }
        if (MirFunction *fn = pCtx.resolveFunction(symName, gTok.m_ref))
        {
            return opBuilder.buildRef(fn, gTok.m_ref);
        }

        // Check if forward reference or runtime symbol
        pCtx.recordForwardReference(symName, targetInst, operandIdx, SymbolKind::Function, gTok.m_ref);
        return opBuilder.buildRtSymbol(std::pmr::string(symName, pCtx.getArena()), gTok.m_ref);
    }

    // Stack reference: %stack[0] or LocalName register %v0
    if (tok.m_kind == Parser::MirTokenKind::LocalName)
    {
        if (tok.m_text.starts_with("%stack") || tok.m_text == "%stack")
        {
            auto sTok = lexer.nextToken();
            if (!matchToken(lexer, Parser::MirTokenKind::LBracket, "Expected '[' after '%stack'"))
            {
                pCtx.recordError();
                return nullptr;
            }
            const auto &idxTok = lexer.nextToken();
            if (!matchToken(lexer, Parser::MirTokenKind::RBracket, "Expected ']' after stack slot index"))
            {
                pCtx.recordError();
                return nullptr;
            }

            MirFunction *curFn = pCtx.getCurrentFunction();
            if (curFn && curFn->getStackFrame())
            {
                size_t slotId = static_cast<size_t>(idxTok.m_intVal);
                const auto &objs = curFn->getStackFrame()->getObjects();
                if (slotId < objs.size())
                {
                    return opBuilder.buildRef(objs[slotId], sTok.m_ref);
                }
            }
            MirType *stkType = expectedType ? expectedType : m_ctx->getTypeTable()->i64();
            return buildIntSafe(opBuilder, stkType, idxTok.m_intVal, sTok.m_ref, m_diag, pCtx);
        }

        auto regTok = lexer.nextToken();
        std::string_view regName = regTok.m_text;

        // Class binding: %p0(rax:GPR64). The asm-name/class pair is parsed and currently dropped
        // because the parser has no target register-class registry to bind it against; malformed
        // bindings are rejected so the token stream cannot desync.
        if (lexer.peekToken().m_kind == Parser::MirTokenKind::LParen)
        {
            lexer.nextToken(); // Consume '('
            while (lexer.peekToken().m_kind != Parser::MirTokenKind::RParen &&
                   lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
            {
                lexer.nextToken();
            }
            if (!matchToken(lexer, Parser::MirTokenKind::RParen, "Expected ')' after register class binding"))
            {
                pCtx.recordError();
                return nullptr;
            }
        }

        return pCtx.getOrCreateRegister(regName, expectedType, regTok.m_ref);
    }

    // Integer immediate: 42, 0x10
    if (tok.m_kind == Parser::MirTokenKind::IntegerLiteral)
    {
        auto immTok = lexer.nextToken();
        MirType *immType = nullptr;
        if (expectedType && expectedType->getKind() == MirTypeKind::Integer)
        {
            size_t bw = expectedType->getTotalSizeInBits();
            if (bw < 64)
            {
                uint64_t maxUnsigned = (bw == 64) ? ~0ULL : ((1ULL << bw) - 1ULL);
                int64_t minSigned = -(1LL << (bw - 1));
                int64_t maxSigned = (1LL << (bw - 1)) - 1;
                if ((immTok.m_intVal >= 0 && static_cast<uint64_t>(immTok.m_intVal) <= maxUnsigned) ||
                    (immTok.m_intVal < 0 && immTok.m_intVal >= minSigned && immTok.m_intVal <= maxSigned))
                {
                    immType = expectedType;
                }
            }
            else
            {
                immType = expectedType;
            }
        }
        if (!immType)
        {
            immType = m_ctx->getTypeTable()->i64();
        }
        return buildIntSafe(opBuilder, immType, immTok.m_intVal, immTok.m_ref, m_diag, pCtx);
    }

    // Float immediate: 3.14
    if (tok.m_kind == Parser::MirTokenKind::FloatLiteral)
    {
        auto fTok = lexer.nextToken();
        MirType *fType = (expectedType && expectedType->getKind() == MirTypeKind::FloatingPoint)
                ? expectedType
                : m_ctx->getTypeTable()->f64();
        try
        {
            return opBuilder.buildFloat(fType, FlexFloat(fTok.m_floatVal), fTok.m_ref);
        }
        catch (const std::exception &ex)
        {
            if (m_diag)
            {
                m_diag->error("MirParser", "Float literal error: {}", ex.what()) << fTok.m_ref;
            }
            pCtx.recordError();
            return nullptr;
        }
    }

    // Explicit Type prefix on operand: e.g. i64 %v0, ptr [ptr %v0 + 8], i64 42
    if ((tok.m_kind >= Parser::MirTokenKind::TypeI1 && tok.m_kind <= Parser::MirTokenKind::TypeToken) ||
        tok.m_kind == Parser::MirTokenKind::TypePtr)
    {
        Ast::MirAstType *astType = parseAstType(lexer, pCtx);
        MirType *explicitType = pCtx.resolveType(astType);
        return parseOperand(lexer, pCtx, targetInst, operandIdx, explicitType);
    }

    // Unknown operand
    if (m_diag)
    {
        m_diag->error("MirParser", "Unexpected operand token '{}'", tok.m_text) << tok.m_ref;
    }
    pCtx.recordError();
    lexer.nextToken();
    return nullptr;
}

/**
 * Dispatches a top-level construct to the target/global/declare/fn parser, reporting an error for
 * any other token.
 */
bool MirParser::parseTopLevelDecl(Parser::MirLexer &lexer, MirParserContext &pCtx, Ast::MirAstModule &module)
{
    const auto &tok = lexer.peekToken();

    if (tok.m_kind == Parser::MirTokenKind::KwTarget ||
        (tok.m_kind == Parser::MirTokenKind::Identifier && tok.m_text == "target"))
    {
        return parseTargetDirective(lexer, pCtx, module);
    }

    if (tok.m_kind == Parser::MirTokenKind::GlobalName)
    {
        return parseGlobalVarDecl(lexer, pCtx, module);
    }

    if (tok.m_kind == Parser::MirTokenKind::KwDeclare)
    {
        return parseFunctionDecl(lexer, pCtx, module);
    }

    if (tok.m_kind == Parser::MirTokenKind::KwFn)
    {
        return parseFunctionDef(lexer, pCtx, module);
    }

    if (m_diag)
    {
        m_diag->error("MirParser", "Unexpected top-level token '{}'", tok.m_text) << tok.m_ref;
    }
    return false;
}

/**
 * Parses a complete module from source into the builder context: creates a local source manager
 * and parser context, loops over top-level declarations, resolves all pending forward references
 * and reports success. Exceptions are caught and reported as diagnostics.
 */
bool MirParser::parseModule(std::string_view source, std::string_view bufferName)
{
    if (!m_ctx)
    {
        return false;
    }

    try
    {
        auto *alloc = m_ctx->getGlobalAllocator();

        // Prefer the caller-owned manager so its buffers outlive the MIR it describes; otherwise
        // fall back to a manager owned for the duration of this parse.
        std::unique_ptr<SourceManager> ownedSourceMgr;
        GenericSourceManager *sourceMgr = m_sourceMgr;
        if (!sourceMgr)
        {
            ownedSourceMgr = std::make_unique<SourceManager>(std::filesystem::current_path(), alloc);
            sourceMgr = ownedSourceMgr.get();
        }
        size_t sourceId = sourceMgr->addSourceContent(std::string(bufferName), source);

        MirParserContext pCtx(m_ctx, m_diag, alloc, sourceMgr, sourceId);
        Parser::MirLexer lexer(source, pCtx);
        Ast::MirAstModule module(pCtx.getArena());

        while (lexer.peekToken().m_kind != Parser::MirTokenKind::EndOfFile)
        {
            if (lexer.peekToken().m_kind == Parser::MirTokenKind::Semicolon)
            {
                lexer.nextToken();
                continue;
            }

            if (!parseTopLevelDecl(lexer, pCtx, module))
            {
                return false;
            }
        }

        if (!pCtx.resolveAllPendingFixups())
        {
            return false;
        }

        if (pCtx.hasErrors())
        {
            return false;
        }

        return true;
    }
    catch (const std::exception &ex)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Parsing failed with exception: {}", ex.what());
        }
        return false;
    }
    catch (...)
    {
        if (m_diag)
        {
            m_diag->error("MirParser", "Parsing failed with unknown exception");
        }
        return false;
    }
}

} // namespace EzMir
