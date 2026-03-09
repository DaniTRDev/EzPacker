#include "IncludeVisitor/IncludeVisitor.h"
#include "BasicSemanticContext.h"

bool IncludeVisitor::visit(IncludeAstNode *include)
{
    const std::string_view &includePath = include->getIncludedFileRelPath();
    if (includePath.empty())
    {
        return false;
    }

    if (m_discoveredInclusions.contains(includePath))
    {
        return true;
    }

    m_discoveredInclusions.insert(includePath);
    return true;
}
const std::set<std::string_view> &IncludeVisitor::getInclusions(std::set<std::string_view> &dest)
{
    return m_discoveredInclusions;
}
