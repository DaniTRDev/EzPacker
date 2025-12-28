#ifndef EZPACKER_SYMBOLRESOLVERVISITOR_H
#define EZPACKER_SYMBOLRESOLVERVISITOR_H

#include "EzSemanticsCommon.h"
#include "Symbol.h"
#include "SymbolAnnotation.h"
#include "SemanticVisitor.h"
#include "ISemanticAnalyzerContext.h"

class SymbolResolverVisitor : public SemanticVisitor
{
  public:
    /**
     * Visits given instruction node. Should return true visitor wants to keep traversing the tree.
     * @param instr
     * @return bool
     */
    bool visit(Instruction *instr) override;

    /**
     * Visits given Label operand node. Should return true visitor wants to keep traversing the tree.
     * @param label
     * @return bool
     */
    bool visit(Label *label) override;

    /**
     * Visits given Module node. Should return true visitor wants to keep traversing the tree.
     * @param module
     * @return bool
     */
    bool visit(Module *module) override;

    /**
     * Visits given Variable node. Should return true visitor wants to keep traversing the tree.
     * @param var
     * @return bool
     */
    bool visit(Variable *var) override;
    
  private:
};
#endif // EZPACKER_SYMBOLRESOLVERVISITOR_H
