/**
 * @file Module.h
 * @brief AST nodes for top-level module declarations.
 *
 * In EzLexer, a module is the closest equivalent to a function definition:
 *
 * `returnType ModuleName(param1, param2) { ...body... }`
 *
 * The syntax tree intentionally splits the declaration into:
 * - ModuleHeader: signature information only.
 * - Module:       pair of header + body scope.
 *
 * This keeps signature processing independent from body traversal.
 */
#ifndef EZPACKER_MODULE_H
#define EZPACKER_MODULE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNodes/CodeScope.h"
#include "AstNodes/Instruction.h"
#include "AstNodes/Label.h"

/**
 * Signature portion of a module declaration.
 *
 * The inherited AstNodeContainer stores parameters as Variable nodes in source
 * order. Parameter nodes keep their parsed type spelling and `%name` exactly as
 * written by the user (after string interning).
 */
class ModuleHeader : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Creates a module header.
     *
     * @param parameters Parameter slice, usually containing Variable nodes.
     * @param moduleName Module/function name.
     * @param returnType Parsed return type spelling.
     */
    ModuleHeader(TypedPoolSlice<AstNode> *parameters, std::string_view moduleName, std::string_view returnType);

    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;
    
    /**
     * Returns the name of this AstNode.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the declared module name.
     */
    const std::string_view &getModuleName() const;

    /**
     * Returns the declared return type spelling.
     *
     * Type resolution is deferred to EzSemantics.
     */
    const std::string_view &getReturnTypeName() const;
    
  private:
    std::string_view m_moduleName;
    std::string_view m_returnType;
};

/**
 * Complete top-level module node.
 *
 * A successful ModuleParser always produces both parts:
 * - a non-null header,
 * - a non-null body CodeScope.
 */
class Module : public AstNode
{
  public:
    /**
     * Creates a module node from its parsed body and header.
     */
    Module(CodeScope *body, ModuleHeader *header);

    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;
    
    /**
     * Returns the body scope that contains the module statements.
     */
    CodeScope *getBody() const;

    /**
     * Returns the name of this AstNode.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the parsed signature information for this module.
     */
    ModuleHeader *getHeader() const;
    
  private:
    CodeScope *m_body;
    ModuleHeader *m_header;
};

#endif // EZPACKER_MODULE_H
