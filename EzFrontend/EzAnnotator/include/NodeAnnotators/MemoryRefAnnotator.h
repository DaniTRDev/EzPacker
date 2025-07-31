#ifndef EZPACKER_MEMORYREFANNOTATOR_H
#define EZPACKER_MEMORYREFANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "INodeAnnotator.h"
#include "ScopedTable/ScopedSymbol.h"
#include "ScopedTable/ScopedType.h"
#include "Annotations/MemoryRefAnnotation.h"
#include "NodeAnnotators/VirtualVariableAnnotator.h"
#include "NodeAnnotators/ConstantAnnotator.h"
#include "NodeAnnotators/ScopeAbleAnnotator.h"

/**
 * This annotator is responsible of:
 *  - Resolving any of the memory reference nodes
 *  - Resolve memory reference type
 *  - Resolve inner reference's virtual variables or constants.
 *
 *  After annotation is done, child nodes are removed (flattening).
 */
class MemoryRefAnnotator : public ScopeAbleAnnotator
{
  public:
    /**
     * Creates the annotator with the given symbol table.
     * @param symbolTable
     */
    MemoryRefAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable);

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
     * Returns true if node and logger are valid; and if the given node is any of the memory nodes (check MemoryRef
     * parser).
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;

    /**
     * Makes the annotator set the type of the referenced memory. It would be equivalent as defining type's pointer in
     * C/C++.
     * @param typeId
     */
    void setMemoryType(size_t typeId);

  private:
    size_t m_refTypeId; // Type of the referenced memory region.
};

#endif // EZPACKER_MEMORYREFANNOTATOR_H
