#ifndef EZPACKER_INCLUDEPHASE_H
#define EZPACKER_INCLUDEPHASE_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"
#include "IncludeVisitor/IncludeVisitor.h"

/**
 * Scans the just-parsed AST for include directives and adds the included files to the compilation unit's list of files
 * to compile.
 */
class IncludePhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * Performs the discovery of includes.
     * @param unit
     * @return bool
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * Returns "AstLoweringPhase"
     * @return const char *
     */
    const char *getName() override;

    /**
     * Moves the included files from the destination set.
     * @param dest
     */
    void moveIncludedFilesToDest(std::set<std::string_view> &dest);

  private:
    std::set<std::string_view> m_includedFiles;
};

#endif // EZPACKER_INCLUDEPHASE_H
