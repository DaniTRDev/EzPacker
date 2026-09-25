#include "SemaPasses/TargetDescPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/RegisterSymbols.h"
#include "Sema/Symbols/TargetDescSymbols.h"
#include "SemaPasses/PassDriver.h"

#include <unordered_map>
#include <unordered_set>

namespace
{
constexpr auto PassName = "Sema::TargetDescPass";

/**
 * Returns true when val is a strictly positive power of two (used for stack slot sizes).
 */
bool isPowerOfTwo(int64_t val) { return val > 0 && (val & (val - 1)) == 0; }
} // namespace

bool TargetDescPass::run(class DiagnosticCollector *collector,
                         class SymbolTable *table,
                         DSL::Ast::TargetDesc::TargetDescFile *file)
{
    if (!Sema::preparePass(collector, table, file, PassName))
    {
        return false;
    }

    bool hasErrors = false;
    const auto &name = file->m_name.m_node;

    if (name.empty())
    {
        collector->error(PassName, "Target name must not be empty.") << file->m_name.m_sourceRef;
        hasErrors = true;
    }
    else if (table->getSymByName(name) != nullptr)
    {
        collector->error(PassName, "Duplicate target descriptor symbol '{}'.", name) << file->m_name.m_sourceRef;
        hasErrors = true;
    }

    // Global uniqueness sets for register definitions.
    std::unordered_set<std::string_view> bankNames;
    std::unordered_set<std::string_view> classNames;
    std::unordered_set<uint32_t> allocatableEncodings;
    std::unordered_set<std::string_view> allRegNames;

    for (const auto &bank : file->m_registerBanks)
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
            allRegNames.insert(canonical);

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
                                             .m_target = name,
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
        const auto &spName = special.m_name.m_node;
        const int64_t id = special.m_id.m_node;

        if (spName.empty())
        {
            collector->error(PassName, "Special register must have a name.") << special.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        if (!specialNames.insert(spName).second)
        {
            collector->error(PassName, "Duplicate special register '{}'.", spName) << special.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        if (id < 0 || id > 0xFFFFFFFFLL)
        {
            collector->error(PassName, "Special register '{}' has invalid id {}.", spName, id)
                    << special.m_id.m_sourceRef;
            hasErrors = true;
            continue;
        }

        const uint32_t resolvedId = static_cast<uint32_t>(id);
        if (allocatableEncodings.contains(resolvedId))
        {
            collector->error(PassName,
                             "Special register '{}' id {} collides with an allocatable register encoding.",
                             spName,
                             resolvedId)
                    << special.m_id.m_sourceRef;
            hasErrors = true;
        }

        Symbols::SpecialRegisterSymbol symData{ .m_name = spName,
                                                .m_target = name,
                                                .m_id = resolvedId,
                                                .m_astNode = &special };
        if (table->declareSym(special.m_name.m_sourceRef, SymbolType::SpecialRegister, std::move(symData), spName) ==
            InvalidSymbolId)
        {
            collector->error(PassName, "Failed to register special register symbol '{}'.", spName)
                    << special.m_name.m_sourceRef;
            hasErrors = true;
        }
    }

    if (file->m_pointerSize.has_value() && file->m_pointerSize->m_node <= 0)
    {
        collector->error(PassName, "pointer_size must be positive (got {}).", file->m_pointerSize->m_node)
                << file->m_pointerSize->m_sourceRef;
        hasErrors = true;
    }

    if (file->m_stackSlot.has_value())
    {
        if (!isPowerOfTwo(file->m_stackSlot->m_node))
        {
            collector->error(PassName,
                             "stack_slot must be a positive power of two (got {}).",
                             file->m_stackSlot->m_node)
                    << file->m_stackSlot->m_sourceRef;
            hasErrors = true;
        }
    }

    if (file->mInstructionPointer.has_value())
    {
        const auto &ip = file->mInstructionPointer->m_node;
        if (ip.empty())
        {
            collector->error(PassName, "instruction_pointer must name a special register.")
                    << file->mInstructionPointer->m_sourceRef;
            hasErrors = true;
        }
        else if (!file->m_registerBanks.empty() || !file->m_specialRegs.empty())
        {
            if (!specialNames.contains(ip) && !allRegNames.contains(ip))
            {
                collector->error(PassName,
                                 "instruction_pointer '{}' does not match any declared special or physical register.",
                                 ip)
                        << file->mInstructionPointer->m_sourceRef;
                hasErrors = true;
            }
        }
    }

    if (file->mDefaultCallingConv.has_value() && file->mDefaultCallingConv->m_node.empty())
    {
        collector->error(PassName, "default_calling_conv must name a calling convention.")
                << file->mDefaultCallingConv->m_sourceRef;
        hasErrors = true;
    }

    if (file->mObjectFormats.empty())
    {
        collector->trace(PassName, "Target '{}' declares no object_formats.", name);
    }

    std::unordered_set<std::string_view> componentSlots;
    for (const auto &component : file->mComponents)
    {
        if (!componentSlots.insert(component.m_slot.m_node).second)
        {
            collector->error(PassName, "Duplicate component slot '{}'.", component.m_slot.m_node)
                    << component.m_slot.m_sourceRef;
            hasErrors = true;
        }
    }

    std::unordered_set<std::string_view> libcallNames;
    for (const auto &libcall : file->mLibcalls)
    {
        if (!libcallNames.insert(libcall.m_name.m_node).second)
        {
            collector->error(PassName, "Duplicate libcall id '{}'.", libcall.m_name.m_node)
                    << libcall.m_name.m_sourceRef;
            hasErrors = true;
        }
    }

    std::unordered_set<std::string_view> extensionNames;
    for (const auto &ext : file->m_extensions)
    {
        if (ext.m_name.m_node.empty())
        {
            collector->error(PassName, "Extension name must not be empty.")
                    << ext.m_name.m_sourceRef;
            hasErrors = true;
            continue;
        }

        if (!extensionNames.insert(ext.m_name.m_node).second)
        {
            collector->error(PassName, "Duplicate extension '{}'.", ext.m_name.m_node)
                    << ext.m_name.m_sourceRef;
            hasErrors = true;
        }
    }

    // Check that implied extensions exist and don't imply self
    for (const auto &ext : file->m_extensions)
    {
        for (const auto &implied : ext.m_implies)
        {
            if (implied.m_node == ext.m_name.m_node)
            {
                collector->error(PassName, "Extension '{}' cannot imply itself.", ext.m_name.m_node)
                        << implied.m_sourceRef;
                hasErrors = true;
            }
            else if (extensionNames.find(implied.m_node) == extensionNames.end())
            {
                collector->error(PassName, "Extension '{}' implies unknown extension '{}'.",
                                 ext.m_name.m_node, implied.m_node)
                        << implied.m_sourceRef;
                hasErrors = true;
            }
        }
    }

    if (hasErrors)
    {
        collector->error(PassName, "Target descriptor pass completed with errors.");
        return false;
    }

    Symbols::TargetDescSymbol symData{ .m_name = name, .m_astNode = file };
    if (table->declareSym(file->m_name.m_sourceRef, SymbolType::TargetDesc, std::move(symData), name) ==
        InvalidSymbolId)
    {
        collector->error(PassName, "Failed to register target descriptor symbol '{}'.", name)
                << file->m_name.m_sourceRef;
        return false;
    }

    collector->trace(PassName, "Registered target descriptor '{}'.", name);
    return true;
}
