#include "SourceManager/SourceManager.h"
#include <algorithm>
#include <fstream>

static void populateLineRanges(SourceFileEntry *entry)
{
    const auto &content = entry->m_content;
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
}

SourceManager::SourceManager(const std::filesystem::path &workingPath, std::pmr::memory_resource *alloc) :
    m_workingPath(workingPath), m_alloc(alloc), m_includePaths(m_alloc), m_pathToIdMap(m_alloc), m_sourceFiles(m_alloc)
{
    // Slot 0 reserved as a nullptr sentinel so 1-based IDs match indexing
    m_sourceFiles.push_back(nullptr);
}

SourceManager::~SourceManager()
{
    for (SourceFileEntry *entry : m_sourceFiles)
    {
        if (entry != nullptr)
        {
            entry->~SourceFileEntry();
            m_alloc->deallocate(entry, sizeof(SourceFileEntry), alignof(SourceFileEntry));
        }
    }
}

bool SourceManager::doesSourceNameExist(const std::string_view &sourceName) const
{
    return m_pathToIdMap.find(sourceName) != m_pathToIdMap.end();
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

    populateLineRanges(entry);

    // Use the arena-backed string to ensure it outlives the map entry
    m_pathToIdMap.emplace(entry->m_name, newId);
    m_sourceFiles.push_back(entry);

    return newId;
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

void SourceManager::addIncludePath(const std::filesystem::path &path)
{
    std::error_code ec;
    if (std::filesystem::exists(path, ec))
    {
        m_includePaths.push_back(std::filesystem::weakly_canonical(path, ec));
    }
    else
    {
        m_includePaths.push_back(path);
    }
}

std::filesystem::path SourceManager::resolveSourcePath(const std::filesystem::path &sourceFile,
                                                       const std::optional<std::filesystem::path> &relativeTo) const
{
    std::error_code ec;

    if (sourceFile.is_absolute() && std::filesystem::exists(sourceFile, ec))
    {
        return std::filesystem::weakly_canonical(sourceFile, ec);
    }

    // Relative to the including source file's directory
    if (relativeTo.has_value())
    {
        auto candidate = *relativeTo / sourceFile;
        if (std::filesystem::exists(candidate, ec))
        {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }

    // Relative to working directory
    auto workingCandidate = m_workingPath / sourceFile;
    if (std::filesystem::exists(workingCandidate, ec))
    {
        return std::filesystem::weakly_canonical(workingCandidate, ec);
    }

    // Search in registered include search paths
    for (const auto &incPath : m_includePaths)
    {
        auto candidate = incPath / sourceFile;
        if (std::filesystem::exists(candidate, ec))
        {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }

    // Fallback: Return weakly canonical path relative to working directory or as-is
    if (sourceFile.is_absolute())
    {
        return std::filesystem::weakly_canonical(sourceFile, ec);
    }
    return std::filesystem::weakly_canonical(m_workingPath / sourceFile, ec);
}

std::optional<size_t> SourceManager::loadFile(const std::filesystem::path &filePath,
                                              const std::optional<std::filesystem::path> &relativeTo)
{
    std::filesystem::path resolvedPath = resolveSourcePath(filePath, relativeTo);
    std::string canonicalName = resolvedPath.string();

    // Avoid loading duplicate entries
    auto it = m_pathToIdMap.find(canonicalName);
    if (it != m_pathToIdMap.end())
    {
        return it->second;
    }

    std::ifstream file(resolvedPath, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return std::nullopt;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    size_t newId = m_sourceFiles.size();

    // Direct allocation of SourceFileEntry and its PMR string buffer without heap intermediates
    void *entryMem = m_alloc->allocate(sizeof(SourceFileEntry), alignof(SourceFileEntry));
    SourceFileEntry *entry = new (entryMem) SourceFileEntry{ std::pmr::string(fileSize, '\0', m_alloc),
                                                             std::pmr::string(canonicalName, m_alloc),
                                                             std::pmr::vector<SourceLineRange>(m_alloc) };

    if (fileSize > 0)
    {
        file.read(entry->m_content.data(), static_cast<std::streamsize>(fileSize));
        if (!file)
        {
            entry->~SourceFileEntry();
            m_alloc->deallocate(entryMem, sizeof(SourceFileEntry), alignof(SourceFileEntry));
            return std::nullopt;
        }
    }

    populateLineRanges(entry);

    m_pathToIdMap.emplace(entry->m_name, newId);
    m_sourceFiles.push_back(entry);

    return newId;
}

const std::pmr::string *SourceManager::getSourceBuffer(size_t id) const
{
    if (id == 0 || id >= m_sourceFiles.size() || !m_sourceFiles[id])
    {
        return nullptr;
    }
    return &m_sourceFiles[id]->m_content;
}

const std::pmr::vector<std::filesystem::path> &SourceManager::getIncludePaths() const { return m_includePaths; }

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