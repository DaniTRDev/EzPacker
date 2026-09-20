#include "CodeGenerators/CodeGenerator.h"

namespace CodeGenerators
{

// Stores the generator identity and the diagnostics/symbol/output dependencies shared by all passes.
CodeGenerator::CodeGenerator(std::string_view generatorName,
                             DiagnosticCollector *collector,
                             SymbolTable *table,
                             std::filesystem::path outPath) :
    m_generatorName(generatorName), m_collector(collector), m_table(table), m_outputPath(std::move(outPath))
{
}

// Confirms the collector and symbol table are non-null and an output path was supplied.
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

// Treats the output path as a directory unless it already names a file with an extension.
std::filesystem::path CodeGenerator::resolveSingleFilePath(std::string_view defaultFileName) const
{
    if (std::filesystem::is_directory(m_outputPath) || !m_outputPath.has_extension())
    {
        return m_outputPath / defaultFileName;
    }
    return m_outputPath;
}

// Derives the header/source pair, honoring an explicit .h/.hpp/.cpp/.cxx output path when given.
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

// Reads and compares the destination, creating parent directories and writing atomically only on change.
bool CodeGenerator::WriteFileIfChanged(const std::filesystem::path &filePath,
                                       std::string_view newContent,
                                       std::string *errorOut)
{
    // Compare sizes first, then read the existing file once; identical content leaves the mtime untouched.
    std::error_code existsEc;
    if (std::filesystem::exists(filePath, existsEc) && !existsEc)
    {
        std::ifstream currentFile(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        if (currentFile.is_open())
        {
            const std::streamoff size = currentFile.tellg();
            if (size >= 0 && static_cast<size_t>(size) == newContent.size())
            {
                std::string current(static_cast<size_t>(size), '\0');
                currentFile.seekg(0, std::ios::beg);
                currentFile.read(current.data(), static_cast<std::streamsize>(current.size()));
                if (current == newContent)
                {
                    // Identical content: leave file unmodified to preserve mtime
                    return true;
                }
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

    // Stage the new content in a sibling file, then rename it over the destination. A crash mid-write
    // can therefore only leave the destination intact or the stale temporary, never a truncated output.
    std::filesystem::path tempPath = filePath;
    tempPath += ".tmp";

    {
        std::ofstream outFile(tempPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!outFile.is_open())
        {
            if (errorOut)
            {
                *errorOut = std::format("Failed to open file '{}' for writing.", tempPath.string());
            }
            return false;
        }

        outFile.write(newContent.data(), static_cast<std::streamsize>(newContent.size()));
        outFile.flush();
        if (!outFile.good())
        {
            if (errorOut)
            {
                *errorOut = std::format("Failed during writing to file '{}'.", tempPath.string());
            }
            outFile.close();
            std::error_code removeEc;
            std::filesystem::remove(tempPath, removeEc);
            return false;
        }
    }

    std::error_code renameEc;
    std::filesystem::rename(tempPath, filePath, renameEc);
    if (renameEc)
    {
        // Some platforms refuse to overwrite via rename; fall back to remove + rename.
        std::error_code removeEc;
        std::filesystem::remove(filePath, removeEc);
        std::filesystem::rename(tempPath, filePath, renameEc);
        if (renameEc)
        {
            if (errorOut)
            {
                *errorOut = std::format("Failed to replace file '{}': {}", filePath.string(), renameEc.message());
            }
            std::filesystem::remove(tempPath, removeEc);
            return false;
        }
    }

    return true;
}

// Writes the file after a content comparison, then records a trace diagnostic on success.
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
