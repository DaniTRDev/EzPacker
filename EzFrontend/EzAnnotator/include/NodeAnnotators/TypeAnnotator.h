#ifndef EZPACKER_TYPEANNOTATOR_H
#define EZPACKER_TYPEANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "INodeAnnotator.h"
#include "ScopedTable/ScopedType.h"
#include "Annotations/TypeAnnotation.h"
#include "NodeAnnotators/ScopeAbleAnnotator.h"

/**
 * This annotator takes a type and checks if there's a real type linked to it. After annotating, types's content
 * is cleared (flattening).
 *
 * In many cases, this class acts as a helper for upper annotators. They use this class to get a type ID, and then
 * they discard everything related to it (node, the annotation itself, ...) while they keep the ID.
 */
class TypeAnnotator : public ScopeAbleAnnotator
{
  public:
    TypeAnnotator(const std::shared_ptr<ScopedTable<ScopedType>> &typeTable);

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
     * Returns true if node and logger are valid; and if given node is a type node.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;
};

#endif // EZPACKER_TYPEANNOTATOR_H
