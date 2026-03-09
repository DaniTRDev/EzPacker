#ifndef EZPACKER_FORASTNODE_H
#define EZPACKER_FORASTNODE_H

#include "EzLexerCommon.h"
#include "CodeScope.h"
#include "ConditionAstNode.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"

class ForAstNode : public AstNode
{
  public:
    /**
     * Constructor for ForAstNode.
     * @param body
     * @param initialization
     * @param nextIt
     * @param condition
     */
    ForAstNode(CodeScope *body, CodeScope *initialization, CodeScope *nextIt, ConditionAstNode *condition);

    /**
     * Returns AstNodeType::For.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts a visitor for traversing the AST.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;

    /**
     * Returns "ForAstNode".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the body of the for loop.
     * @return CodeScope *
     */
    CodeScope *getBody() const;

    /**
     * Returns the initialization clause of the for loop.
     * @return CodeScope *
     */
    CodeScope *getInitialization() const;

    /**
     * Returns the code to execute when the current iteration finished.
     * @return CodeScope *
     */
    CodeScope *getNextItClause() const;

    /**
     * Returns the condition of the for loop.
     * @return ConditionAstNode *
     */
    ConditionAstNode *getCondition() const;

  private:
    CodeScope *m_body;
    CodeScope *m_initialization;
    CodeScope *m_nextIt;
    ConditionAstNode *m_condition;
};

#endif // EZPACKER_FORASTNODE_H
