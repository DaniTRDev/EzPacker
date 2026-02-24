#include "SourceManager/SourceManager.h"
#include <algorithm>
#include <stdexcept>

bool SourceManager::addSourceContent(const std::string &name, const std::string &content)
{
    // C++20 contains check
    if (m_sources.contains(name))
        return false;

    // Store the content
    m_sources[name] = content;

    // We will build the line ranges.
    // Optimization: Reserve distinct memory based on a heuristic to avoid reallocations,
    // though not strictly necessary for correctness.
    std::vector<LineSourceRange> &lines = m_sourceLines[name];
    lines.reserve(content.size() / 40); // Estimate avg line length of 40 chars

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

    return true;
}

std::shared_ptr<SourceReference>
SourceManager::createReference(size_t col, size_t length, size_t line, const std::string &sourceFile)
{
    auto it = m_sourceLines.find(sourceFile);
    if (it == m_sourceLines.end())
        return nullptr; // File not found

    const std::vector<LineSourceRange> &lines = it->second;

    if (line >= lines.size())
        return nullptr; // Invalid line number

    const LineSourceRange &lineRange = lines[line];

    // Bounds check: Column + Length must not exceed the actual line length
    if ((col + length) > lineRange.m_length)
        return nullptr;

    auto ref = std::make_shared<SourceReference>();
    ref->m_col = col;
    ref->m_length = length;
    ref->m_line = line;
    ref->m_sourceFile = sourceFile;

    return ref;
}

std::string SourceManager::getReferenceContent(const std::shared_ptr<SourceReference> &ref)
{
    if (!ref)
        return {};

    auto itSource = m_sources.find(ref->m_sourceFile);
    auto itLines = m_sourceLines.find(ref->m_sourceFile);

    if (itSource == m_sources.end() || itLines == m_sourceLines.end())
        return {};

    const std::vector<LineSourceRange> &lines = itLines->second;
    if (ref->m_line >= lines.size())
        return {};

    const LineSourceRange &lineRange = lines[ref->m_line];
    const std::string &fullSource = itSource->second;

    // Note: lineRange.m_length excludes the newline character.
    std::string lineContent = fullSource.substr(lineRange.m_start, lineRange.m_length);

    std::string markerLine;
    markerLine.reserve(lineContent.size() + 1);

    // Step 4a: Build the prefix (indentation)
    // We iterate through the line up to the error column.
    // IMPORTANT: We mirror tabs as tabs and other chars as spaces to preserve visual alignment.
    for (size_t i = 0; i < ref->m_col && i < lineContent.size(); ++i)
    {
        if (lineContent[i] == '\t')
            markerLine += '\t';
        else
            markerLine += ' ';
    }

    // We clamp the length to ensure we don't overflow the actual line length
    size_t markLength = ref->m_length;
    if (ref->m_col + markLength > lineContent.size())
    {
        markLength = (ref->m_col < lineContent.size()) ? lineContent.size() - ref->m_col : 0;
    }

    // Ensure we print at least one caret if the length is 0 (e.g. EOF or single char token)
    if (markLength == 0)
        markLength = 1;

    markerLine.append(markLength, '^');

    return lineContent + "\n" + markerLine;
}

std::shared_ptr<SourceReference>
SourceManager::mergeReferences(const std::vector<std::shared_ptr<SourceReference>> &refs)
{
    if (refs.empty())
        return nullptr;

    // Validation: All references must belong to the same file and the same line
    // to be mergeable into a single contiguous block (conceptually).
    const std::string &targetFile = refs[0]->m_sourceFile;
    const size_t targetLine = refs[0]->m_line;

    uint64_t minCol = UINT64_MAX;
    uint64_t maxBound = 0;

    for (const auto &ref : refs)
    {
        if (!ref)
            continue;

        // Sanity check: Can only merge refs on same line of same file
        if (ref->m_sourceFile != targetFile || ref->m_line != targetLine)
        {
            throw std::runtime_error("Internal compiler error: Tried to merge a multi-reference in different lines");
        }

        minCol = std::min(minCol, (uint64_t)ref->m_col);
        maxBound = std::max(maxBound, (uint64_t)(ref->m_col + ref->m_length));
    }

    if (minCol == UINT64_MAX)
        return nullptr; // All inputs were nullptr

    return std::make_shared<SourceReference>(SourceReference{ .m_col = (size_t)minCol,
                                                              .m_length = (size_t)(maxBound - minCol),
                                                              .m_line = targetLine,
                                                              .m_sourceFile = targetFile });
}

void MultiSourceReferenceCreator::attach(std::shared_ptr<SourceManager> sourceManager)
{
    if (!sourceManager)
        throw std::invalid_argument("Invalid source manager passed to attach");

    m_sourceManager = std::move(sourceManager);
}

void MultiSourceReferenceCreator::push(std::shared_ptr<SourceReference> reference)
{
    if (!reference)
        throw std::invalid_argument("Invalid source reference passed to push");

    m_references.push_back(std::move(reference));
}

std::shared_ptr<SourceReference> MultiSourceReferenceCreator::merge()
{
    if (!m_sourceManager)
        throw std::runtime_error("SourceManager not attached to MultiSourceReferenceCreator");

    if (m_references.empty())
        throw std::runtime_error("No references to merge");

    auto result = m_sourceManager->mergeReferences(m_references);

    if (result)
    {
        return result;
    }

    throw std::runtime_error("Failed to merge references (likely file/line mismatch)");
}