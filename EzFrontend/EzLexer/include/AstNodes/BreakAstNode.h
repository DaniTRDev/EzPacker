#ifndef EZPACKER_BREAKASTNODE_H
#define EZPACKER_BREAKASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"

/**
 * @brief AST node representing a `break` statement.
 *
 * A BreakAstNode is a leaf node (no children) produced by the parser when it
 * encounters the `break;` keyword inside a loop body.  During semantic
 * analysis the TypeCheckVisitor verifies that this node actually appears
 * within a while-loop; during lowering the BreakLowerer emits a JMP to the
 * enclosing loop's exit block.
 */
class BreakAstNode : public AstNode
{
  public:
    /**
     * Constructs an empty BreakAstNode.
     */
    BreakAstNode();

    /**
     * Returns AstNodeType::Break.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its visit(BreakAstNode*) overload.
     * Returns the result of that visit call.
     * @param visitor
     * @return bool
     */
    bool accept(class AstNodeVisitor *visitor) override;

    /**
     * Returns "BreakAstNode".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns a human-readable string for this node (same as getAstNodeName,
     * since break has no children or extra data).
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;
};

#endif // EZPACKER_BREAKASTNODE_H
