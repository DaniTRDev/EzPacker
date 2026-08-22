#include "Diagnostics/DiagnosticCollector.h"
#include "Parser/ParseContext.h"
#include "SourceManager/GenericSourceManager.h"

ParseContext::ParseContext(class DiagnosticCollector *diagCollector,
                           class GenericSourceManager *sourceManager,
                           size_t sourceId) :
    m_diagCollector(diagCollector), m_sourceManager(sourceManager), m_sourceId(sourceId), m_handler({ .ctx = *this })
{
}

class GenericSourceManager *ParseContext::getSourceManager() const { return m_sourceManager; }

class DiagnosticCollector *ParseContext::getDiagCollector() const { return m_diagCollector; }

size_t ParseContext::getSourceId() const { return m_sourceId; }

SourceReference *ParseContext::createRef(const char *startIter, const char *endIter)
{
    const char *basePtr = m_sourceManager->getSourceContent(m_sourceId).data();

    const size_t startOffset = static_cast<size_t>(startIter - basePtr);
    const size_t length = static_cast<size_t>(endIter - startIter);

    return m_sourceManager->createReference(startOffset, length, m_sourceId);
}

void ParseContext::pushToCollector(std::string_view sourceName,
                                   std::string_view message,
                                   struct SourceReference *sourceRef)
{
    m_diagCollector->error(sourceName, message) << sourceRef;
}