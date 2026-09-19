#include "SemaPasses/InstructionSelectPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/InstructionSelectSymbols.h"
#include <unordered_set>

namespace
{
constexpr auto PassName = "Sema::InstructionSelectPass";

/**
 * Recursively gathers every variable name bound by a match tree (SSA registers, immediate
 * symbols, address-mode arguments, and nested trees) so later clauses can be checked against them.
 */
void collectBoundVars(const DSL::Ast::InstructionSelectDef::PatternTree &tree,
                      std::unordered_set<std::string_view> &boundVars)
{
    for (const auto &op : tree.m_operands)
    {
        switch (op.m_kind)
        {
            case DSL::Ast::InstructionSelectDef::PatternOperand::Kind::SsaRegister:
            case DSL::Ast::InstructionSelectDef::PatternOperand::Kind::ImmediateSymbol:
                boundVars.insert(op.m_name.m_node);
                break;
            case DSL::Ast::InstructionSelectDef::PatternOperand::Kind::AddrModeRef:
                for (const auto &arg : op.m_addrModeArgs)
                {
                    boundVars.insert(arg.m_node);
                }
                break;
            case DSL::Ast::InstructionSelectDef::PatternOperand::Kind::NestedTree:
                if (op.m_nestedTree)
                {
                    collectBoundVars(*op.m_nestedTree, boundVars);
                }
                break;
            case DSL::Ast::InstructionSelectDef::PatternOperand::Kind::ImmediateLiteral:
                break;
        }
    }
}

} // namespace

bool InstructionSelectPass::run(DiagnosticCollector *collector,
                                SymbolTable *table,
                                DSL::Ast::InstructionSelectDef::InstructionSelectFile *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(PassName,
                     "Running semantic validation for {} addressing modes and {} selection patterns",
                     file->m_addressingModes.size(),
                     file->m_patterns.size());

    bool hasErrors = false;

    for (const auto &mode : file->m_addressingModes)
    {
        if (!validateAddrMode(collector, table, mode))
        {
            hasErrors = true;
        }
    }

    for (const auto &pattern : file->m_patterns)
    {
        if (!validatePattern(collector, table, pattern))
        {
            hasErrors = true;
        }
    }

    return !hasErrors;
}

bool InstructionSelectPass::validateAddrMode(DiagnosticCollector *collector,
                                             SymbolTable *table,
                                             const DSL::Ast::InstructionSelectDef::AddrModeDecl &mode)
{
    SourceReference *ref = mode.m_modeName.m_sourceRef;

    if (table->getSymByName(mode.m_modeName.m_node) != nullptr)
    {
        collector->error(PassName, "Addressing mode '{}': Duplicate symbol declaration", mode.m_modeName.m_node) << ref;
        return false;
    }

    std::unordered_set<std::string_view> paramNames;
    for (const auto &param : mode.m_params)
    {
        if (!paramNames.insert(param.m_name.m_node).second)
        {
            collector->error(PassName,
                             "Addressing mode '{}': Duplicate parameter name '{}'",
                             mode.m_modeName.m_node,
                             param.m_name.m_node)
                    << param.m_name.m_sourceRef;
            return false;
        }
    }

    Symbols::AddrModeSymbol symData{ .m_name = mode.m_modeName.m_node, .m_astNode = &mode };

    SymbolId id = table->declareSym(ref, SymbolType::AddressingMode, std::move(symData), mode.m_modeName.m_node);
    if (id == InvalidSymbolId)
    {
        collector->error(PassName, "Failed to declare addressing mode '{}'", mode.m_modeName.m_node) << ref;
        return false;
    }

    collector->trace(PassName, "Declared addressing mode '{}'", mode.m_modeName.m_node);
    return true;
}

bool InstructionSelectPass::validatePattern(DiagnosticCollector *collector,
                                            SymbolTable *table,
                                            const DSL::Ast::InstructionSelectDef::SelectionPattern &pattern)
{
    SourceReference *ref = pattern.m_name.m_sourceRef;

    if (table->getSymByName(pattern.m_name.m_node) != nullptr)
    {
        collector->error(PassName, "Selection pattern '{}': Duplicate symbol declaration", pattern.m_name.m_node)
                << ref;
        return false;
    }

    std::unordered_set<std::string_view> boundVars;
    collectBoundVars(pattern.m_matchTree, boundVars);

    // Validate when clauses
    for (const auto &when : pattern.m_whenClauses)
    {
        for (const auto &arg : when.m_args)
        {
            if (boundVars.find(arg.m_node) == boundVars.end())
            {
                collector->error(PassName,
                                 "Pattern '{}': 'when' clause references unbound variable '${}'",
                                 pattern.m_name.m_node,
                                 arg.m_node)
                        << arg.m_sourceRef;
                return false;
            }
        }
    }

    // Validate select clauses
    for (const auto &inst : pattern.m_selectClauses)
    {
        for (const auto &op : inst.m_operands)
        {
            if (op.m_kind == DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::BoundVar ||
                op.m_kind == DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::ClassBoundVar)
            {
                if (boundVars.find(op.m_name.m_node) == boundVars.end())
                {
                    collector->error(PassName,
                                     "Pattern '{}': 'select' clause references unbound variable '${}'",
                                     pattern.m_name.m_node,
                                     op.m_name.m_node)
                            << op.m_name.m_sourceRef;
                    return false;
                }
            }
            else if (op.m_kind == DSL::Ast::InstructionSelectDef::TargetEmitOperand::Kind::AddrModeMem)
            {
                for (const auto &memOp : op.m_memOperands)
                {
                    if (boundVars.find(memOp.m_node) == boundVars.end())
                    {
                        collector->error(PassName,
                                         "Pattern '{}': Memory operand references unbound variable '${}'",
                                         pattern.m_name.m_node,
                                         memOp.m_node)
                                << memOp.m_sourceRef;
                        return false;
                    }
                }
            }
        }
    }

    Symbols::SelectionPatternSymbol symData{ .m_name = pattern.m_name.m_node,
                                             .m_cost = pattern.m_cost,
                                             .m_astNode = &pattern };

    SymbolId id = table->declareSym(ref, SymbolType::SelectionPattern, std::move(symData), pattern.m_name.m_node);
    if (id == InvalidSymbolId)
    {
        collector->error(PassName, "Failed to declare selection pattern '{}'", pattern.m_name.m_node) << ref;
        return false;
    }

    collector->trace(PassName, "Declared selection pattern '{}'", pattern.m_name.m_node);
    return true;
}
