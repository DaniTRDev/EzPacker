#ifndef EZPACKER_SEMANTICVISITOR_H
#define EZPACKER_SEMANTICVISITOR_H

#include "EzSemanticsCommon.h"

/**
 * This interface adds the ability of modifying semantic context to visitors.
 */
class SemanticVisitor : public AstNodeVisitor
{
  public:
    /**
     * Sets the semantic context.
     * @param ctx
     */
    void setSemanticContext(const std::shared_ptr<class BasicSemanticContext> &ctx);

    /**
     * Returns the semantic context of this object.
     * @return const std::shared_ptr<class BasicSemanticContext> &
     */
    const std::shared_ptr<class BasicSemanticContext> &getSemanticContext() const;
  protected:
    std::shared_ptr<class BasicSemanticContext> m_ctx;
};

#endif // EZPACKER_SEMANTICVISITOR_H
