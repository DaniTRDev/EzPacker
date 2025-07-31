#ifndef EZPACKER_CONSTANTANNOTATOR_H
#define EZPACKER_CONSTANTANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "INodeAnnotator.h"
#include "Annotations/ConstantAnnotation.h"

/**
 * Annotates a node that contains a constant value, this includes:
 *  - String nodes
 *  - Integer nodes
 *  - Float nodes
 *
 *  It appends a ConstantAnnotation to the given node. It removes node's content (flattening).
 */
class ConstantAnnotator : public INodeAnnotator
{
  public:
    /**
     * Annotates information to the constant node. Returns true if succeeded. If it failed it will show errors with
     * the given logger and will return false.
     * @param node
     * @param logger
     * @return bool
     */
    bool annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger) override;

    /**
     * Returns true if node and logger are valid; and if given node is either an int node, a float node or a string
     * node.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;

    /**
     * Sets the type used in the compiled file for this constant.
     * @param id
     */
    void setTypeId(size_t id);

  private:
    size_t m_typeId;
};

#endif // EZPACKER_CONSTANTANNOTATOR_H
