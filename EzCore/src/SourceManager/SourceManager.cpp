#include "SourceManager/SourceManager.h"

SourceManager::SourceManager() {}

SourceManager::~SourceManager() { m_source.clear(); }

bool SourceManager::addSourceContent(const std::string &name, const std::string &content)
{
    if (m_source.contains(name))
        return false;

    size_t previousPos = 0;
    std::vector<std::string> lines;

    size_t currentPos = content.find('\n', previousPos);
    while (currentPos != std::string::npos)
    {
        lines.push_back(content.substr(previousPos, currentPos - previousPos));

        previousPos = currentPos + 1;
        currentPos = content.find('\n', previousPos);
    }

    // Handle the last line (or the whole content if no newlines were found)
    if (previousPos < content.size())
    {
        lines.push_back(content.substr(previousPos));
    }

    m_source[name] = std::move(lines);
    return true;
}

bool SourceManager::addSourceFile(const std::string &sourceFile, std::vector<std::string> content)
{
    if (m_source.contains(sourceFile))
        return false;

    m_source[sourceFile] = std::move(content);
    return true;
}

std::shared_ptr<SourceReference>
SourceManager::createReference(size_t col, size_t length, size_t line, const std::string &sourceFile)
{
    auto it = m_source.find(sourceFile);
    if (it == m_source.end())
    {
        return nullptr; // Given file is not in the source list.
    }

    if (line >= it->second.size())
    {
        return nullptr; // Invalid line of source file.
    }

    std::shared_ptr<SourceReference> ref = std::make_shared<SourceReference>();
    ref->m_col = col;
    ref->m_length = length;
    ref->m_line = line;
    ref->m_sourceFile = sourceFile;

    size_t lineSize = it->second[line].size();
    if (col >= lineSize || length > (lineSize - col))
    {
        ref->m_length = 0; // This is caused by last empty line.
        return ref;
    }

    return ref;
}

std::string SourceManager::getReferenceContent(const std::shared_ptr<SourceReference> &ref)
{
    std::string result;

    auto it = m_source.find(ref->m_sourceFile);
    if (it != m_source.end() && ref->m_line < it->second.size())
    {
        const std::string &line = it->second[ref->m_line];

        // Add content before the error
        if (ref->m_col < line.size())
            result += line.substr(0, ref->m_col);

        // Add the marked error section
        if (ref->m_col < line.size() && ref->m_length > 0)
        {
            size_t length = std::min(ref->m_length, line.size() - ref->m_col);
            result += "~~" + line.substr(ref->m_col, length) + "~~";
        }

        // Add the content after the marked section
        if (ref->m_col + ref->m_length < line.size())
            result += line.substr(ref->m_col + ref->m_length);
    }

    return result;
}
