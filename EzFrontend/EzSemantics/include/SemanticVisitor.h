/**
 * @file SemanticVisitor.h
 * @brief Common visitor base for all semantic-analysis and lowering passes.
 *
 * `SemanticVisitor` extends `AstNodeVisitor` with a shared
 * `BasicSemanticContext`. It does not implement any semantic logic by itself;
 * it simply standardises how passes receive and expose the context that owns
 * scopes, symbol pools, annotations and diagnostics.
 *
 * All public EzSemantics passes derive from this type, so code that builds a
 * pipeline can configure them uniformly before traversing the AST.
 */
#ifndef EZPACKER_SEMANTICVISITOR_H
#define EZPACKER_SEMANTICVISITOR_H

#include "EzSemanticsCommon.h"

/**
 * Base class for visitors that operate with a shared `BasicSemanticContext`.
 */
class SemanticVisitor : public AstNodeVisitor
{
  public:
    /**
     * Attaches the semantic context that this visitor should use while
     * traversing the AST.
     *
     * The visitor stores the shared pointer but does not create or clone the
     * context.
     */
    void setSemanticContext(const std::shared_ptr<class BasicSemanticContext> &ctx);

    /**
     * Returns the semantic context currently associated with this visitor.
     */
    const std::shared_ptr<class BasicSemanticContext> &getSemanticContext() const;
  protected:
    std::shared_ptr<class BasicSemanticContext> m_ctx;
};

#endif // EZPACKER_SEMANTICVISITOR_H
