#include "SourceManager/SourceManager.h"
#include <algorithm>
#include <stdexcept>

SourceManager::SourceManager(const std::filesystem::path &workingPath) :
    m_workingPath(std::filesystem::absolute(workingPath))
{
    // Reserve ID 0 as an invalid/empty identifier flag
    m_sourceFiles.push_back({ .name = "INVALID", .content = "", .lines = {} });
}

bool SourceManager::doesSourceNameExist(const std::string_view &sourceName) const
{
    std::string resolved = resolveSourcePath(sourceName).string();
    return m_pathToIdMap.contains(resolved);
}

size_t SourceManager::addSourceContent(const std::string &name, const std::string &content)
{
    std::string resolvedName = resolveSourcePath(name).string();

    if (m_pathToIdMap.contains(resolvedName))
    {
        return 0;
    }

    size_t assignedId = m_sourceFiles.size();
    m_pathToIdMap[resolvedName] = assignedId;

    SourceFileEntry entry;
    entry.name = resolvedName;
    entry.content = content;
    entry.lines.reserve(content.size() / 40);

    size_t lineStart = 0;
    size_t currentPos = 0;
    const size_t contentSize = content.size();

    while (currentPos < contentSize)
    {
        if (content[currentPos] == '\n')
        {
            size_t lineLength = currentPos - lineStart;
            // Robust CRLF handling: Strip trailing \r line weights
            if (lineLength > 0 && content[currentPos - 1] == '\r')
            {
                lineLength--;
            }
            entry.lines.push_back({ .m_start = lineStart, .m_length = lineLength });
            lineStart = currentPos + 1;
        }
        currentPos++;
    }

    if (lineStart <= contentSize)
    {
        size_t lineLength = contentSize - lineStart;
        if (lineLength > 0 && content[contentSize - 1] == '\r')
        {
            lineLength--;
        }
        entry.lines.push_back({ .m_start = lineStart, .m_length = lineLength });
    }

    m_sourceFiles.push_back(std::move(entry));
    return assignedId;
}

SourceReference SourceManager::createReference(size_t col, size_t length, size_t line, size_t sourceId)
{
    if (sourceId >= m_sourceFiles.size() || sourceId == 0)
        return {};

    const auto &lines = m_sourceFiles[sourceId].lines;
    if (line >= lines.size())
        return {};

    const auto &lineRange = lines[line];
    if ((col + length) > lineRange.m_length)
        return {};

    return SourceReference{ true, col, length, line, sourceId };
}

SourceReference SourceManager::createReference(size_t col, size_t length, size_t line, const std::string &sourceFile)
{
    std::string resolved = resolveSourcePath(sourceFile).string();
    auto it = m_pathToIdMap.find(resolved);
    if (it == m_pathToIdMap.end())
        return {};
    return createReference(col, length, line, it->second);
}

std::filesystem::path SourceManager::resolveSourcePath(const std::filesystem::path &sourceFile) const
{
    return sourceFile.is_relative() ? (m_workingPath / sourceFile) : sourceFile;
}

std::string SourceManager::getRawLineContent(const SourceReference &ref) const
{
    if (!ref.m_valid || ref.m_sourceFileId >= m_sourceFiles.size())
        return "";

    const auto &file = m_sourceFiles[ref.m_sourceFileId];
    if (ref.m_line >= file.lines.size())
        return "";

    const auto &range = file.lines[ref.m_line];
    return file.content.substr(range.m_start, range.m_length);
}

std::string SourceManager::getReferenceContent(const SourceReference &ref) const
{
    std::string lineContent = getRawLineContent(ref);
    if (lineContent.empty() && ref.m_valid)
    {
        return "Internal Compiler Error: Malformed Source Reference";
    }

    std::string indent;
    indent.reserve(ref.m_col);
    for (size_t i = 0; i < ref.m_col && i < lineContent.size(); ++i)
    {
        indent += (lineContent[i] == '\t') ? '\t' : ' ';
    }

    size_t markLength = std::max<size_t>(1, std::min(ref.m_length, lineContent.size() - ref.m_col));
    std::string squiggles = "^" + std::string(markLength - 1, '~');

    return std::format("{}\n{}{}", lineContent, indent, squiggles);
}

std::string_view SourceManager::getSourceContent(size_t id) const
{
    return (id < m_sourceFiles.size()) ? m_sourceFiles[id].content : "";
}

std::string_view SourceManager::getSourceName(size_t id) const
{
    return (id < m_sourceFiles.size()) ? m_sourceFiles[id].name : "";
}
