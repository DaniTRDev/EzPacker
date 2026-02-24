#ifndef EZPACKER_WHILEASTNODE_H
#define EZPACKER_WHILEASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNodes/CodeScope.h"
#include "AstNodes/ConditionAstNode.h"

class WhileAstNode : public AstNode
{
  public:
    /**
     * Creates the node.
     */
    WhileAstNode();

    /**
     * Returns 'While' as the type of this node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns 'WhileAstNode'.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Sets the code scope for this while loop.
     * @param codeScope
     */
    void setCodeScope(const std::shared_ptr<CodeScope> &codeScope);

    /**
     * Sets the condition for this while loop.
     * @param condition
     */
    void setCondition(const std::shared_ptr<ConditionAstNode> &condition);

    /**
     * Gets the condition of this while loop.
     * @return const std::shared_ptr<ConditionAstNode> &
     */
    const std::shared_ptr<ConditionAstNode> &getCondition() const;

    /**
     * Gets the code scope of this while loop.
     * @return const std::shared_ptr<CodeScope> &
     */
    const std::shared_ptr<CodeScope> &getCodeScope() const;

    /**
     * Returns a string representation of this node. This is described as:
     * while (condition)
     * {CodeScope::getAsStr(mode)}
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    std::shared_ptr<ConditionAstNode> m_condition;
    std::shared_ptr<CodeScope> m_codeScope;
};

#endif // EZPACKER_WHILEASTNODE_H
