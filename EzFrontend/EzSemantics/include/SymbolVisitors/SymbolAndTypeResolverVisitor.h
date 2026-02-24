#ifndef EZPACKER_SYMBOLANDTYPERESOLVERVISITOR_H
#define EZPACKER_SYMBOLANDTYPERESOLVERVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "Scope/Scope.h"
#include "SemanticAnnotations/DataTypeAnnotation.h"
#include "SemanticAnnotations/ScopeAnnotation.h"
#include "SemanticAnnotations/ScopedSymbolAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"

/**
 * This resolves every symbol. If a symbol is used but not defined, an error is thrown. It also resolves types
 * used in memory operands.
 */
class SymbolAndTypeResolverVisitor : public SemanticVisitor
{
  public:
    /**
     * Visits given instruction node. If instruction uses virtual variables, this visitor will check if they have been
     * previously defined. If instruction == create, it returns true without doing nothing.
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
     * Visits given Memory operand node. If the operand uses virtual variables, this visitor will check if they have
     * been previously defined. It will also create the corresponding type, using POINTER as default (if no type is
     * provided).
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
     * Visits given Variable node. Resolves the symbol this variables represent.
     * @param var
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Variable> &var) override;
};

#endif // EZPACKER_SYMBOLANDTYPERESOLVERVISITOR_H
