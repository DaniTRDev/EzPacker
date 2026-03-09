#ifndef EZPACKER_INCLUDEVISITOR_H
#define EZPACKER_INCLUDEVISITOR_H

#include "EzFrontendCompilerCommon.h"

/**
 * This resolves includes in the top-level scope.
 */
class IncludeVisitor : public AstNodeVisitor
{
  public:
    /**
     * Visits the given IncludeAstNode. This will resolve the include and add the included file to compilation process.
     * @param include
     * @return
     */
    bool visit(IncludeAstNode *include) override;

    /**
     * Returns the discovered inclusions set.
     * @param dest
     * @param const std::set<std::string_view> &
     */
    const std::set<std::string_view> &getInclusions(std::set<std::string_view> &dest);

  private:
    std::set<std::string_view> m_discoveredInclusions; // Set of all the included files.
};

#endif // EZPACKER_INCLUDEVISITOR_H
