#ifndef EZPACKER_CONTINUEASTNODE_H
#define EZPACKER_CONTINUEASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"

class ContinueAstNode : public AstNode
{
  public:
    /**
     * Creates the node.
     */
    ContinueAstNode();

    /**
     * Returns AstNodeType::Continue.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type.Returns
     * the result of visit.
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
     * Returns the same as getAstNodeName, since this node has no children.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;
};

#endif // EZPACKER_CONTINUEASTNODE_H
