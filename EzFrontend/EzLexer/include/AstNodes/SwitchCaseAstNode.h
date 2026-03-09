#ifndef EZPACKER_SWITCHCASEASTNODE_H
#define EZPACKER_SWITCHCASEASTNODE_H

#include "EzLexerCommon.h"
#include "CodeScope.h"
#include "ImmediateOperand.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"

class SwitchCaseAstNode : public AstNode
{
  public:
    /**
     * Creates a new SwitchCaseAstNode with the given case value and code scope. The case value is the value that will
     * be compared against the switch expression, and the code scope is the body of the case that will be executed if
     * the case value matches the switch expression.
     * @param caseScope
     * @param caseValue
     * @param ownerSwitch
     */
    SwitchCaseAstNode(CodeScope *caseScope, ImmediateOperand *caseValue);

    /**
     * Returns AstNodeType::SwitchCase.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts a visitor that will perform some operation on this node.
     * @param visitor
     * @return
     */
    bool accept(AstNodeVisitor *visitor) override;

    /**
     * Returns "SwitchCase".
     * @return const char *
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the code scope associated with this case.
     * @return CodeScope *
     */
    CodeScope *getBody();

    /**
     * Returns the case value for this case. This is the value that will be compared against the switch expression.
     * @return ImmediateOperand *
     */
    ImmediateOperand *getCaseValue();

  private:
    CodeScope *m_caseBody;
    ImmediateOperand *m_caseValue;
};

#endif // EZPACKER_SWITCHCASEASTNODE_H
