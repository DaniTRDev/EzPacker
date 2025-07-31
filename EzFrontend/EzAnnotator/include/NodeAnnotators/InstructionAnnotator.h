#ifndef EZPACKER_INSTRUCTIONANNOTATOR_H
#define EZPACKER_INSTRUCTIONANNOTATOR_H

#include "INodeAnnotator.h"
#include "InstructionTable.h"
#include "Annotations/InstructionAnnotation.h"
#include "NodeAnnotators/MemoryRefAnnotator.h"
#include "NodeAnnotators/ConstantAnnotator.h"
#include "NodeAnnotators/VirtualVariableAnnotator.h"
#include "NodeAnnotators/TypeAnnotator.h"
#include "NodeAnnotators/ScopeAbleAnnotator.h"

/**
 * Class that will annotate instruction nodes. It's responsible of:
 *  - Resolving instruction ID.
 *  - Resolving instruction operands (virtual variables, memory references or constant values).
 *
 * After annotation, the given instruction node is flattened and only instruction operand nodes are preserved, removing
 * any node below them.
 */
class InstructionAnnotator : public ScopeAbleAnnotator
{
  public:
    /**
     * Creates the annotator with the given symbol and type table.
     * @param symbolTable
     * @param typeTable
     */
    InstructionAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
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
     * Returns true if node and logger are valid; and if given node is an instruction node.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;
};

#endif // EZPACKER_INSTRUCTIONANNOTATOR_H
