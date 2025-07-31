#ifndef EZPACKER_INODEANNOTATOR_H
#define EZPACKER_INODEANNOTATOR_H

#include "EzAnnotatorCommon.h"

/**
 * This interface models the behaviour of an annotator. There's a method that checks if the given node can be annotated,
 * and there's a method that appends the annotation (skipping if it's suitable for annotation, as it was done in the
 * previous function).
 *
 * TODO: Append logger to canAnnotate and make it a real preCondition checker module.
 */
class INodeAnnotator
{
  public:
    virtual ~INodeAnnotator() = default;

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
    virtual bool annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger) = 0;

    /**
     * Returns true if this annotator can append an annotation to this node.
     * @param node
     * @return bool
     */
    virtual bool canAnnotate(const std::shared_ptr<AstNode> &node) = 0;
};

#endif // EZPACKER_INODEANNOTATOR_H
