#include "SemaPasses/CallingConvPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/CallingConvSymbols.h"
#include <unordered_set>

namespace
{
constexpr auto PassName = "Sema::CallingConvPass";

/**
 * Returns true when val is a strictly positive power of two (alignments and slot sizes).
 */
bool isPowerOfTwo(int64_t val) { return val > 0 && (val & (val - 1)) == 0; }
} // namespace

bool CallingConvPass::validateSingleCallingConv(DiagnosticCollector *collector,
                                                SymbolTable *table,
                                                const DSL::Ast::CallingConvDef::CallingConventionDecl &decl)
{
    collector->trace(PassName, "Running semantic validation for calling convention '{}'", decl.m_name.m_node);

    bool hasErrors = false;

    SourceReference *nameRef = decl.m_name.m_sourceRef;
    if (decl.m_name.m_node.empty())
    {
        collector->error(PassName, "Calling convention name must not be empty") << nameRef;
        hasErrors = true;
    }
    else if (table->getSymByName(decl.m_name.m_node) != nullptr)
    {
        collector->error(PassName, "Calling convention '{}': Duplicate symbol declaration", decl.m_name.m_node)
                << nameRef;
        hasErrors = true;
    }

    if (!validateStack(collector, decl.m_stack))
    {
        hasErrors = true;
    }

    if (!validateRegisters(collector, decl))
    {
        hasErrors = true;
    }

    if (!validateArguments(collector, decl.m_arguments))
    {
        hasErrors = true;
    }

    if (!validateReturns(collector, decl.m_returns))
    {
        hasErrors = true;
    }

    if (decl.m_varargs.has_value())
    {
        const auto &va = *decl.m_varargs;
        if (va.m_stackAlign.has_value() && !isPowerOfTwo(va.m_stackAlign->m_node))
        {
            collector->error(PassName,
                             "Varargs stack_align must be a positive power of two (got {})",
                             va.m_stackAlign->m_node)
                    << va.m_stackAlign->m_sourceRef;
            hasErrors = true;
        }
    }

    if (hasErrors)
    {
        return false;
    }

    Symbols::CallingConvSymbol symData{ .m_name = decl.m_name.m_node, .m_astNode = &decl };

    SymbolId id = table->declareSym(nameRef, SymbolType::CallingConv, std::move(symData), decl.m_name.m_node);
    if (id == InvalidSymbolId)
    {
        collector->error(PassName, "Failed to declare calling convention symbol '{}'", decl.m_name.m_node) << nameRef;
        return false;
    }

    collector->trace(PassName, "Successfully declared calling convention symbol '{}'", decl.m_name.m_node);
    return true;
}

bool CallingConvPass::run(DiagnosticCollector *collector,
                          SymbolTable *table,
                          DSL::Ast::CallingConvDef::CallingConventionDefFile *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    // A file that declares a single convention without a list uses itself as that convention.
    if (file->m_conventions.empty())
    {
        file->m_conventions.push_back(*file);
    }

    bool allOk = true;
    for (auto &decl : file->m_conventions)
    {
        if (!validateSingleCallingConv(collector, table, decl))
        {
            allOk = false;
        }
    }
    return allOk;
}

bool CallingConvPass::validateStack(DiagnosticCollector *collector, const DSL::Ast::CallingConvDef::StackDef &stack)
{
    bool valid = true;

    if (!isPowerOfTwo(stack.m_alignment.m_node))
    {
        collector->error(PassName, "Stack alignment must be a positive power of two (got {})", stack.m_alignment.m_node)
                << stack.m_alignment.m_sourceRef;
        valid = false;
    }

    if (stack.m_shadowSpace.m_node < 0)
    {
        collector->error(PassName, "Shadow space size cannot be negative (got {})", stack.m_shadowSpace.m_node)
                << stack.m_shadowSpace.m_sourceRef;
        valid = false;
    }

    if (stack.m_stackPointer.m_node.empty())
    {
        collector->error(PassName, "Stack pointer register (sp) must not be empty") << stack.m_stackPointer.m_sourceRef;
        valid = false;
    }

    if (stack.m_redZone.has_value() && stack.m_redZone->m_node < 0)
    {
        collector->error(PassName, "Red zone size cannot be negative (got {})", stack.m_redZone->m_node)
                << stack.m_redZone->m_sourceRef;
        valid = false;
    }

    return valid;
}

bool CallingConvPass::validateRegisters(DiagnosticCollector *collector,
                                        const DSL::Ast::CallingConvDef::CallingConventionDecl &file)
{
    bool valid = true;
    std::unordered_set<std::string_view> calleeSaved;
    std::unordered_set<std::string_view> callerSaved;

    for (const auto &reg : file.m_calleeSaved)
    {
        if (!calleeSaved.insert(reg.m_node).second)
        {
            collector->error(PassName, "Duplicate register '{}' in callee-saved preserve list", reg.m_node)
                    << reg.m_sourceRef;
            valid = false;
        }
    }

    for (const auto &reg : file.m_callerSaved)
    {
        if (!callerSaved.insert(reg.m_node).second)
        {
            collector->error(PassName, "Duplicate register '{}' in caller-saved preserve list", reg.m_node)
                    << reg.m_sourceRef;
            valid = false;
        }

        if (calleeSaved.contains(reg.m_node))
        {
            collector->error(PassName, "Register '{}' cannot be both callee-saved and caller-saved", reg.m_node)
                    << reg.m_sourceRef;
            valid = false;
        }
    }

    return valid;
}

bool CallingConvPass::validateArguments(DiagnosticCollector *collector,
                                        const DSL::Ast::CallingConvDef::ArgumentPassingDef &args)
{
    bool valid = true;

    for (size_t i = 0; i < args.m_unifiedSlots.size(); ++i)
    {
        const auto &slot = args.m_unifiedSlots[i];
        std::unordered_set<std::string_view> boundClasses;
        for (const auto &b : slot.m_bindings)
        {
            if (!boundClasses.insert(b.m_abiClass.m_node).second)
            {
                collector->error(PassName,
                                 "Unified slot {}: Duplicate binding for ABI class '{}'",
                                 i,
                                 b.m_abiClass.m_node)
                        << b.m_abiClass.m_sourceRef;
                valid = false;
            }
        }
    }

    for (const auto &rule : args.m_rules)
    {
        if (rule.m_abiClass.m_node.empty())
        {
            collector->error(PassName, "Argument pass rule ABI class name must not be empty")
                    << rule.m_abiClass.m_sourceRef;
            valid = false;
        }

        if (rule.m_fallback.has_value())
        {
            if (!isPowerOfTwo(rule.m_fallback->m_slotSize.m_node))
            {
                collector->error(PassName,
                                 "Fallback stack slot size for ABI class '{}' must be a positive power of two",
                                 rule.m_abiClass.m_node)
                        << rule.m_fallback->m_slotSize.m_sourceRef;
                valid = false;
            }
        }
    }

    if (args.m_defaultStackFallback.has_value())
    {
        if (!isPowerOfTwo(args.m_defaultStackFallback->m_slotSize.m_node))
        {
            collector->error(PassName, "Default argument stack fallback slot size must be a positive power of two")
                    << args.m_defaultStackFallback->m_slotSize.m_sourceRef;
            valid = false;
        }
    }

    return valid;
}

bool CallingConvPass::validateReturns(DiagnosticCollector *collector, const DSL::Ast::CallingConvDef::ReturnDef &rets)
{
    bool valid = true;

    if (rets.m_sret.has_value())
    {
        const auto &sret = *rets.m_sret;
        if (sret.m_pointerRegister.m_node.empty())
        {
            collector->error(PassName, "sret pointer register must not be empty") << sret.m_pointerRegister.m_sourceRef;
            valid = false;
        }
    }

    for (const auto &rule : rets.m_rules)
    {
        if (rule.m_abiClass.m_node.empty())
        {
            collector->error(PassName, "Return pass rule ABI class name must not be empty")
                    << rule.m_abiClass.m_sourceRef;
            valid = false;
        }
    }

    return valid;
}
