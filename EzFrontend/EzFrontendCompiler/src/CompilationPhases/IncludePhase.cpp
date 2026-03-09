#include "CompilationPhases/IncludePhase.h"

bool IncludePhase::execute(struct FrontendCompilationUnit *unit)
{
    IncludeVisitor visitor;
    for (AstNode *node : *unit->getGlobalScopeAstNodes())
    {
        node->accept(&visitor);
    }

    for (auto &inclusion : visitor.getInclusions(m_includedFiles))
    {
        if (!m_includedFiles.contains(inclusion))
        {
            m_includedFiles.insert(inclusion);
        }
    }

    return true;
}

const char *IncludePhase::getName() { return "IncludePhase"; }

void IncludePhase::moveIncludedFilesToDest(std::set<std::string_view> &dest) { dest = std::move(m_includedFiles); }