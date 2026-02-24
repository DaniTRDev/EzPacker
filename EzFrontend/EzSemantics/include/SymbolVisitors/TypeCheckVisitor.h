#ifndef EZPACKER_TYPECHECKVISITOR_H
#define EZPACKER_TYPECHECKVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "Scope/TypeTable.h"
#include "SemanticAnnotations/DataTypeAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"
#include "SemanticAnnotations/TypeCastAnnotation.h"

class TypeCheckVisitor : public SemanticVisitor
{
  public:
    // Needed to include every visit method from SemanticVisitor. TODO: Remove after changing to an "accept" design.
    using SemanticVisitor::visit;

    /**
     * Visits given instruction node. Recursively visits operands.
     * @param instr
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Instruction> &instr) override;

    /**
     * Visits given Memory operand node. Visits used variable nodes (if any).
     * @param operand
     * @return bool
     */
    bool visit(const std::shared_ptr<struct MemoryOperandAstNode> &operand) override;

    /**
     * Visits given Variable node. This visitor will check for the types of
     * the symbol this variable references and the used type. If these types do not match will change node's annotation
     * to a TypeCastAnnotation.
     * @param var
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Variable> &var) override;

  private:
};

#endif // EZPACKER_TYPECHECKVISITOR_H
