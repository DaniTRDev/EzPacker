/**
 * @file SwitchAstNode.h
 * @brief AST node for a parsed `switch` statement.
 *
 * A SwitchAstNode groups three pieces of information:
 * - the selector variable used by `switch (...)`,
 * - an ordered slice of explicit `case` clauses,
 * - an optional `default` body.
 */
#ifndef EZPACKER_SWITCHASTNODE_H
#define EZPACKER_SWITCHASTNODE_H

#include "EzLexerCommon.h"
#include "CodeScope.h"
#include "ConditionAstNode.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"

class SwitchAstNode : public AstNode
{
  public:
    /**
     * Creates a switch node.
     *
     * @param _default Optional default case body. May be nullptr.
     * @param switchVariable Selector variable from `switch (%var)`.
     */
    SwitchAstNode(CodeScope *_default, Variable *switchVariable);

    /**
     * Returns AstNodeType::Switch.
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
     * Returns "SwitchAstNode".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the default-case body, or nullptr when no `default` clause was
     * present in the source.
     */
    CodeScope *getDefault();

    /**
     * Returns the explicit cases in source order.
     *
     * Elements are expected to be SwitchCaseAstNode instances.
     */
    TypedPoolSlice<AstNode> *getCases();

    /**
     * Returns the selector variable evaluated by the switch statement.
     */
    Variable *getSwitchVariable();

    /**
     * Sets the cases of the switch.
     * @param cases
     */
    void setCases(TypedPoolSlice<AstNode> * cases);
    
  private:
    CodeScope *m_default;
    TypedPoolSlice<AstNode> *m_cases;
    Variable *m_switchVariable;
};

#endif // EZPACKER_SWITCHASTNODE_H
