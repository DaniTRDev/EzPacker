/**
 * @file CodeScope.h
 * @brief AST node for a brace-delimited block of statements: `{ … }`.
 *
 * A CodeScope is both an AstNode and an AstNodeContainer — it owns an
 * ordered list of child expressions (instructions, labels, nested
 * control-flow, break/continue, etc.).  It serves as the body for labels,
 * if/else branches, while loops, and module definitions.
 */
#ifndef EZPACKER_CODESCOPE_H
#define EZPACKER_CODESCOPE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNode/AstNodeContainer.h"

class CodeScope : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Returns AstNodeType::CodeScope.
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
     * Returns "CodeScope".
     * @return const char*
     */
    const char *getAstNodeName() const override;
};

#endif // EZPACKER_CODESCOPE_H
