#ifndef EZPACKER_LABEL_H
#define EZPACKER_LABEL_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNodes/Instruction.h"

using LabelExpression = std::variant<std::shared_ptr<Instruction>, std::shared_ptr<class Label>>;

inline std::shared_ptr<AstNode> GetLabelExpressionAsNode(LabelExpression expression)
{
    if (std::holds_alternative<std::shared_ptr<class Label>>(expression))
    {
        return std::dynamic_pointer_cast<AstNode>(std::get<std::shared_ptr<class Label>>(expression));
    }
    else
    {
        return std::dynamic_pointer_cast<AstNode>(std::get<std::shared_ptr<Instruction>>(expression));
    }

    return nullptr;
}

/**
 * This class represents a label, which is a specific region inside the body of a module.
 */
class Label : public AstNode
{
  public:
    /**
     * Creates the label with the given name and expressions.
     * @param name
     * @param expressions
     */
    Label(std::string name, std::vector<LabelExpression> expressions);

    /**
     * Returns AstNodeType::Label.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns "Label".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. Shows label names and nested labels
     * or instructions. Mode is passed to expression AST nodes.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

    /**
     * Returns the name of the label.
     * @return const std::string &
     */
    const std::string &getLabelName() const;

    /**
     * Returns the vector of expressions defined in this label.
     * @return const std::vector<LabelExpression> &
     */
    const std::vector<LabelExpression> &getExpressions() const;

  private:
    std::string m_name;
    std::vector<LabelExpression> m_expressions;
};

#endif // EZPACKER_LABEL_H
