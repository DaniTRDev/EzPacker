#ifndef EZPACKER_SYMBOLDEFINITIONVISITOR_H
#define EZPACKER_SYMBOLDEFINITIONVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "SemanticVisitor.h"
#include "Scope/Scope.h"
#include "SemanticAnnotations/ScopeAnnotation.h"
#include "SemanticAnnotations/ScopedSymbolAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"

/**
 * This visitor (pass) will only take care of creating scopes and symbols. Any further check will be done in other
 * place.
 */
class SymbolDefinitionVisitor : public SemanticVisitor
{
  public:
    /**
     * Visits given instruction node. It will only do something on "create" instruction, which is a language
     * keyword used to create local variables.
     * @param instr
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Instruction> &instr) override;

    /**
     * Visits given Label operand node. It will create the symbol of the label, and will populate label's internal scope
     * with other scopes / symbols from nested expression.
     * @param label
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Label> &label) override;

    /**
     * Visits given Module node. It will create its symbol and will populate module's scope with other scopes / symbols
     * from nested expression.
     * @param module
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Module> &module) override;

    /**
     * Visits given Variable node. This visitor will only be called for **GLOBAL VARIABLES.
     * @param variable
     * @return bool
     */
    bool visit(const std::shared_ptr<struct Variable> &variable) override;
};

#endif // EZPACKER_SYMBOLDEFINITIONVISITOR_H
