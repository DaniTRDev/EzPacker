#include "SourceManager/SourceManager.h"
#include <algorithm>
#include <stdexcept>

SourceManager::SourceManager(const std::filesystem::path &workingPath) : m_workingPath(workingPath) {}

bool SourceManager::doesSourceNameExist(const std::string_view &sourceName) const
{
    size_t id = std::hash<std::string_view>{}(resolveSourcePath(sourceName).string());
    return m_sources.contains(id);
}

size_t SourceManager::addSourceContent(const std::string &name, const std::string &content)
{
    std::string resolvedName = resolveSourcePath(name).string();
    size_t id = std::hash<std::string>{}(resolvedName);

    if (m_sources.contains(id))
        return 0; // Source with the same name already exists, return 0 to indicate failure.

    // Store the content
    m_sources[id] = content;
    m_sourcesNames[id] = resolvedName;

    // We will build the line ranges.
    std::vector<LineSourceRange> &lines = m_sourceLines[id];
    lines.reserve(content.size() / 40); // Optimization: estimate avg line length of 40 chars.

    size_t lineStart = 0;
    size_t currentPos = 0;
    const size_t contentSize = content.size();

    while (currentPos < contentSize)
    {
        if (content[currentPos] == '\n')
        {
            // Store offset and length (excluding the newline character)
            lines.push_back({ .m_start = lineStart, .m_length = currentPos - lineStart });
            lineStart = currentPos + 1;
        }
        currentPos++;
    }

    // Handle the last line (if the file doesn't end with a newline, or even if it's empty)
    if (lineStart <= contentSize)
    {
        lines.push_back({ .m_start = lineStart, .m_length = contentSize - lineStart });
    }

    return id;
}

SourceReference SourceManager::createReference(size_t col, size_t length, size_t line, const std::string &sourceFile)
{
    size_t id = std::hash<std::string>{}(resolveSourcePath(sourceFile).string());
    return createReference(col, length, line, id);
}

SourceReference SourceManager::createReference(size_t col, size_t length, size_t line, size_t sourceId)
{
    auto it = m_sourceLines.find(sourceId);
    if (it == m_sourceLines.end())
        return {}; // File not found

    const std::vector<LineSourceRange> &lines = it->second;

    if (line >= lines.size())
        return {}; // Invalid line number

    const LineSourceRange &lineRange = lines[line];

    // Bounds check: Column + Length must not exceed the actual line length
    if ((col + length) > lineRange.m_length)
        return {};

    SourceReference ref;
    ref.m_valid = true;
    ref.m_col = col;
    ref.m_length = length;
    ref.m_line = line;
    ref.m_sourceFileId = sourceId;

    return ref;
}

const std::filesystem::path &SourceManager::getWorkingPath() const { return m_workingPath; }

std::filesystem::path SourceManager::resolveSourcePath(const std::filesystem::path &sourceFile) const
{
    std::filesystem::path sourcePath;
    if (sourceFile.is_relative())
    {
        sourcePath = m_workingPath / sourceFile;
    }
    return sourcePath;
}

std::string SourceManager::getRawLineContent(const SourceReference &ref)
{
    if (!ref.m_valid)
        return "";

    auto itSource = m_sources.find(ref.m_sourceFileId);
    auto itLines = m_sourceLines.find(ref.m_sourceFileId);

    if (itSource == m_sources.end() || itLines == m_sourceLines.end())
        return "";

    const std::vector<LineSourceRange> &lines = itLines->second;
    if (ref.m_line >= lines.size())
        return "";

    const LineSourceRange &lineRange = lines[ref.m_line];
    const std::string &fullSource = itSource->second;

    return fullSource.substr(lineRange.m_start, lineRange.m_length);
}

std::string SourceManager::getReferenceContent(const SourceReference &ref)
{
    auto lineContentOpt = getRawLineContent(ref);

    if (lineContentOpt.empty())
        return "Internal Compiler Error: Invalid SourceReference";

    std::string lineContent = lineContentOpt;
    std::string indent;
    indent.reserve(ref.m_col);

    // Build indentation mirroring tabs
    for (size_t i = 0; i < ref.m_col && i < lineContent.size(); ++i)
    {
        indent += (lineContent[i] == '\t') ? '\t' : ' ';
    }

    size_t markLength = ref.m_length;
    if (ref.m_col + markLength > lineContent.size())
    {
        markLength = (ref.m_col < lineContent.size()) ? lineContent.size() - ref.m_col : 0;
    }
    if (markLength == 0)
        markLength = 1;

    std::string squiggles = "^";
    if (markLength > 1)
        squiggles += std::string(markLength - 1, '~');

    return std::format("{}\n{}{}", lineContent, indent, squiggles);
}

std::string_view SourceManager::getSourceContent(size_t id) const
{
    auto it = m_sources.find(id);
    if (it == m_sources.end())
    {
        return "";
    }

    return it->second;
}

std::string_view SourceManager::getSourceName(size_t id) const
{
    auto it = m_sourcesNames.find(id);
    if (it == m_sourcesNames.end())
    {
        return "";
    }

    return it->second;
}
