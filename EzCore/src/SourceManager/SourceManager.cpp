#include "SourceManager/SourceManager.h"
#include <algorithm>
#include <fstream>

// Scans content buffer for newline boundaries and constructs 1-based SourceLineRange records
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
    // Record the final line only when the buffer does not already end with a newline, so a
    // trailing '\n' does not create a phantom empty line at EOF.
    if (content.empty() || content.back() != '\n')
    {
        entry->m_lines.push_back({ lineStart, content.size(), lineNumber });
    }
}

/**
 * Initializes the working directory and arena-backed containers, reserving slot 0 as a null
 * sentinel so source IDs are 1-based.
 */
SourceManager::SourceManager(const std::filesystem::path &workingPath, std::pmr::memory_resource *alloc) :
    m_workingPath(workingPath), m_alloc(alloc), m_includePaths(m_alloc), m_pathToIdMap(m_alloc), m_sourceFiles(m_alloc)
{
    // Slot 0 reserved as a nullptr sentinel so 1-based IDs match indexing
    m_sourceFiles.push_back(nullptr);
}

/**
 * Explicitly runs each SourceFileEntry destructor and returns its arena memory to the resource.
 */
SourceManager::~SourceManager()
{
    // Explicitly destroy and deallocate each arena-allocated SourceFileEntry
    for (SourceFileEntry *entry : m_sourceFiles)
    {
        if (entry != nullptr)
        {
            entry->~SourceFileEntry();
            m_alloc->deallocate(entry, sizeof(SourceFileEntry), alignof(SourceFileEntry));
        }
    }
}

/**
 * Returns true when the name or canonical path is already present in the registry.
 */
bool SourceManager::doesSourceNameExist(const std::string_view &sourceName) const
{
    return m_pathToIdMap.find(sourceName) != m_pathToIdMap.end();
}

/**
 * Registers in-memory content under name, precomputes its line table and returns the new 1-based
 * ID, or 0 if the name is already registered. The entry and its strings are allocated in the arena.
 */
size_t SourceManager::addSourceContent(const std::string &name, const std::string_view &content)
{
    if (doesSourceNameExist(name))
    {
        return 0;
    }

    SourceFileEntry *entry = createEntry(std::pmr::string(content, m_alloc), std::pmr::string(name, m_alloc));
    return registerEntry(entry);
}

/**
 * Allocates an arena-backed SourceFileEntry from the given content/name, precomputes its line
 * table, and returns the entry without registering it.
 */
SourceFileEntry *SourceManager::createEntry(std::pmr::string content, std::pmr::string name)
{
    void *entryMem = m_alloc->allocate(sizeof(SourceFileEntry), alignof(SourceFileEntry));
    SourceFileEntry *entry = new (entryMem)
            SourceFileEntry{ std::move(content), std::move(name), std::pmr::vector<SourceLineRange>(m_alloc) };

    populateLineRanges(entry);
    return entry;
}

/**
 * Assigns entry the next 1-based ID, registers it in the path map and source list, and returns
 * that ID. The map key points at the entry's arena-backed name so it outlives the map.
 */
size_t SourceManager::registerEntry(SourceFileEntry *entry)
{
    size_t newId = m_sourceFiles.size();
    m_pathToIdMap.emplace(entry->m_name, newId);
    m_sourceFiles.push_back(entry);
    return newId;
}

/**
 * Allocates a SourceReference for [startOffset, startOffset+length) in sourceId, clamping the end
 * to the file length. Returns nullptr for invalid IDs, entries or start offsets.
 */
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

/**
 * Resolves the file by name and delegates to the ID-based createReference; returns nullptr when
 * the name is unknown.
 */
SourceReference *SourceManager::createReference(size_t startOffset, size_t length, const std::string_view &sourceFile)
{
    auto it = m_pathToIdMap.find(sourceFile);
    if (it == m_pathToIdMap.end())
    {
        return nullptr;
    }
    return createReference(startOffset, length, it->second);
}

/**
 * Locates the precomputed line containing the reference's begin offset via upper_bound, then
 * verifies the offset lies within the preceding range. Returns nullptr when no range matches.
 * The returned pointer references the entry's line table and is read-only.
 */
const SourceLineRange *SourceManager::getReferenceLine(SourceReference *ref) const
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

/**
 * Registers an include directory, canonicalizing it when it exists and storing it unchanged
 * otherwise.
 */
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

/**
 * Resolves a source path by trying, in order: an existing absolute path, the directory of the
 * including file, the working directory, then each registered include path. Falls back to a
 * working-directory-relative canonical path when nothing exists.
 */
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

/**
 * Resolves and reads filePath into an arena-backed entry, reusing the existing ID for an
 * already-loaded canonical path. Returns the new ID, or std::nullopt when the file cannot be
 * opened or read fully.
 */
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

    // A failed end-seek leaves tellg() at -1; casting that to size_t would request a huge allocation.
    file.seekg(0, std::ios::end);
    std::streampos endPos = file.tellg();
    if (endPos == std::streampos(-1))
    {
        return std::nullopt;
    }
    size_t fileSize = static_cast<size_t>(endPos);
    file.seekg(0, std::ios::beg);

    // Read into an arena string first so a short read can bail out before any entry is registered.
    std::pmr::string content(fileSize, '\0', m_alloc);
    if (fileSize > 0)
    {
        file.read(content.data(), static_cast<std::streamsize>(fileSize));
        if (!file)
        {
            return std::nullopt;
        }
    }

    SourceFileEntry *entry = createEntry(std::move(content), std::pmr::string(canonicalName, m_alloc));
    return registerEntry(entry);
}

/**
 * Returns a view of the full line containing the reference (terminators excluded), or an empty
 * view when the reference line cannot be resolved.
 */
std::string_view SourceManager::getRawLineContent(SourceReference *ref) const
{
    const SourceLineRange *lineRange = getReferenceLine(ref);
    if (!lineRange)
    {
        return {};
    }

    const SourceFileEntry *entry = m_sourceFiles[ref->m_sourceFileId];
    return std::string_view(entry->m_content.data() + lineRange->m_beginOffset, lineRange->length());
}

/**
 * Returns a view of the exact referenced byte span, or an empty view when the reference is
 * invalid or its offsets fall outside the file content.
 */
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

/**
 * Returns the full content of the source with the given ID, or an empty view if invalid.
 */
std::string_view SourceManager::getSourceContent(size_t id) const
{
    if (id == 0 || id >= m_sourceFiles.size() || !m_sourceFiles[id])
    {
        return {};
    }
    return m_sourceFiles[id]->m_content;
}

/**
 * Returns the registered display name of the source with the given ID, or an empty view if invalid.
 */
std::string_view SourceManager::getSourceName(size_t id) const
{
    if (id == 0 || id >= m_sourceFiles.size() || !m_sourceFiles[id])
    {
        return {};
    }
    return m_sourceFiles[id]->m_name;
}