#include "CodeGenerators/CppEncodingTableGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "NameRegistry.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetSymbols.h"

#include <format>
#include <string>
#include <vector>

namespace CodeGenerators
{

namespace
{

// Function-local static registry: constructed on first use to avoid static-initialization order issues.
NameRegistry<EncodingCodegenBackend *> &backendRegistry()
{
    static NameRegistry<EncodingCodegenBackend *> s_registry;
    return s_registry;
}

} // namespace

void registerEncodingBackend(std::string_view name, EncodingCodegenBackend *backend)
{
    if (name.empty() || !backend)
    {
        return;
    }

    backendRegistry().add(name, backend);
}

EncodingCodegenBackend *findEncodingBackend(std::string_view name) { return backendRegistry().find(name); }

// Binds the generator to its diagnostics/symbols and selects the backend by target name.
CppEncodingTableGenerator::CppEncodingTableGenerator(DiagnosticCollector *collector,
                                                     SymbolTable *table,
                                                     std::filesystem::path outPath,
                                                     std::string targetName) :
    CodeGenerator("CodeGenerators::EncodingTable", collector, table, std::move(outPath)),
    m_targetName(SanitizeCppIdentifier(targetName, "Target"))
{
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
        const auto instSymbols = m_table
                ? m_table->collect<Symbols::TargetInstructionSymbol>(SymbolType::TargetInstruction)
                : std::vector<const Symbol *>{};

        emitter.emitLine("inline constexpr {} s_encodings[] =", m_backend->arrayType());
        {
            auto scope = emitter.enterScope("{", "};");
            for (const Symbol *sym : instSymbols)
            {
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();
                emitter.emitLine("{},", m_backend->row(*data, getCollector()));
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

    const std::string baseName = std::format("{}EncodingTable", SanitizeCppIdentifier(m_targetName, "Target"));
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

} // namespace CodeGenerators
