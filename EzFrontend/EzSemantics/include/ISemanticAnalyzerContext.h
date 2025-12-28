#ifndef EZPACKER_ISEMANTICANALYZERCONTEXT_H
#define EZPACKER_ISEMANTICANALYZERCONTEXT_H

#include "EzSemanticsCommon.h"
#include "ScopeManager/ScopeManager.h"

/**
 * Interface that modelates what functionality should the context of the semantic analyzer provide upper services
 * (semantic ast node visitors).
 */
class ISemanticAnalyzerContext
{
  public:
    virtual ~ISemanticAnalyzerContext() = default;

    /**
     * Returns the error collector linked to this semantic analyzer context.
     * @return const std::shared_ptr<ErrorCollector> &
     */
    virtual const std::shared_ptr<ErrorCollector> &getErrorCollector() const = 0;

    /**
     * Returns scopes defined in this context.
     * @return const std::map<size_t std::shared_ptr<Scope>> &
     */
    virtual const std::shared_ptr<ScopeManager> &getScopeManager() const = 0;
};

#endif // EZPACKER_ISEMANTICANALYZERCONTEXT_H
