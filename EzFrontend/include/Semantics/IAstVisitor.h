#ifndef EZPACKER_IASTVISITOR_H
#define EZPACKER_IASTVISITOR_H

#include "EzFrontendCommon.h"
#include "Parser/Ast/Ast.h"

/**
 * Interface that defines a visitor that will walk the AST produced by the parser.
 */
template <typename VisitorResult> class IAstVisitor
{
  public:
    virtual ~IAstVisitor() = default;
    
    /**
     * Visits this node and its children.
     * @param tree
     * @return VisitorResult
     */
    virtual const VisitorResult &visit(std::shared_ptr<Ast> tree) = 0;
};

#endif // EZPACKER_IASTVISITOR_H
