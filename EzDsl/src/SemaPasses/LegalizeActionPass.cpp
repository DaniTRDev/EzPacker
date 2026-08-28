#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "Sema/Symbols/Symbols.h"

constexpr auto PassName = "Sema::LegalizeActionPass";

bool LegalizeActionPass::run(DiagnosticCollector *collector,
                             SymbolTable *table,
                             DSL::Ast::LegalizeActionDef::TargetLegalizeDef *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(PassName, "Running semantic validation for legalization actions");

    bool success = true;
    for (const auto &decl : file->m_instructionActions)
    {
        if (!processInstructionDecl(collector, table, decl))
        {
            success = false;
        }
    }

    return success;
}

bool LegalizeActionPass::processInstructionDecl(DiagnosticCollector *collector,
                                                SymbolTable *table,
                                                const DSL::Ast::LegalizeActionDef::InstructionLegalizeDecl &decl)
{
    const auto &instIdentifier = decl.m_instName;

    // 1. Verify that the target IR opcode is defined and is a valid IR Instruction
    Symbol *irSym = table->getSymByName(instIdentifier.m_node);
    if (!irSym || irSym->getType() != SymbolType::IrInstruction)
    {
        collector->error(PassName, "Unknown or undefined IR opcode '{}'", instIdentifier.m_node)
                << instIdentifier.m_sourceRef;
        return false;
    }

    // 2. Declare the legalization action symbol for this opcode
    Sema::Symbols::LegalizeActionSymbol actionSym{ .m_genericOpcode = instIdentifier.m_node,
                                                   .m_maxOperandIndex = 0,
                                                   .m_clauses = std::pmr::vector<Sema::Symbols::LegalizeClauseSymbol>{
                                                           table->getAllocator() } };

    SymbolId symId = table->declareSym(instIdentifier.m_sourceRef,
                                       SymbolFlags::IsDefined,
                                       SymbolType::LegalizeAction,
                                       std::move(actionSym),
                                       instIdentifier.m_node);

    if (symId == InvalidSymbolId)
    {
        collector->error(PassName, "Redefinition of legalization actions for opcode '{}'", instIdentifier.m_node)
                << instIdentifier.m_sourceRef;
        return false;
    }

    Symbol *registeredSymbol = table->getSymById(symId);
    auto *actionData = registeredSymbol ? registeredSymbol->getIf<Sema::Symbols::LegalizeActionSymbol>() : nullptr;
    if (!actionData)
    {
        return false;
    }

    bool success = true;
    size_t maxOperandIndex = 0;

    for (const auto &clause : decl.m_actions)
    {
        Sema::Symbols::LegalizeClauseSymbol clauseSym{
            .m_kind = clause.m_kind,
            .m_types = std::pmr::vector<Sema::Symbols::LegalizeConstraintSymbol>{ table->getAllocator() },
            .m_targetTypeId = std::nullopt,
            .m_libcallSymbol = std::nullopt,
            .m_customRules = std::nullopt
        };

        if (!processClause(collector, table, clause, instIdentifier.m_node, clauseSym, maxOperandIndex))
        {
            success = false;
            continue;
        }

        actionData->m_clauses.push_back(std::move(clauseSym));
    }

    // Records highest operand slot index constrained (determines table dimensionality: 1D, 2D, 3D)
    actionData->m_maxOperandIndex = maxOperandIndex;

    collector->trace(PassName,
                     "Registered {} legalization clauses for opcode '{}'",
                     actionData->m_clauses.size(),
                     instIdentifier.m_node);

    return success;
}

bool LegalizeActionPass::processClause(DiagnosticCollector *collector,
                                       SymbolTable *table,
                                       const DSL::Ast::LegalizeActionDef::LegalizeActionClause &clause,
                                       std::string_view instName,
                                       Sema::Symbols::LegalizeClauseSymbol &outClause,
                                       size_t &maxOperandIndex)
{
    bool success = true;

    // 1. Resolve and validate matched type constraints
    for (const auto &typeConstraint : clause.m_types)
    {
        Sema::Symbols::LegalizeConstraintSymbol constraintSym;
        if (!resolveConstraint(collector, table, typeConstraint, constraintSym))
        {
            success = false;
            continue;
        }

        if (constraintSym.m_operandIndex.has_value())
        {
            uint32_t operandIndex = constraintSym.m_operandIndex.value();
            if (operandIndex > maxOperandIndex)
            {
                maxOperandIndex = operandIndex;
            }
        }

        outClause.m_types.push_back(constraintSym);
    }

    // Non-CUSTOM actions must declare at least one type constraint
    if (outClause.m_types.empty() && clause.m_kind != DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom)
    {
        collector->error(PassName,
                         "Legalization clause for opcode '{}' must declare at least one type constraint",
                         instName);
        success = false;
    }

    // 2. Kind-specific target and symbol validation
    switch (clause.m_kind)
    {
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar:
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar:
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Bitcast:
        {
            if (clause.m_customRules.has_value())
            {
                collector->error(PassName, "Action does not accept custom rule targets");
                success = false;
            }

            if (clause.m_libcallSymbol.has_value())
            {
                collector->error(PassName, "Action does not accept a libcall symbol target")
                        << clause.m_libcallSymbol->m_sourceRef;
                success = false;
            }

            if (!clause.m_targetType.has_value())
            {
                collector->error(PassName, "Action requires a target type transformation ('>> TargetType')");
                return false;
            }

            const auto &targetTypeId = *clause.m_targetType;
            Symbol *targetSym = table->getSymByName(targetTypeId.m_node);
            if (!targetSym || targetSym->getType() != SymbolType::Type)
            {
                collector->error(PassName, "Undefined target type '{}' in legalization clause", targetTypeId.m_node)
                        << targetTypeId.m_sourceRef;
                return false;
            }

            outClause.m_targetTypeId = targetSym->getId();
            const auto *targetData = targetSym->getIf<Sema::Symbols::TypeSymbol>();

            // Bitwidth verification against source types
            if (targetData && targetData->m_bitWidth > 0)
            {
                for (const auto &constraintSym : outClause.m_types)
                {
                    Symbol *srcSym = table->getSymById(constraintSym.m_typeId);
                    const auto *srcData = srcSym ? srcSym->getIf<Sema::Symbols::TypeSymbol>() : nullptr;

                    if (!srcData || srcData->m_bitWidth == 0)
                    {
                        continue;
                    }

                    if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar &&
                        targetData->m_bitWidth <= srcData->m_bitWidth)
                    {
                        collector->error(
                                PassName,
                                "WIDENS target type '{}' ({} bits) must be larger than source type '{}' ({} bits)",
                                targetData->m_name,
                                targetData->m_bitWidth,
                                srcData->m_name,
                                srcData->m_bitWidth)
                                << targetTypeId.m_sourceRef;
                        success = false;
                    }
                    else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar &&
                             targetData->m_bitWidth >= srcData->m_bitWidth)
                    {
                        collector->error(
                                PassName,
                                "NARROWS target type '{}' ({} bits) must be smaller than source type '{}' ({} bits)",
                                targetData->m_name,
                                targetData->m_bitWidth,
                                srcData->m_name,
                                srcData->m_bitWidth)
                                << targetTypeId.m_sourceRef;
                        success = false;
                    }
                    else if (clause.m_kind == DSL::Ast::LegalizeActionDef::LegalizeActionKind::Bitcast &&
                             targetData->m_bitWidth != srcData->m_bitWidth)
                    {
                        collector->error(
                                PassName,
                                "BITCAST target type '{}' ({} bits) must match source type '{}' ({} bits) width",
                                targetData->m_name,
                                targetData->m_bitWidth,
                                srcData->m_name,
                                srcData->m_bitWidth)
                                << targetTypeId.m_sourceRef;
                        success = false;
                    }
                }
            }
            break;
        }

        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Libcall:
        {
            if (clause.m_customRules.has_value())
            {
                collector->error(PassName, "LIBCALL action does not accept custom rule targets");
                success = false;
            }

            if (clause.m_targetType.has_value())
            {
                collector->error(PassName, "LIBCALL action does not accept a target type")
                        << clause.m_targetType->m_sourceRef;
                success = false;
            }

            if (!clause.m_libcallSymbol.has_value() || clause.m_libcallSymbol->m_node.empty())
            {
                collector->error(PassName,
                                 "LIBCALL action requires a runtime library function symbol ('>> \"symbol\"')");
                return false;
            }

            outClause.m_libcallSymbol = clause.m_libcallSymbol->m_node;
            break;
        }

        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Custom:
        {
            if (clause.m_targetType.has_value())
            {
                collector->error(PassName, "CUSTOM clause does not accept a target type transformation")
                        << clause.m_targetType->m_sourceRef;
                success = false;
            }

            if (clause.m_libcallSymbol.has_value())
            {
                collector->error(PassName, "CUSTOM clause does not accept a libcall symbol")
                        << clause.m_libcallSymbol->m_sourceRef;
                success = false;
            }

            if (!clause.m_customRules.has_value() || clause.m_customRules->empty())
            {
                collector->error(PassName, "CUSTOM clause requires at least one custom rule ('>> RuleName')");
                success = false;
            }
            else
            {
                const auto &rules = *clause.m_customRules;
                std::pmr::vector<SymbolId> resolvedRules(table->getAllocator());
                resolvedRules.reserve(rules.size());

                for (const auto &rule : rules)
                {
                    Symbol *sym = table->getSymByName(rule.m_node);
                    if (!sym || sym->getType() != SymbolType::LegalizeRule)
                    {
                        collector->error(PassName, "CUSTOM clause rule '{}' is not defined", rule.m_node)
                                << rule.m_sourceRef;
                        success = false;
                        continue;
                    }

                    resolvedRules.push_back(sym->getId());
                }

                outClause.m_customRules = std::move(resolvedRules);
            }
            break;
        }

        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal:
        case DSL::Ast::LegalizeActionDef::LegalizeActionKind::Unsupported:
        {
            if (clause.m_targetType.has_value())
            {
                collector->error(PassName, "Clause does not accept a target type") << clause.m_targetType->m_sourceRef;
                success = false;
            }

            if (clause.m_libcallSymbol.has_value())
            {
                collector->error(PassName, "Clause does not accept a libcall symbol")
                        << clause.m_libcallSymbol->m_sourceRef;
                success = false;
            }

            if (clause.m_customRules.has_value())
            {
                collector->error(PassName, "Clause does not accept custom rule targets");
                success = false;
            }
            break;
        }
    }

    return success;
}

bool LegalizeActionPass::resolveConstraint(DiagnosticCollector *collector,
                                           SymbolTable *table,
                                           const DSL::Ast::LegalizeActionDef::TypeConstraint &constraint,
                                           Sema::Symbols::LegalizeConstraintSymbol &outConstraint)
{
    const auto &typeName = constraint.m_type;
    Symbol *typeSym = table->getSymByName(typeName.m_node);

    if (!typeSym || typeSym->getType() != SymbolType::Type)
    {
        collector->error(PassName, "Unknown type '{}' in type constraint", typeName.m_node) << typeName.m_sourceRef;
        return false;
    }

    outConstraint.m_typeId = typeSym->getId();

    if (constraint.m_operandIndex.has_value())
    {
        const auto &indexLit = *constraint.m_operandIndex;
        if (indexLit.m_node < 0)
        {
            collector->error(PassName, "Type index cannot be negative (got {})", indexLit.m_node)
                    << indexLit.m_sourceRef;
            return false;
        }
        outConstraint.m_operandIndex = static_cast<uint32_t>(indexLit.m_node);
    }
    else
    {
        outConstraint.m_operandIndex = std::nullopt;
    }

    return true;
}