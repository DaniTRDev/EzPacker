/**
 * @file Label.h
 * @brief AST node for a named label with its own code scope: `name: { … }`.
 *
 * A Label defines a named entry point inside a module body.  It owns a
 * CodeScope containing the statements that belong to that label.  During
 * lowering, LabelLowerer creates a new MIR basic block and links the
 * label's symbol to that block's ID.
 */
#ifndef EZPACKER_LABEL_H
#define EZPACKER_LABEL_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNodes/CodeScope.h"
#include "AstNodes/Instruction.h"
#include "AstNode/AstNodeVisitor.h"

/**
 * This class represents a label, which is a specific region inside the body of a module.
 */
class Label : public AstNode
{
  public:
    /**
     * Creates the label with the given name. The code scope must be set separately via setCodeScope().
     * @param name
     */
    Label(std::string_view name);

    /**
     * Returns AstNodeType::Label.
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
     * Returns the code scope of the label.
     * @return CodeScope*
     */
    CodeScope *getCodeScope() const;

    /**
     * Returns "Label".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Sets the code scope of the label.
     * @param codeScope
     */
    void setCodeScope(CodeScope *codeScope);
    
    /**
     * Returns the name of the label.
     * @return const std::string_view &
     */
    const std::string_view &getLabelName() const;

  private:
    CodeScope *m_codeScope;
    std::string_view m_name;
};

#endif // EZPACKER_LABEL_H
