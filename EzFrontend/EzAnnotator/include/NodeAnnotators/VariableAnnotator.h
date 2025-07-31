#ifndef EZPACKER_VARIABLEANNOTATOR_H
#define EZPACKER_VARIABLEANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "Annotations/VariableAnnotation.h"
#include "ScopeAbleAnnotator.h"
#include "SymbolAnnotator.h"
#include "TypeAnnotator.h"
#include "ConstantAnnotator.h"

/**
 *
 * This annotator is also quite complex, as ModuleAnnotator or InstructionAnnotator. It resolves variable's type
 * and symbol and then it annotates its initializers. Initializers are preserved as children of the given variable node,
 * but they are annotated (constant annotator) on their own.
 *
 * After annotating, type and name child nodes are removed (flattening).
 */
class VariableAnnotator : public ScopeAbleAnnotator
{
  public:
    /**
     * Creates the annotator with the given symbol and type table.
     * @param symbolTable
     * @param typeTable
     */
    VariableAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
                      const ScopeAbleAnnotator::TypeTableT &typeTable);

    /**
     * Annotates information to the given node. Returns true if succeeded. If it failed it will push errors to
     * the given logger and will return false.
     *
     * It assumes canAnnotate was called previously and it returned true.
     *
     * @param node
     * @param logger
     * @return bool
     */
    bool annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger) override;

    /**
     * Returns true if this node and logger are valid; and if given node is a variable node.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;
};

#endif // EZPACKER_VARIABLEANNOTATOR_H
