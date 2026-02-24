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
    /**
     * Visits given CodeScope node. Recursively visits sub code scopes.
     * @param scope
     * @return bool
     */
    bool visit(const std::shared_ptr<struct CodeScope> &scope) override;
    
    /**
     * Visits given instruction node. Recursively visits operands.
     * @param instr
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Instruction> &instr) override;

    /**
     * Visits given Label operand node. Recursively visits sub labels and instructions.
     * @param label
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Label> &label) override;

    /**
     * Visits given Memory operand node. Visits used variable nodes (if any).
     * @param operand
     * @return bool
     */
    bool visit(const std::shared_ptr<struct MemoryOperandAstNode> &operand) override;

    /**
     * Visits given Module node. Recursively visits child instructions and labels.
     * @param module
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Module> &module) override;

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
