#include "SemaPasses/TargetDescPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetDescSymbols.h"
#include "SemaPasses/PassDriver.h"

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

    if (file->mInstructionPointer.has_value() && file->mInstructionPointer->m_node.empty())
    {
        collector->error(PassName, "instruction_pointer must name a special register.")
                << file->mInstructionPointer->m_sourceRef;
        hasErrors = true;
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
