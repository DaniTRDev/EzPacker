#ifndef EZPACKER_CONTINUEASTNODE_H
#define EZPACKER_CONTINUEASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"

/**
 * @brief AST node representing a `continue` statement.
 *
 * A ContinueAstNode is a leaf node (no children) produced by the parser when
 * it encounters the `continue;` keyword inside a loop body.  During semantic
 * analysis the TypeCheckVisitor verifies that this node actually appears
 * within a while-loop; during lowering the ContinueLowerer emits a JMP back
 * to the enclosing loop's condition-check block.
 */
class ContinueAstNode : public AstNode
{
  public:
    /**
     * Constructs an empty ContinueAstNode.
     */
    ContinueAstNode();

    /**
     * Returns AstNodeType::Continue.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its visit(ContinueAstNode*) overload.
     * Returns the result of that visit call.
     * @param visitor
     * @return bool
     */
    bool accept(class AstNodeVisitor *visitor) override;

    /**
     * Returns "ContinueAstNode".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns a human-readable string for this node (same as getAstNodeName,
     * since continue has no children or extra data).
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;
};

#endif // EZPACKER_CONTINUEASTNODE_H
