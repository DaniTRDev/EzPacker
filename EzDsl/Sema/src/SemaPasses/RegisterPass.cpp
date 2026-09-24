#include "SemaPasses/RegisterPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/RegisterSymbols.h"
#include "SemaPasses/PassDriver.h"

#include <unordered_map>
#include <unordered_set>

namespace
{
constexpr auto PassName = "Sema::RegisterPass";
} // namespace

bool RegisterPass::run(class DiagnosticCollector *collector,
                       class SymbolTable *table,
                       DSL::Ast::RegisterDef::RegisterFile *file)
{
    if (!Sema::preparePass(collector, table, file, PassName))
    {
        return false;
    }

    bool hasErrors = false;

    // Global (whole-file) uniqueness sets.
    std::unordered_set<std::string_view> bankNames;
    std::unordered_set<std::string_view> classNames;
    std::unordered_set<uint32_t> allocatableEncodings;

    for (auto &bank : file->m_banks)
    {
        const auto &bankName = bank.m_name.m_node;

        if (bankName.empty())
        {
            collector->error(PassName, "Register bank name must not be empty.") << bank.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        if (!bankNames.insert(bankName).second)
        {
            collector->error(PassName, "Duplicate register bank '{}'.", bankName) << bank.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        // Classes must be declared before they are referenced by names/edges.
        std::unordered_map<std::string_view, uint32_t> bankClasses;
        for (const auto &cls : bank.m_classes)
        {
            const auto &className = cls.m_name.m_node;
            const int64_t bits = cls.m_bitSize.m_node;

            if (className.empty())
            {
                collector->error(PassName, "Register class in bank '{}' must have a name.", bankName)
                        << cls.m_name.m_sourceRef;
                hasErrors = true;
                continue;
            }

            if (!bankClasses.emplace(className, static_cast<uint32_t>(bits)).second)
            {
                collector->error(PassName, "Duplicate register class '{}' in bank '{}'.", className, bankName)
                        << cls.m_name.m_sourceRef;
                hasErrors = true;
                continue;
            }

            if (!classNames.insert(className).second)
            {
                collector->error(PassName, "Register class '{}' is already declared in another bank.", className)
                        << cls.m_name.m_sourceRef;
                hasErrors = true;
                continue;
            }

            if (bits <= 0 || bits > 4096)
            {
                collector->error(PassName,
                                 "Register class '{}' has invalid bit size {} (must be in 1..4096).",
                                 className,
                                 bits)
                        << cls.m_bitSize.m_sourceRef;
                hasErrors = true;
                continue;
            }

            Symbols::RegisterClassSymbol symData{ .m_name = className,
                                                  .m_bankName = bankName,
                                                  .m_bitSize = static_cast<uint32_t>(bits),
                                                  .m_astNode = &cls };
            if (table->declareSym(cls.m_name.m_sourceRef, SymbolType::RegisterClass, std::move(symData), className) ==
                InvalidSymbolId)
            {
                collector->error(PassName, "Failed to register class symbol '{}'.", className)
                        << cls.m_name.m_sourceRef;
                hasErrors = true;
            }
        }

        // Sub-register edges must reference classes of this bank with a decreasing width.
        for (const auto &edge : bank.m_subRegisterEdges)
        {
            const auto &wide = edge.m_wideClass.m_node;
            const auto &narrow = edge.m_narrowClass.m_node;

            auto wideIt = bankClasses.find(wide);
            auto narrowIt = bankClasses.find(narrow);

            if (wideIt == bankClasses.end())
            {
                collector->error(PassName,
                                 "Sub-register edge references undeclared class '{}' in bank '{}'.",
                                 wide,
                                 bankName)
                        << edge.m_wideClass.m_sourceRef;
                hasErrors = true;
            }
            if (narrowIt == bankClasses.end())
            {
                collector->error(PassName,
                                 "Sub-register edge references undeclared class '{}' in bank '{}'.",
                                 narrow,
                                 bankName)
                        << edge.m_narrowClass.m_sourceRef;
                hasErrors = true;
            }
            if (wideIt != bankClasses.end() && narrowIt != bankClasses.end() && wideIt->second <= narrowIt->second)
            {
                collector->error(PassName,
                                 "Sub-register edge '{} <: {}' must map a wider class onto a narrower one "
                                 "({} bits <= {} bits).",
                                 wide,
                                 narrow,
                                 wideIt->second,
                                 narrowIt->second)
                        << edge.m_wideClass.m_sourceRef;
                hasErrors = true;
            }
        }

        // Physical registers: canonical name and hardware encoding unique within the bank.
        std::unordered_set<std::string_view> regNames;
        std::unordered_set<uint32_t> encodings;
        for (const auto &reg : bank.m_registers)
        {
            const auto &canonical = reg.m_canonicalName.m_node;
            const int64_t enc = reg.m_encoding.m_node;

            if (canonical.empty())
            {
                collector->error(PassName, "Physical register in bank '{}' must have a canonical name.", bankName)
                        << reg.m_canonicalName.m_sourceRef;
                hasErrors = true;
                continue;
            }

            if (!regNames.insert(canonical).second)
            {
                collector->error(PassName, "Duplicate register '{}' in bank '{}'.", canonical, bankName)
                        << reg.m_canonicalName.m_sourceRef;
                hasErrors = true;
            }

            if (enc < 0 || enc > 0xFFFFFFFFLL)
            {
                collector->error(PassName, "Register '{}' has invalid hardware encoding {}.", canonical, enc)
                        << reg.m_encoding.m_sourceRef;
                hasErrors = true;
                continue;
            }

            const uint32_t encoding = static_cast<uint32_t>(enc);
            if (!encodings.insert(encoding).second)
            {
                collector->error(PassName, "Duplicate hardware encoding {} in bank '{}'.", encoding, bankName)
                        << reg.m_encoding.m_sourceRef;
                hasErrors = true;
            }
            allocatableEncodings.insert(encoding);

            // Each name binding must reference a declared class of this bank, once per class.
            std::unordered_set<std::string_view> boundClasses;
            for (const auto &binding : reg.m_names)
            {
                const auto &className = binding.m_className.m_node;
                const auto &asmName = binding.m_asmName.m_node;

                if (asmName.empty())
                {
                    collector->error(PassName, "Register '{}' has an empty assembly-name binding.", canonical)
                            << binding.m_asmName.m_sourceRef;
                    hasErrors = true;
                    continue;
                }

                if (!bankClasses.contains(className))
                {
                    collector->error(PassName,
                                     "Register '{}' binds name '{}' to undeclared class '{}' in bank '{}'.",
                                     canonical,
                                     asmName,
                                     className,
                                     bankName)
                            << binding.m_className.m_sourceRef;
                    hasErrors = true;
                    continue;
                }

                if (!boundClasses.insert(className).second)
                {
                    collector->error(PassName,
                                     "Register '{}' declares class '{}' more than once.",
                                     canonical,
                                     className)
                            << binding.m_className.m_sourceRef;
                    hasErrors = true;
                }
            }

            Symbols::RegisterSymbol symData{ .m_name = canonical,
                                             .m_bankName = bankName,
                                             .m_hwEncoding = encoding,
                                             .m_astNode = &reg };
            if (table->declareSym(reg.m_canonicalName.m_sourceRef,
                                  SymbolType::Register,
                                  std::move(symData),
                                  canonical) == InvalidSymbolId)
            {
                collector->error(PassName, "Failed to register physical register symbol '{}'.", canonical)
                        << reg.m_canonicalName.m_sourceRef;
                hasErrors = true;
            }
        }

        Symbols::RegisterBankSymbol bankSym{ .m_name = bankName,
                                             .m_target = file->m_target.m_node,
                                             .m_astNode = &bank };
        if (table->declareSym(bank.m_name.m_sourceRef, SymbolType::RegisterBank, std::move(bankSym), bankName) ==
            InvalidSymbolId)
        {
            collector->error(PassName, "Failed to register bank symbol '{}'.", bankName) << bank.m_name.m_sourceRef;
            hasErrors = true;
        }
    }

    // Special (pseudo) registers: unique names, and ids must not collide with allocatable encodings.
    std::unordered_set<std::string_view> specialNames;
    for (const auto &special : file->m_specialRegs)
    {
        const auto &name = special.m_name.m_node;
        const int64_t id = special.m_id.m_node;

        if (name.empty())
        {
            collector->error(PassName, "Special register must have a name.") << special.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        if (!specialNames.insert(name).second)
        {
            collector->error(PassName, "Duplicate special register '{}'.", name) << special.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        if (id < 0 || id > 0xFFFFFFFFLL)
        {
            collector->error(PassName, "Special register '{}' has invalid id {}.", name, id)
                    << special.m_id.m_sourceRef;
            hasErrors = true;
            continue;
        }

        const uint32_t resolvedId = static_cast<uint32_t>(id);
        if (allocatableEncodings.contains(resolvedId))
        {
            collector->error(PassName,
                             "Special register '{}' id {} collides with an allocatable register encoding.",
                             name,
                             resolvedId)
                    << special.m_id.m_sourceRef;
            hasErrors = true;
        }

        Symbols::SpecialRegisterSymbol symData{ .m_name = name,
                                                .m_target = file->m_target.m_node,
                                                .m_id = resolvedId,
                                                .m_astNode = &special };
        if (table->declareSym(special.m_name.m_sourceRef, SymbolType::SpecialRegister, std::move(symData), name) ==
            InvalidSymbolId)
        {
            collector->error(PassName, "Failed to register special register symbol '{}'.", name)
                    << special.m_name.m_sourceRef;
            hasErrors = true;
        }
    }

    // Root symbol holding the whole register file for code generation.
    Symbols::RegisterFileSymbol fileSym{ .m_target = file->m_target.m_node, .m_astNode = file };
    if (table->declareSym(file->m_target.m_sourceRef,
                          SymbolType::RegisterFile,
                          std::move(fileSym),
                          file->m_target.m_node) == InvalidSymbolId)
    {
        collector->error(PassName, "Failed to register register-file symbol '{}'.", file->m_target.m_node)
                << file->m_target.m_sourceRef;
        hasErrors = true;
    }

    if (hasErrors)
    {
        collector->error(PassName, "Register definition pass completed with errors.");
        return false;
    }

    collector->trace(PassName,
                     "Registered {} banks, {} special registers for target '{}'.",
                     file->m_banks.size(),
                     file->m_specialRegs.size(),
                     file->m_target.m_node);
    return true;
}
