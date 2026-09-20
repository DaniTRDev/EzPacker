#include "CodeGenerators/CppEncodingTableGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetSymbols.h"

#include <cctype>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

namespace CodeGenerators
{

namespace
{

// Normalizes a backend name to a case-insensitive lookup key.
std::string normalizeBackendName(std::string_view name)
{
    std::string result;
    result.reserve(name.size());
    for (char c : name)
    {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

std::unordered_map<std::string, EncodingCodegenBackend *> &backendRegistry()
{
    static std::unordered_map<std::string, EncodingCodegenBackend *> s_registry;
    return s_registry;
}

// Rewrites raw into a valid C++ identifier, substituting illegal characters and prefixing leading digits.
std::string sanitizeIdentifier(std::string_view raw, std::string_view fallback)
{
    std::string result;
    result.reserve(raw.size());
    for (char c : raw)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || c == '_')
        {
            result.push_back(c);
        }
        else
        {
            result.push_back('_');
        }
    }
    if (result.empty())
    {
        result = std::string(fallback);
    }
    if (std::isdigit(static_cast<unsigned char>(result.front())))
    {
        result.insert(result.begin(), '_');
    }
    return result;
}

} // namespace

void registerEncodingBackend(std::string_view name, EncodingCodegenBackend *backend)
{
    if (name.empty() || !backend)
    {
        return;
    }
    backendRegistry()[normalizeBackendName(name)] = backend;
}

EncodingCodegenBackend *findEncodingBackend(std::string_view name)
{
    if (name.empty())
    {
        return nullptr;
    }
    auto &registry = backendRegistry();
    auto it = registry.find(normalizeBackendName(name));
    return it == registry.end() ? nullptr : it->second;
}

// Binds the generator to its diagnostics/symbols and selects the backend by target name.
CppEncodingTableGenerator::CppEncodingTableGenerator(DiagnosticCollector *collector,
                                                     SymbolTable *table,
                                                     std::filesystem::path outPath,
                                                     std::string targetName) :
    CodeGenerator("CodeGenerators::EncodingTable", collector, table, std::move(outPath)),
    m_targetName(std::move(targetName))
{
    if (m_targetName.empty())
    {
        m_targetName = "Target";
    }
    m_backend = findEncodingBackend(m_targetName);
}

// Emits the header-only encoding descriptor table and its id/name lookup helpers.
void CppEncodingTableGenerator::emitHeader(CppSourceEmitter &emitter) const
{
    if (!m_backend)
    {
        return;
    }

    emitter.emitBanner("CppEncodingTableGenerator");
    emitter.emitBlankLine();
    emitter.emitLine("#pragma once");
    emitter.emitBlankLine();
    emitter.emitInclude(m_backend->includeHeader());
    emitter.emitLine("#include <cstddef>");
    emitter.emitLine("#include <cstring>");
    emitter.emitBlankLine();

    emitter.emitComment("Table-driven instruction encodings generated from ENCODING blocks in the .idf file.");
    {
        auto nsScope = emitter.enterNamespace(m_backend->namespaceName());
        emitter.emitBlankLine();

        // Collect target instructions in symbol-table order; encodings stay index-aligned with them.
        std::vector<const Symbol *> instSymbols;
        if (m_table)
        {
            for (const Symbol *sym : m_table->getSymbols())
            {
                if (sym && sym->getType() == SymbolType::TargetInstruction &&
                    sym->hasData<Symbols::TargetInstructionSymbol>())
                {
                    instSymbols.push_back(sym);
                }
            }
        }

        emitter.emitLine("inline constexpr {} s_encodings[] =", m_backend->arrayType());
        {
            auto scope = emitter.enterScope("{", "};");
            for (const Symbol *sym : instSymbols)
            {
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();
                emitter.emitLine("{},", m_backend->row(*data));
            }
            if (instSymbols.empty())
            {
                emitter.emitLine("{}{{}},", m_backend->arrayType());
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_encodingCount = {};", instSymbols.size());
        emitter.emitBlankLine();

        emitter.emitLine("inline constexpr const char *s_encodingNames[] =");
        {
            auto scope = emitter.enterScope("{", "};");
            for (const Symbol *sym : instSymbols)
            {
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();
                emitter.emitLine("\"{}\",", data->m_name);
            }
            if (instSymbols.empty())
            {
                emitter.emitLine("\"\",");
            }
        }
        emitter.emitBlankLine();

        emitter.emitLine("inline const {} *getEncodingDesc(std::size_t id)", m_backend->arrayType());
        {
            auto scope = emitter.enterScope();
            emitter.emitLine("if (id == 0 || id > s_encodingCount)");
            {
                auto inner = emitter.enterScope();
                emitter.emitLine("return nullptr;");
            }
            emitter.emitLine("return &s_encodings[id - 1];");
        }
        emitter.emitBlankLine();

        emitter.emitLine("inline const {} *findEncodingDesc(const char *name)", m_backend->arrayType());
        {
            auto scope = emitter.enterScope();
            emitter.emitLine("if (!name) return nullptr;");
            emitter.emitLine("for (std::size_t i = 0; i < s_encodingCount; ++i)");
            {
                auto inner = emitter.enterScope();
                emitter.emitLine("if (std::strcmp(s_encodingNames[i], name) == 0) return &s_encodings[i];");
            }
            emitter.emitLine("return nullptr;");
        }
    }
}

// Emits the encoding table for the current symbol table and writes the single output header.
bool CppEncodingTableGenerator::run()
{
    if (!validate())
    {
        return false;
    }

    if (!m_backend)
    {
        error("No encoding code generation backend registered for target '{}'", m_targetName);
        return false;
    }

    const std::string baseName = std::format("{}EncodingTable", sanitizeIdentifier(m_targetName, "Target"));
    const auto targetFilePath = resolveSingleFilePath(baseName + ".h");

    CppSourceEmitter emitter;
    emitHeader(emitter);

    if (!writeOutput(targetFilePath, emitter.view()))
    {
        return false;
    }

    trace("Synthesized target encoding table into {}", targetFilePath.filename().string());
    return true;
}

// Convenience wrapper retained for callers that do not need to configure a generator object.
bool GenerateEncodingTable(DiagnosticCollector *collector,
                           SymbolTable *table,
                           std::filesystem::path outPath,
                           std::string targetName)
{
    CppEncodingTableGenerator generator(collector, table, std::move(outPath), std::move(targetName));
    return generator.run();
}

} // namespace CodeGenerators
