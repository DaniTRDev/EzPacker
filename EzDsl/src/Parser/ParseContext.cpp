#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "SourceManager/GenericSourceManager.h"

ParseContext::ParseContext(class DiagnosticCollector *diagCollector,
                           class GenericSourceManager *sourceManager,
                           size_t sourceId,
                           std::pmr::memory_resource *alloc) :
    m_diagCollector(diagCollector), m_sourceManager(sourceManager), m_sourceId(sourceId), m_handler({ .ctx = *this }),
    m_alloc(alloc)
{
}

class GenericSourceManager *ParseContext::getSourceManager() const { return m_sourceManager; }

class DiagnosticCollector *ParseContext::getDiagCollector() const { return m_diagCollector; }

size_t ParseContext::getSourceId() const { return m_sourceId; }

SourceReference *ParseContext::createRef(const char *startIter, const char *endIter)
{
    // Compute character offset relative to the start of the source buffer
    const char *basePtr = m_sourceManager->getSourceContent(m_sourceId).data();

    const size_t startOffset = static_cast<size_t>(startIter - basePtr);
    const size_t length = static_cast<size_t>(endIter - startIter);

    return m_sourceManager->createReference(startOffset, length, m_sourceId);
}

std::pmr::memory_resource *ParseContext::getAllocator() const { return m_alloc; }

void ParseContext::pushToCollector(std::string_view sourceName,
                                   std::string_view message,
                                   struct SourceReference *sourceRef)
{
    // Format error diagnostic and attach source location span
    m_diagCollector->error(sourceName, message) << sourceRef;
}