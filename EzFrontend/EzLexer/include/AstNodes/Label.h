#ifndef EZPACKER_LABEL_H
#define EZPACKER_LABEL_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNodes/CodeScope.h"
#include "AstNodes/Instruction.h"

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
    Label(std::string name);

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
     * Sets the code scope of the label.
     * @param codeScope
     */
    void setCodeScope(const std::shared_ptr<CodeScope> &codeScope);

    /**
     * Returns the code scope of the label.
     * @return const std::shared_ptr<CodeScope> &
     */
    const std::shared_ptr<CodeScope> &getCodeScope() const;

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

  private:
    std::string m_name;
    std::shared_ptr<CodeScope> m_codeScope;
};

#endif // EZPACKER_LABEL_H
