#include "CodeGenerators/CodeGenerator.h"

namespace CodeGenerators
{

CodeGenerator::CodeGenerator(std::string_view generatorName,
                             DiagnosticCollector *collector,
                             SymbolTable *table,
                             std::filesystem::path outPath) :
    m_generatorName(generatorName),
    m_collector(collector),
    m_table(table),
    m_outputPath(std::move(outPath))
{
}

bool CodeGenerator::validate() const
{
    if (!m_collector)
    {
        return false;
    }

    if (!m_table)
    {
        error("Cannot run {} with a null SymbolTable.", m_generatorName);
        return false;
    }

    if (m_outputPath.empty())
    {
        error("Output path for {} cannot be empty.", m_generatorName);
        return false;
    }

    return true;
}

std::filesystem::path CodeGenerator::resolveSingleFilePath(std::string_view defaultFileName) const
{
    if (std::filesystem::is_directory(m_outputPath) || !m_outputPath.has_extension())
    {
        return m_outputPath / defaultFileName;
    }
    return m_outputPath;
}

CodeGenerator::HeaderAndSourcePaths CodeGenerator::resolveHeaderAndSourcePaths(std::string_view defaultBaseName) const
{
    HeaderAndSourcePaths result;

    if (std::filesystem::is_directory(m_outputPath) || !m_outputPath.has_extension())
    {
        result.m_headerPath = m_outputPath / std::format("{}.h", defaultBaseName);
        result.m_sourcePath = m_outputPath / std::format("{}.cpp", defaultBaseName);
    }
    else
    {
        std::string ext = m_outputPath.extension().string();
        if (ext == ".h" || ext == ".hpp")
        {
            result.m_headerPath = m_outputPath;
            result.m_sourcePath = m_outputPath;
            result.m_sourcePath.replace_extension(".cpp");
        }
        else
        {
            result.m_sourcePath = m_outputPath;
            result.m_headerPath = m_outputPath;
            result.m_headerPath.replace_extension(".h");
        }
    }

    return result;
}

bool CodeGenerator::WriteFileIfChanged(const std::filesystem::path &filePath,
                                      std::string_view newContent,
                                      std::string *errorOut)
{
    // Check if the file already exists and has identical content
    if (std::filesystem::exists(filePath))
    {
        std::ifstream currentFile(filePath, std::ios::in | std::ios::binary);
        if (currentFile.is_open())
        {
            std::ostringstream ss;
            ss << currentFile.rdbuf();
            if (ss.str() == newContent)
            {
                // Identical content: leave file unmodified to preserve mtime
                return true;
            }
        }
    }

    // Ensure parent directory exists
    if (filePath.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(filePath.parent_path(), ec);
        if (ec)
        {
            if (errorOut)
            {
                *errorOut = std::format("Failed to create parent directory '{}': {}",
                                        filePath.parent_path().string(),
                                        ec.message());
            }
            return false;
        }
    }

    // Write new content
    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!outFile.is_open())
    {
        if (errorOut)
        {
            *errorOut = std::format("Failed to open file '{}' for writing.", filePath.string());
        }
        return false;
    }

    outFile.write(newContent.data(), static_cast<std::streamsize>(newContent.size()));
    if (!outFile.good())
    {
        if (errorOut)
        {
            *errorOut = std::format("Failed during writing to file '{}'.", filePath.string());
        }
        return false;
    }

    return true;
}

bool CodeGenerator::writeOutput(const std::filesystem::path &filePath, std::string_view content) const
{
    std::string err;
    if (!WriteFileIfChanged(filePath, content, &err))
    {
        error("{}", err);
        return false;
    }

    trace("Generated file: {}", filePath.string());
    return true;
}

} // namespace CodeGenerators
