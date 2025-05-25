#include "SourceManager/SourceManager.h"

SourceManager::SourceManager()
{
}

SourceManager::~SourceManager()
{
    m_source.clear();
}

bool SourceManager::addSourceContent(const std::string &name, const std::string &content)
{
    if (m_source.contains(name))
        return false;

    size_t previousPos = 0;
    std::vector<std::string> lines;

    while(size_t currentPos = content.find('\n', previousPos))
    {
        lines.push_back(content.substr(previousPos, currentPos));
        previousPos = currentPos + 1;

        // 'If' is here, so we catch last line or the only line, if no \n was found.
        if (currentPos == std::string::npos)
            break;
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

std::shared_ptr<SourceReference> SourceManager::createReference(size_t col, size_t length, size_t line,
                                                                const std::string &sourceFile)
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

    size_t lineSize = it->second[line].size();
    if (col >= lineSize || length > (lineSize - col))
    {
        return nullptr; // Invalid column of the current line.
    }

    std::shared_ptr<SourceReference> ref = std::make_shared<SourceReference>();
    ref->m_col = col;
    ref->m_length = length;
    ref->m_line = line;
    ref->m_sourceFile = sourceFile;

    return ref;
}

std::string_view SourceManager::getReferenceContent(const std::shared_ptr<SourceReference> &ref)
{
    std::string_view result;

    auto it = m_source.find(ref->m_sourceFile);
    if (it != m_source.end())
    {
        result = std::string_view(it->second[ref->m_line].data() + ref->m_col, ref->m_length);
    }

    return std::move(result);
}
