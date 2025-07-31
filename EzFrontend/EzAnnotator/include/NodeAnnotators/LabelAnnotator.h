#ifndef EZPACKER_LABELANNOTATOR_H
#define EZPACKER_LABELANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "ScopeAbleAnnotator.h"
#include "SymbolAnnotator.h"
#include "InstructionAnnotator.h"
#include "Annotations/LabelAnnotation.h"

/**
 * This annotator resolves a label's symbol within the current module scope and annotates child instructions. In the
 * process of resolving label's symbol, the identifier node is removed.
 */
class LabelAnnotator : public ScopeAbleAnnotator
{
  public:
    /**
     * Creates the annotator with the given symbol and type tables.
     * @param symbolTable
     * @param typeTable
     */
    LabelAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
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
     * Returns true if node and logger are valid; and if given node is a label node.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;
};

#endif // EZPACKER_LABELANNOTATOR_H
