#include "SourceManager/SourceManager.h"

SourceManager::SourceManager(const std::filesystem::path &workingPath, std::pmr::memory_resource *alloc) :
    m_workingPath(workingPath), m_alloc(alloc), m_pathToIdMap(m_alloc), m_sourceFiles(m_alloc)
{
    // Slot 0 reserved as a nullptr sentinel so 1-based IDs match indexing
    m_sourceFiles.push_back(nullptr);
}

bool SourceManager::doesSourceNameExist(const std::string_view &sourceName) const
{
    return m_pathToIdMap.find(sourceName) != m_pathToIdMap.end();
}

std::filesystem::path SourceManager::resolveSourcePath(const std::filesystem::path &sourceFile) const
{
    if (sourceFile.is_absolute())
    {
        return std::filesystem::weakly_canonical(sourceFile);
    }
    return std::filesystem::weakly_canonical(m_workingPath / sourceFile);
}

size_t SourceManager::addSourceContent(const std::string &name, const std::string_view &content)
{
    if (doesSourceNameExist(name))
    {
        return 0;
    }

    size_t newId = m_sourceFiles.size();

    // Allocate SourceFileEntry using the PMR memory resource
    void *entryMem = m_alloc->allocate(sizeof(SourceFileEntry), alignof(SourceFileEntry));
    SourceFileEntry *entry = new (entryMem) SourceFileEntry{ std::pmr::string(content, m_alloc),
                                                             std::pmr::string(name, m_alloc),
                                                             std::pmr::vector<SourceLineRange>(m_alloc) };

    // Precompute line bounds with 1-based line numbers (safely handles \n and \r\n)
    size_t lineStart = 0;
    size_t lineNumber = 1;

    for (size_t i = 0; i < content.size(); ++i)
    {
        if (content[i] == '\n')
        {
            entry->m_lines.push_back({ lineStart, i, lineNumber++ });
            lineStart = i + 1;
        }
    }
    if (lineStart <= content.size())
    {
        entry->m_lines.push_back({ lineStart, content.size(), lineNumber });
    }

    // Use the arena-backed string to ensure it outlives the map entry
    m_pathToIdMap.emplace(entry->m_name, newId);
    m_sourceFiles.push_back(entry);

    return newId;
}

SourceLineRange *SourceManager::getReferenceLine(SourceReference *ref) const
{
    if (!ref || ref->m_sourceFileId == 0 || ref->m_sourceFileId >= m_sourceFiles.size())
    {
        return nullptr;
    }

    SourceFileEntry *entry = m_sourceFiles[ref->m_sourceFileId];
    if (!entry || entry->m_lines.empty())
    {
        return nullptr;
    }

    // Binary search for the line range containing ref->m_beginOffset
    auto it = std::upper_bound(entry->m_lines.begin(),
                               entry->m_lines.end(),
                               ref->m_beginOffset,
                               [](size_t val, const SourceLineRange &range) { return val < range.m_beginOffset; });

    if (it != entry->m_lines.begin())
    {
        --it;
        // Verify the offset falls within this line range
        if (ref->m_beginOffset >= it->m_beginOffset && ref->m_beginOffset <= it->m_endOffset)
        {
            return &(*it);
        }
    }

    return nullptr;
}

SourceReference *SourceManager::createReference(size_t startOffset, size_t length, size_t sourceId)
{
    if (sourceId == 0 || sourceId >= m_sourceFiles.size())
    {
        return nullptr;
    }

    SourceFileEntry *entry = m_sourceFiles[sourceId];
    if (!entry)
    {
        return nullptr;
    }

    size_t fileLength = entry->m_content.size();
    if (startOffset > fileLength)
    {
        return nullptr;
    }

    size_t endOffset = std::min(startOffset + length, fileLength);

    void *mem = m_alloc->allocate(sizeof(SourceReference), alignof(SourceReference));
    return new (mem) SourceReference{ startOffset, endOffset, sourceId };
}

SourceReference *SourceManager::createReference(size_t startOffset, size_t length, const std::string_view &sourceFile)
{
    auto it = m_pathToIdMap.find(sourceFile);
    if (it == m_pathToIdMap.end())
    {
        return nullptr;
    }
    return createReference(startOffset, length, it->second);
}

std::string_view SourceManager::getRawLineContent(SourceReference *ref) const
{
    SourceLineRange *lineRange = getReferenceLine(ref);
    if (!lineRange)
    {
        return {};
    }

    const SourceFileEntry *entry = m_sourceFiles[ref->m_sourceFileId];
    return std::string_view(entry->m_content.data() + lineRange->m_beginOffset, lineRange->length());
}

std::string_view SourceManager::getReferenceContent(SourceReference *ref) const
{
    if (!ref || ref->m_sourceFileId == 0 || ref->m_sourceFileId >= m_sourceFiles.size())
    {
        return {};
    }

    const SourceFileEntry *entry = m_sourceFiles[ref->m_sourceFileId];
    if (!entry)
    {
        return {};
    }

    const auto &content = entry->m_content;
    if (ref->m_beginOffset > content.size() || ref->m_endOffset > content.size() ||
        ref->m_beginOffset > ref->m_endOffset)
    {
        return {};
    }

    return std::string_view(content.data() + ref->m_beginOffset, ref->length());
}

std::string_view SourceManager::getSourceContent(size_t id) const
{
    if (id == 0 || id >= m_sourceFiles.size() || !m_sourceFiles[id])
    {
        return {};
    }
    return m_sourceFiles[id]->m_content;
}

std::string_view SourceManager::getSourceName(size_t id) const
{
    if (id == 0 || id >= m_sourceFiles.size() || !m_sourceFiles[id])
    {
        return {};
    }
    return m_sourceFiles[id]->m_name;
}