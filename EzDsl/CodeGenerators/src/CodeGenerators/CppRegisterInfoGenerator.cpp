#include "CodeGenerators/CppRegisterInfoGenerator.h"
#include "Ast/RegisterDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/RegisterSymbols.h"

#include <cctype>
#include <format>
#include <map>
#include <string>
#include <vector>

namespace CodeGenerators
{

namespace
{

struct RegisterEntryRow
{
    std::string m_bank;
    std::string m_class;
    uint32_t m_bitSize{ 0 };
    uint32_t m_hwEncoding{ 0 };
    std::string m_asmName;
};

struct SubRegEdgeRow
{
    std::string m_parentClass;
    std::string m_parentAsm;
    std::string m_childClass;
    std::string m_childAsm;
};

struct SpecialRow
{
    std::string m_name;
    uint32_t m_id{ 0 };
};

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

struct CollectedRegisterData
{
    std::vector<RegisterEntryRow> m_entries;
    std::vector<SubRegEdgeRow> m_edges;
    std::vector<SpecialRow> m_specials;
    std::string m_targetName;
};

CollectedRegisterData collectData(const SymbolTable *table, std::string_view targetName)
{
    CollectedRegisterData data;
    data.m_targetName = std::string(targetName);

    if (!table)
    {
        return data;
    }

    const DSL::Ast::RegisterDef::RegisterFile *file = nullptr;
    for (const Symbol *sym : table->getSymbols())
    {
        if (!sym || sym->getType() != SymbolType::RegisterFile)
        {
            continue;
        }
        if (const auto *fileSym = sym->getIf<Symbols::RegisterFileSymbol>())
        {
            file = fileSym->m_astNode;
            break;
        }
    }

    if (!file)
    {
        return data;
    }

    for (const auto &bank : file->m_banks)
    {
        const std::string bankName(bank.m_name.m_node);

        std::map<std::string_view, uint32_t> classBits;
        for (const auto &cls : bank.m_classes)
        {
            classBits[cls.m_name.m_node] = static_cast<uint32_t>(cls.m_bitSize.m_node);
        }

        for (const auto &reg : bank.m_registers)
        {
            const uint32_t encoding = static_cast<uint32_t>(reg.m_encoding.m_node);

            for (const auto &binding : reg.m_names)
            {
                auto bitIt = classBits.find(binding.m_className.m_node);
                if (bitIt == classBits.end())
                {
                    continue;
                }
                data.m_entries.push_back(RegisterEntryRow{ .m_bank = bankName,
                                                           .m_class = std::string(binding.m_className.m_node),
                                                           .m_bitSize = bitIt->second,
                                                           .m_hwEncoding = encoding,
                                                           .m_asmName = std::string(binding.m_asmName.m_node) });
            }

            // Build per-register alias edge rows from the bank's sub-register relations.
            for (const auto &edge : bank.m_subRegisterEdges)
            {
                std::string_view parentAsm;
                std::string_view childAsm;

                for (const auto &binding : reg.m_names)
                {
                    if (binding.m_className.m_node == edge.m_wideClass.m_node)
                    {
                        parentAsm = binding.m_asmName.m_node;
                    }
                    if (binding.m_className.m_node == edge.m_narrowClass.m_node)
                    {
                        childAsm = binding.m_asmName.m_node;
                    }
                }

                if (!parentAsm.empty() && !childAsm.empty())
                {
                    data.m_edges.push_back(SubRegEdgeRow{ .m_parentClass = std::string(edge.m_wideClass.m_node),
                                                          .m_parentAsm = std::string(parentAsm),
                                                          .m_childClass = std::string(edge.m_narrowClass.m_node),
                                                          .m_childAsm = std::string(childAsm) });
                }
            }
        }
    }

    for (const auto &special : file->m_specialRegs)
    {
        data.m_specials.push_back(
                SpecialRow{ .m_name = std::string(special.m_name.m_node), .m_id = static_cast<uint32_t>(special.m_id.m_node) });
    }

    return data;
}

} // namespace

CppRegisterInfoGenerator::CppRegisterInfoGenerator(DiagnosticCollector *collector,
                                                   SymbolTable *table,
                                                   std::filesystem::path outPath,
                                                   std::string targetName) :
    CodeGenerator("CodeGenerators::RegisterInfo", collector, table, std::move(outPath)),
    m_targetName(std::move(targetName))
{
    if (m_targetName.empty())
    {
        m_targetName = "Target";
    }
}

void CppRegisterInfoGenerator::emitHeader(CppSourceEmitter &emitter) const
{
    const auto data = collectData(getSymbolTable(), m_targetName);
    const std::string ns = sanitizeIdentifier(m_targetName, "Target");
    const std::string emissionNs = std::format("EzCodeEmitter::TableGen::{}", ns);

    emitter.emitBanner("CppRegisterInfoGenerator");
    emitter.emitBlankLine();
    emitter.emitLine("#pragma once");
    emitter.emitBlankLine();

    emitter.emitInclude("cstddef", true);
    emitter.emitInclude("cstdint", true);
    emitter.emitInclude("memory_resource", true);
    emitter.emitInclude("string_view", true);
    emitter.emitInclude("unordered_map", true);
    emitter.emitBlankLine();

    emitter.emitInclude("Operand/MirRegisterBank.h");
    emitter.emitInclude("Operand/MirRegisterClass.h");
    emitter.emitBlankLine();

    {
        auto nsScope = emitter.enterNamespace(emissionNs);
        emitter.emitBlankLine();

        emitter.emitComment("Flat register table consumed by EzTriple (bank/class setup) and EzCodeEmitter (encoding lookup).");
        {
            auto s = emitter.enterStruct("RegisterInfoEntry");
            emitter.emitLine("const char *bankName;");
            emitter.emitLine("const char *className;");
            emitter.emitLine("uint32_t bitSize;");
            emitter.emitLine("uint32_t hwEncoding;");
            emitter.emitLine("const char *asmName;");
        }
        emitter.emitBlankLine();

        {
            auto s = emitter.enterStruct("SubRegEdge");
            emitter.emitLine("const char *parentClass;");
            emitter.emitLine("const char *parentAsm;");
            emitter.emitLine("const char *childClass;");
            emitter.emitLine("const char *childAsm;");
        }
        emitter.emitBlankLine();

        {
            auto s = emitter.enterStruct("SpecialRegInfo");
            emitter.emitLine("const char *name;");
            emitter.emitLine("uint32_t id;");
        }
        emitter.emitBlankLine();

        // Register entries table.
        emitter.emitLine("inline constexpr RegisterInfoEntry s_registerEntries[] =");
        {
            auto s = emitter.enterScope("{", "};");
            if (data.m_entries.empty())
            {
                emitter.emitLine("RegisterInfoEntry{ nullptr, nullptr, 0, 0, nullptr },");
            }
            else
            {
                for (const auto &e : data.m_entries)
                {
                    emitter.emitLine("RegisterInfoEntry{{ \"{}\", \"{}\", {}, {}, \"{}\" }},",
                                     e.m_bank, e.m_class, e.m_bitSize, e.m_hwEncoding, e.m_asmName);
                }
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_registerEntryCount = {};", data.m_entries.size());
        emitter.emitBlankLine();

        // Sub-register edges table.
        emitter.emitLine("inline constexpr SubRegEdge s_subRegEdges[] =");
        {
            auto s = emitter.enterScope("{", "};");
            if (data.m_edges.empty())
            {
                emitter.emitLine("SubRegEdge{ nullptr, nullptr, nullptr, nullptr },");
            }
            else
            {
                for (const auto &e : data.m_edges)
                {
                    emitter.emitLine("SubRegEdge{{ \"{}\", \"{}\", \"{}\", \"{}\" }},",
                                     e.m_parentClass, e.m_parentAsm, e.m_childClass, e.m_childAsm);
                }
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_subRegEdgeCount = {};", data.m_edges.size());
        emitter.emitBlankLine();

        // Special registers table.
        emitter.emitLine("inline constexpr SpecialRegInfo s_specialRegs[] =");
        {
            auto s = emitter.enterScope("{", "};");
            if (data.m_specials.empty())
            {
                emitter.emitLine("SpecialRegInfo{ nullptr, 0 },");
            }
            else
            {
                for (const auto &e : data.m_specials)
                {
                    emitter.emitLine("SpecialRegInfo{{ \"{}\", {} }},", e.m_name, e.m_id);
                }
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_specialRegCount = {};", data.m_specials.size());
        emitter.emitBlankLine();

        emitter.emitLine("inline const RegisterInfoEntry *getRegisterEntries() { return s_registerEntries; }");
        emitter.emitLine("inline std::size_t getRegisterEntryCount() { return s_registerEntryCount; }");
        emitter.emitLine("inline const SubRegEdge *getSubRegEdges() { return s_subRegEdges; }");
        emitter.emitLine("inline std::size_t getSubRegEdgeCount() { return s_subRegEdgeCount; }");
        emitter.emitLine("inline const SpecialRegInfo *getSpecialRegs() { return s_specialRegs; }");
        emitter.emitLine("inline std::size_t getSpecialRegCount() { return s_specialRegCount; }");
        emitter.emitBlankLine();

        emitter.emitLine("inline uint32_t getSpecialRegId(const char *name)");
        {
            auto s = emitter.enterScope();
            emitter.emitLine("for (std::size_t i = 0; i < s_specialRegCount; ++i)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("if (std::string_view(s_specialRegs[i].name) == name)");
                {
                    auto g = emitter.enterScope();
                    emitter.emitLine("return s_specialRegs[i].id;");
                }
            }
            emitter.emitLine("return ~uint32_t{0};");
        }
        emitter.emitBlankLine();

        emitter.emitComment("Constructs the register banks, classes, registers and aliasing described by the tables above.");
        emitter.emitLine("inline std::pmr::vector<MirRegisterBank *> initializeRegisterBanks(std::pmr::memory_resource *alloc)");
        {
            auto s = emitter.enterScope();
            emitter.emitLine("std::pmr::vector<MirRegisterBank *> banks(alloc);");
            emitter.emitLine("std::unordered_map<std::string_view, MirRegisterBank *> bankMap;");
            emitter.emitLine("std::unordered_map<std::string_view, MirRegisterClass *> classMap;");
            emitter.emitLine("std::pmr::polymorphic_allocator<MirRegisterBank> bankAlloc(alloc);");
            emitter.emitLine("std::pmr::polymorphic_allocator<MirRegisterClass> classAlloc(alloc);");
            emitter.emitBlankLine();

            emitter.emitLine("for (std::size_t i = 0; i < s_registerEntryCount; ++i)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("const auto &entry = s_registerEntries[i];");
                emitter.emitLine("if (bankMap.find(entry.bankName) == bankMap.end())");
                {
                    auto g = emitter.enterScope();
                    emitter.emitLine("auto *bank = bankAlloc.new_object<MirRegisterBank>(entry.bankName, alloc);");
                    emitter.emitLine("bankMap.emplace(entry.bankName, bank);");
                    emitter.emitLine("banks.push_back(bank);");
                }
            }
            emitter.emitBlankLine();

            emitter.emitLine("for (std::size_t i = 0; i < s_registerEntryCount; ++i)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("const auto &entry = s_registerEntries[i];");
                emitter.emitLine("if (classMap.find(entry.className) == classMap.end())");
                {
                    auto g = emitter.enterScope();
                    emitter.emitLine("auto *bank = bankMap.at(entry.bankName);");
                    emitter.emitLine("auto *cls = classAlloc.new_object<MirRegisterClass>(entry.className, bank, alloc);");
                    emitter.emitLine("bank->addClass(entry.className, cls);");
                    emitter.emitLine("classMap.emplace(entry.className, cls);");
                }
            }
            emitter.emitBlankLine();

            emitter.emitLine("for (std::size_t i = 0; i < s_registerEntryCount; ++i)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("const auto &entry = s_registerEntries[i];");
                emitter.emitLine("classMap.at(entry.className)->addRegister(entry.asmName, entry.bitSize, 0, {}, entry.hwEncoding);");
            }
            emitter.emitBlankLine();

            emitter.emitLine("for (std::size_t i = 0; i < s_subRegEdgeCount; ++i)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("const auto &edge = s_subRegEdges[i];");
                emitter.emitLine("auto *parent = classMap.at(edge.parentClass)->getReg(edge.parentAsm);");
                emitter.emitLine("auto *child = classMap.at(edge.childClass)->getReg(edge.childAsm);");
                emitter.emitLine("if (parent && child)");
                {
                    auto g = emitter.enterScope();
                    emitter.emitLine("parent->addSubPart(child);");
                }
            }
            emitter.emitBlankLine();
            emitter.emitLine("return banks;");
        }
    }
}

bool CppRegisterInfoGenerator::run()
{
    if (!validate())
    {
        return false;
    }

    const auto data = collectData(getSymbolTable(), m_targetName);
    if (data.m_entries.empty() && data.m_specials.empty())
    {
        trace("No register definitions found; emitting an empty register info header.");
    }

    const std::string baseName = std::format("{}RegisterInfo", sanitizeIdentifier(m_targetName, "Target"));
    const auto targetFilePath = resolveSingleFilePath(baseName + ".h");

    CppSourceEmitter emitter;
    emitHeader(emitter);

    if (!writeOutput(targetFilePath, emitter.view()))
    {
        return false;
    }

    trace("Synthesized {} register entries, {} sub-register edges and {} special registers into {}",
          data.m_entries.size(),
          data.m_edges.size(),
          data.m_specials.size(),
          targetFilePath.filename().string());

    return true;
}

bool GenerateRegisterInfo(DiagnosticCollector *collector,
                          SymbolTable *table,
                          std::filesystem::path outPath,
                          std::string targetName)
{
    CppRegisterInfoGenerator generator(collector, table, std::move(outPath), std::move(targetName));
    return generator.run();
}

} // namespace CodeGenerators
