#include "SourceManager/SourceManager.h"

SourceManager::SourceManager() {}

SourceManager::~SourceManager() { m_sources.clear(); }

bool SourceManager::addSourceContent(const std::string &name, const std::string &content)
{
    if (m_sources.contains(name))
        return false;

    size_t numLines = std::count(content.begin(), content.end(), '\n');

    m_sources[name] = content;
    m_sourceLines[name] = std::vector<LineSourceRange>(numLines);

    std::vector<LineSourceRange> &currentSourceLines = m_sourceLines[name];

    size_t lineStart = 0, lineEnd = content.find('\n', lineStart);

    size_t id = 0;
    while (lineEnd != std::string::npos)
    {
        currentSourceLines[id] = LineSourceRange{ .m_end = lineEnd, .m_start = lineStart };

        id++;
        lineStart = lineEnd + 1;
        lineEnd = content.find('\n', lineStart);
    }

    // Handle the last line (or the whole content if no newlines were found)
    if (lineStart < content.size())
    {
        currentSourceLines.push_back({ .m_end = content.size() - lineStart, .m_start = lineStart });
    }

    return true;
}

std::shared_ptr<SourceReference>
SourceManager::createReference(size_t col, size_t length, size_t line, const std::string &sourceFile)
{
    auto it = m_sourceLines.find(sourceFile);
    if (it == m_sourceLines.end())
    {
        return nullptr; // Given file is not in the source list.
    }

    if (line >= it->second.size())
    {
        return nullptr; // Invalid line of source file.
    }

    if ((col + length) > it->second[line].m_end)
    {
        return nullptr; // Reference is out of bounds of the line.
    }

    std::shared_ptr<SourceReference> ref = std::make_shared<SourceReference>();
    ref->m_col = col;
    ref->m_length = length;
    ref->m_line = line;
    ref->m_sourceFile = sourceFile;

    return ref;
}

std::string SourceManager::getReferenceContent(const std::shared_ptr<SourceReference> &ref)
{
    std::string result;

    auto it = m_sourceLines.find(ref->m_sourceFile);
    if (it != m_sourceLines.end() && ref->m_line < it->second.size())
    {
        auto sourceContent = m_sources[ref->m_sourceFile];
        auto sourceContentData = sourceContent.data();

        result.resize(ref->m_length);
        std::copy_n(&sourceContentData[ref->m_col], ref->m_length, result.data());
    }

    return result;
}

std::shared_ptr<SourceReference>
SourceManager::mergeReferences(const std::vector<std::shared_ptr<SourceReference>> &refs)
{
    if (refs.empty())
        return nullptr;

    uint64_t minCol = UINT64_MAX;
    uint64_t maxCol = 0;

    for (auto &ref : refs)
    {
        minCol = std::min(minCol, ref->m_col);
        maxCol = std::max(maxCol, ref->m_col + ref->m_length);
    }

    return std::make_shared<SourceReference>(SourceReference{ .m_col = minCol,
                                                              .m_length = maxCol - minCol,
                                                              .m_line = refs[0]->m_line,
                                                              .m_sourceFile = refs[0]->m_sourceFile });
    ;
}

void MultiSourceReferenceCreator::attach(std::shared_ptr<SourceManager> sourceManager)
{
    if (!sourceManager)
        throw std::runtime_error("Invalid source manager given to MultiSourceReferenceCreator");

    m_sourceManager = std::move(sourceManager);
}

void MultiSourceReferenceCreator::push(std::shared_ptr<SourceReference> reference)
{
    if (!reference)
        throw std::runtime_error("Invalid source reference given to MultiSourceReferenceCreator");

    m_references.push_back(std::move(reference));
}

std::shared_ptr<SourceReference> MultiSourceReferenceCreator::merge()
{
    if (!m_sourceManager)
        throw std::runtime_error("Invalid source manager given to MultiSourceReferenceCreator when merging");

    if (m_references.empty())
        throw std::runtime_error("Invalid source references given to MultiSourceReferenceCreator when merging");

    if (std::shared_ptr<SourceReference> result = m_sourceManager->mergeReferences(m_references); result != nullptr)
        return result;

    throw std::runtime_error("Invalid merged source reference in MultiSourceReferenceCreator");
    return nullptr;
}
