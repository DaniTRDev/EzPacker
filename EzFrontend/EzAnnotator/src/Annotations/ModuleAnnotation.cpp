#include "Annotations/ModuleAnnotation.h"

ModuleAnnotation::ModuleAnnotation(size_t symbolId, size_t typeId)
{
    setTypeId(typeId);
    setSymbolId(symbolId);
}

const char *ModuleAnnotation::getAnnotationName() { return "Module"; }

size_t ModuleAnnotation::getArgumentSymbolId(size_t argId) const
{
    if (argId >= m_argumentSymbolIds.size())
        return 0;

    return m_argumentSymbolIds[argId];
}

void ModuleAnnotation::pushArgument(size_t symbolId) { m_argumentSymbolIds.push_back(symbolId); }

const std::vector<size_t> &ModuleAnnotation::getArgumentSymbolIds() const { return m_argumentSymbolIds; }
