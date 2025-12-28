#ifndef EZPACKER_SEMANTICVISITOR_H
#define EZPACKER_SEMANTICVISITOR_H

#include "EzSemanticsCommon.h"

/**
 * This interface adds the ability of modifying semantic context to visitors.
 */
class SemanticVisitor : public IAstNodeVisitor
{
  public:
    /**
     * Creates the visitor with the given semantic context.
     * @param ctx
     */
    SemanticVisitor(std::shared_ptr<class ISemanticAnalyzerContext> ctx);
    
  protected:
    std::shared_ptr<class ISemanticAnalyzerContext> m_ctx;
};

#endif // EZPACKER_SEMANTICVISITOR_H
