#include "Ast/TypeDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "SemaPasses/PassDriver.h"
#include "Sema/Symbol.h"
#include "SourceManager/GenericSourceManager.h"

#include <algorithm>

constexpr auto PassName = "Sema::LegalizeActionPass";

namespace
{
using namespace DSL::Ast::LegalizeActionDef;

/**
 * Verifies that a transformation target type satisfies the width relation implied by its kind
 * (wider for WIDENS, narrower for NARROWS, equal for BITCAST).
 */
bool validateBitwidths(DiagnosticCollector *collector,
                       SymbolTable *table,
                       LegalizeActionKind kind,
                       const Symbols::TypeSymbol *targetData,
                       const std::pmr::vector<Symbols::LegalizeActionConstraintSymbol> &types,
                       SourceReference *sourceRef)
{
    if (!targetData || targetData->m_bitWidth == 0)
    {
        return true;
    }

    bool success = true;
    for (const auto &constraintSym : types)
    {
        Symbol *srcSym = table->getSymById(constraintSym.m_typeId);
        const auto *srcData = srcSym ? srcSym->getIf<Symbols::TypeSymbol>() : nullptr;
        if (!srcData || srcData->m_bitWidth == 0)
        {
            continue;
        }

        bool violation = false;
        std::string_view relationText;

        switch (kind)
        {
            case LegalizeActionKind::WidenScalar:
                violation = (targetData->m_bitWidth <= srcData->m_bitWidth);
                relationText = "larger than";
                break;
            case LegalizeActionKind::NarrowScalar:
                violation = (targetData->m_bitWidth >= srcData->m_bitWidth);
                relationText = "smaller than";
                break;
            case LegalizeActionKind::Bitcast:
                violation = (targetData->m_bitWidth != srcData->m_bitWidth);
                relationText = "equal to";
                break;
            default:
                break;
        }

        if (violation)
        {
            collector->error(PassName,
                             "Target type '{}' ({} bits) must be {} source type '{}' ({} bits)",
                             targetData->m_name,
                             targetData->m_bitWidth,
                             relationText,
                             srcData->m_name,
                             srcData->m_bitWidth)
                    << sourceRef;
            success = false;
        }
    }
    return success;
}

/**
 * Expands a list of per-operand constraint options into every combination (Cartesian product),
 * used for clauses that constrain an indexed type set on multiple operands.
 */
void generateCartesianProduct(const std::vector<std::vector<Symbols::LegalizeActionConstraintSymbol>> &lists,
                              size_t depth,
                              std::vector<Symbols::LegalizeActionConstraintSymbol> &current,
                              std::vector<std::vector<Symbols::LegalizeActionConstraintSymbol>> &result)
{
    if (depth == lists.size())
    {
        result.push_back(current);
        return;
    }
    for (const auto &item : lists[depth])
    {
        current.push_back(item);
        generateCartesianProduct(lists, depth + 1, current, result);
        current.pop_back();
    }
}
} // namespace

bool LegalizeActionPass::run(DiagnosticCollector *collector,
                             SymbolTable *table,
                             DSL::Ast::LegalizeActionDef::LegalizeActionFile *file)
{
    if (!Sema::preparePass(collector, table, file, PassName))
    {
        return false;
    }

    if (file->m_targetName.has_value())
    {
        collector->trace(PassName, "Running legalization validation for target '{}'", file->m_targetName->m_node);
    }
    else
    {
        collector->trace(PassName, "Running semantic validation for legalization actions");
    }

    bool success = true;

    // 1. Process type_set declarations
    for (const auto &typeSet : file->m_typeSets)
    {
        if (!processTypeSetDecl(collector, table, typeSet))
        {
            success = false;
        }
    }

    // 2. Process group declarations
    for (const auto &group : file->m_groups)
    {
        for (const auto &instId : group.m_instructions)
        {
            DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl synthDecl;
            synthDecl.m_instName = instId;
            synthDecl.m_clampClause = group.m_clampClause;
            synthDecl.m_actionClauses = group.m_actionClauses;

            if (!processInstructionDecl(collector, table, synthDecl))
            {
                success = false;
            }
        }
    }

    // 3. Process direct action declarations
    for (const auto &decl : file->m_legalizeInstrDecls)
    {
        if (!processInstructionDecl(collector, table, decl))
        {
            success = false;
        }
    }

    return success;
}

bool LegalizeActionPass::processTypeSetDecl(DiagnosticCollector *collector,
                                            SymbolTable *table,
                                            const DSL::Ast::LegalizeActionDef::TypeSetDecl &decl)
{
    std::pmr::vector<SymbolId> resolvedTypeIds(table->getAllocator());
    bool valid = true;

    for (const auto &typeId : decl.m_types)
    {
        Symbol *typeSym = table->getSymByName(typeId.m_node);
        if (!typeSym || typeSym->getType() != SymbolType::Type)
        {
            collector->error(PassName, "Unknown type '{}' in type_set '{}'", typeId.m_node, decl.m_name.m_node)
                    << typeId.m_sourceRef;
            valid = false;
            continue;
        }
        resolvedTypeIds.push_back(typeSym->getId());
    }

    if (!valid)
    {
        return false;
    }

    Symbols::TypeSetSymbol symData{ .m_name = decl.m_name.m_node, .m_typeIds = std::move(resolvedTypeIds) };
    SymbolId symId =
            table->declareSym(decl.m_name.m_sourceRef, SymbolType::TypeSet, std::move(symData), decl.m_name.m_node);

    if (symId == InvalidSymbolId)
    {
        collector->error(PassName, "Redefinition of type_set '{}'", decl.m_name.m_node) << decl.m_name.m_sourceRef;
        return false;
    }

    collector->trace(PassName, "Registered type_set '{}' with {} types", decl.m_name.m_node, decl.m_types.size());
    return true;
}

bool LegalizeActionPass::applyClampScalar(DiagnosticCollector *collector,
                                          SymbolTable *table,
                                          const DSL::Ast::LegalizeActionDef::ClampScalarClause &clamp,
                                          std::string_view instName,
                                          Symbols::LegalizeActionSymbol &actionData)
{
    Symbol *minSym = table->getSymByName(clamp.m_minType.m_node);
    Symbol *maxSym = table->getSymByName(clamp.m_maxType.m_node);

    if (!minSym || minSym->getType() != SymbolType::Type)
    {
        collector->error(PassName,
                         "Unknown minimum type '{}' in CLAMP_SCALAR for opcode '{}'",
                         clamp.m_minType.m_node,
                         instName)
                << clamp.m_minType.m_sourceRef;
        return false;
    }

    if (!maxSym || maxSym->getType() != SymbolType::Type)
    {
        collector->error(PassName,
                         "Unknown maximum type '{}' in CLAMP_SCALAR for opcode '{}'",
                         clamp.m_maxType.m_node,
                         instName)
                << clamp.m_maxType.m_sourceRef;
        return false;
    }

    const auto *minData = minSym->getIf<Symbols::TypeSymbol>();
    const auto *maxData = maxSym->getIf<Symbols::TypeSymbol>();

    if (!minData || minData->m_kind != DSL::Ast::TypeDef::TypeKind::Integer)
    {
        collector->error(PassName, "Minimum type '{}' in CLAMP_SCALAR must be an integer type", clamp.m_minType.m_node)
                << clamp.m_minType.m_sourceRef;
        return false;
    }

    if (!maxData || maxData->m_kind != DSL::Ast::TypeDef::TypeKind::Integer)
    {
        collector->error(PassName, "Maximum type '{}' in CLAMP_SCALAR must be an integer type", clamp.m_maxType.m_node)
                << clamp.m_maxType.m_sourceRef;
        return false;
    }

    if (minData->m_bitWidth > maxData->m_bitWidth)
    {
        collector->error(PassName,
                         "Clamp min type '{}' ({} bits) cannot be larger than max type '{}' ({} bits)",
                         clamp.m_minType.m_node,
                         minData->m_bitWidth,
                         clamp.m_maxType.m_node,
                         maxData->m_bitWidth)
                << clamp.m_minType.m_sourceRef;
        return false;
    }

    // Minimal view of declared integer types used to synthesize clamp expansion clauses.
    struct IntTypeInfo
    {
        SymbolId id;       // Symbol id of the integer type.
        uint32_t bitWidth; // Width in bits.
    };
    std::vector<IntTypeInfo> intTypes;
    for (Symbol *sym : table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::Type)
        {
            if (const auto *td = sym->getIf<Symbols::TypeSymbol>())
            {
                if (td->m_kind == DSL::Ast::TypeDef::TypeKind::Integer && td->m_bitWidth > 0)
                {
                    intTypes.push_back({ sym->getId(), td->m_bitWidth });
                }
            }
        }
    }

    std::sort(intTypes.begin(), intTypes.end(), [](const auto &a, const auto &b) { return a.bitWidth < b.bitWidth; });

    for (const auto &t : intTypes)
    {
        if (t.bitWidth < minData->m_bitWidth)
        {
            Symbols::LegalizeActionClauseSymbol widenClause{
                .m_kind = DSL::Ast::LegalizeActionDef::LegalizeActionKind::WidenScalar,
                .m_types = std::pmr::vector<Symbols::LegalizeActionConstraintSymbol>{ table->getAllocator() },
                .m_targetTypeId = minSym->getId(),
                .m_libcallSymbol = std::nullopt,
                .m_lowerHandler = std::nullopt,
                .m_customRules = std::nullopt
            };
            widenClause.m_types.push_back({ .m_typeId = t.id, .m_operandIndex = std::nullopt });
            actionData.m_clauses.push_back(std::move(widenClause));
        }
        else if (t.bitWidth <= maxData->m_bitWidth)
        {
            Symbols::LegalizeActionClauseSymbol legalClause{
                .m_kind = DSL::Ast::LegalizeActionDef::LegalizeActionKind::Legal,
                .m_types = std::pmr::vector<Symbols::LegalizeActionConstraintSymbol>{ table->getAllocator() },
                .m_targetTypeId = std::nullopt,
                .m_libcallSymbol = std::nullopt,
                .m_lowerHandler = std::nullopt,
                .m_customRules = std::nullopt
            };
            legalClause.m_types.push_back({ .m_typeId = t.id, .m_operandIndex = std::nullopt });
            actionData.m_clauses.push_back(std::move(legalClause));
        }
        else
        {
            Symbols::LegalizeActionClauseSymbol narrowClause{
                .m_kind = DSL::Ast::LegalizeActionDef::LegalizeActionKind::NarrowScalar,
                .m_types = std::pmr::vector<Symbols::LegalizeActionConstraintSymbol>{ table->getAllocator() },
                .m_targetTypeId = maxSym->getId(),
                .m_libcallSymbol = std::nullopt,
                .m_lowerHandler = std::nullopt,
                .m_customRules = std::nullopt
            };
            narrowClause.m_types.push_back({ .m_typeId = t.id, .m_operandIndex = std::nullopt });
            actionData.m_clauses.push_back(std::move(narrowClause));
        }
    }

    return true;
}

/**
 * Resolves all action clauses for one opcode, merging them into a shared LegalizeActionSymbol
 * so grouped and direct declarations accumulate on the same opcode.
 */
bool LegalizeActionPass::processInstructionDecl(DiagnosticCollector *collector,
                                                SymbolTable *table,
                                                const DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl &decl)
{
    const auto &instIdentifier = decl.m_instName;

    // Verify that the target IR opcode is defined and is a valid IR Instruction
    Symbol *irSym = table->getSymByName(instIdentifier.m_node, SymbolType::IrInstruction);
    if (!irSym)
    {
        collector->error(PassName, "Unknown or undefined IR opcode '{}'", instIdentifier.m_node)
                << instIdentifier.m_sourceRef;
        return false;
    }

    // Check if an action symbol was already declared for this opcode (e.g. from group + specific action)
    Symbols::LegalizeActionSymbol *actionData = nullptr;
    Symbol *actSym = table->getSymByName(instIdentifier.m_node, SymbolType::LegalizeAction);
    if (actSym)
    {
        actionData = actSym->getIf<Symbols::LegalizeActionSymbol>();
    }

    if (!actionData)
    {
        Symbols::LegalizeActionSymbol actionSym{ .m_genericOpcode = instIdentifier.m_node,
                                                 .m_maxOperandIndex = 0,
                                                 .m_clauses = std::pmr::vector<Symbols::LegalizeActionClauseSymbol>{
                                                         table->getAllocator() } };

        SymbolId symId = table->declareSym(instIdentifier.m_sourceRef,
                                           SymbolType::LegalizeAction,
                                           std::move(actionSym),
                                           instIdentifier.m_node);

        if (symId == InvalidSymbolId)
        {
            collector->error(PassName, "Redefinition of legalization action for opcode '{}'", instIdentifier.m_node)
                    << instIdentifier.m_sourceRef;
            return false;
        }

        Symbol *registeredSymbol = table->getSymById(symId);
        actionData = registeredSymbol ? registeredSymbol->getIf<Symbols::LegalizeActionSymbol>() : nullptr;
        if (!actionData)
        {
            return false;
        }
    }

    bool success = true;

    // Apply clamping clause if present
    if (decl.m_clampClause.has_value())
    {
        if (!applyClampScalar(collector, table, *decl.m_clampClause, instIdentifier.m_node, *actionData))
        {
            success = false;
        }
    }

    size_t maxOperandIndex = actionData->m_maxOperandIndex;

    for (const auto &clause : decl.m_actionClauses)
    {
        // Resolve constraints, expanding type sets
        std::vector<std::vector<Symbols::LegalizeActionConstraintSymbol>> constraintLists;
        bool hasTypeSetWithIndex = false;
        bool resolveSuccess = true;

        for (const auto &typeConstraint : clause.m_types)
        {
            const auto &typeName = typeConstraint.m_type;
            Symbol *sym = table->getSymByName(typeName.m_node);
            if (!sym)
            {
                collector->error(PassName, "Unknown type or type_set '{}' in type constraint", typeName.m_node)
                        << typeName.m_sourceRef;
                resolveSuccess = false;
                continue;
            }

            std::optional<uint32_t> opIdx;
            if (typeConstraint.m_operandIndex.has_value())
            {
                const auto &indexLit = *typeConstraint.m_operandIndex;
                if (indexLit.m_node < 0)
                {
                    collector->error(PassName, "Type index cannot be negative (got {})", indexLit.m_node)
                            << indexLit.m_sourceRef;
                    resolveSuccess = false;
                    continue;
                }
                opIdx = static_cast<uint32_t>(indexLit.m_node);
                maxOperandIndex = std::max(maxOperandIndex, static_cast<size_t>(*opIdx));
            }

            std::vector<Symbols::LegalizeActionConstraintSymbol> resolved;
            if (sym->getType() == SymbolType::Type)
            {
                resolved.push_back({ .m_typeId = sym->getId(), .m_operandIndex = opIdx });
            }
            else if (sym->getType() == SymbolType::TypeSet)
            {
                const auto *tsData = sym->getIf<Symbols::TypeSetSymbol>();
                if (tsData)
                {
                    for (SymbolId tid : tsData->m_typeIds)
                    {
                        resolved.push_back({ .m_typeId = tid, .m_operandIndex = opIdx });
                    }
                }
                if (opIdx.has_value())
                {
                    hasTypeSetWithIndex = true;
                }
            }
            else
            {
                collector->error(PassName, "Symbol '{}' is neither a Type nor a TypeSet", typeName.m_node)
                        << typeName.m_sourceRef;
                resolveSuccess = false;
                continue;
            }

            constraintLists.push_back(std::move(resolved));
        }

        if (!resolveSuccess)
        {
            success = false;
            continue;
        }

        if (hasTypeSetWithIndex && constraintLists.size() > 1)
        {
            // Cartesian product for heterogeneous indexed clauses (e.g. STORE LEGAL(GPR_SCALARS:0, ptr:1))
            std::vector<std::vector<Symbols::LegalizeActionConstraintSymbol>> combinations;
            std::vector<Symbols::LegalizeActionConstraintSymbol> current;
            generateCartesianProduct(constraintLists, 0, current, combinations);

            for (const auto &comb : combinations)
            {
                Symbols::LegalizeActionClauseSymbol clauseSym{
                    .m_kind = clause.m_kind,
                    .m_types = std::pmr::vector<Symbols::LegalizeActionConstraintSymbol>{ comb.begin(),
                                                                                          comb.end(),
                                                                                          table->getAllocator() },
                    .m_targetTypeId = std::nullopt,
                    .m_libcallSymbol = std::nullopt,
                    .m_lowerHandler = std::nullopt,
                    .m_customRules = std::nullopt
                };

                if (!processClause(collector, table, clause, instIdentifier.m_node, clauseSym))
                {
                    success = false;
                    continue;
                }

                actionData->m_clauses.push_back(std::move(clauseSym));
            }
        }
        else
        {
            Symbols::LegalizeActionClauseSymbol clauseSym{
                .m_kind = clause.m_kind,
                .m_types = std::pmr::vector<Symbols::LegalizeActionConstraintSymbol>{ table->getAllocator() },
                .m_targetTypeId = std::nullopt,
                .m_libcallSymbol = std::nullopt,
                .m_lowerHandler = std::nullopt,
                .m_customRules = std::nullopt
            };

            for (const auto &list : constraintLists)
            {
                for (const auto &c : list)
                {
                    clauseSym.m_types.push_back(c);
                }
            }

            if (!processClause(collector, table, clause, instIdentifier.m_node, clauseSym))
            {
                success = false;
                continue;
            }

            actionData->m_clauses.push_back(std::move(clauseSym));
        }
    }

    actionData->m_maxOperandIndex = maxOperandIndex;

    collector->trace(PassName,
                     "Registered {} legalization action clauses for opcode '{}'",
                     actionData->m_clauses.size(),
                     instIdentifier.m_node);

    return success;
}

/**
 * Validates one action clause's kind-specific target payload and fills the resolved clause symbol.
 */
bool LegalizeActionPass::processClause(DiagnosticCollector *collector,
                                       SymbolTable *table,
                                       const DSL::Ast::LegalizeActionDef::LegalizeActionClause &clause,
                                       std::string_view instName,
                                       Symbols::LegalizeActionClauseSymbol &outClause)
{
    bool success = true;

    // Helper checks for clause mutual exclusivity
    auto ensureNoTargetType = [&]()
    {
        if (clause.m_targetType.has_value())
        {
            collector->error(PassName, "Action does not accept a target type transformation")
                    << clause.m_targetType->m_sourceRef;
            success = false;
        }
    };

    auto ensureNoLibcall = [&]()
    {
        if (clause.m_libcallSymbol.has_value())
        {
            collector->error(PassName, "Action does not accept a libcall symbol target")
                    << clause.m_libcallSymbol->m_sourceRef;
            success = false;
        }
    };

    auto ensureNoLowerHandler = [&]()
    {
        if (clause.m_lowerHandler.has_value())
        {
            collector->error(PassName, "Action does not accept a lowering handler target")
                    << clause.m_lowerHandler->m_sourceRef;
            success = false;
        }
    };

    auto ensureNoCustomRules = [&]()
    {
        if (clause.m_customRules.has_value())
        {
            collector->error(PassName, "Action does not accept custom rule targets");
            success = false;
        }
    };

    if (outClause.m_types.empty() && clause.m_kind != LegalizeActionKind::Custom &&
        clause.m_kind != LegalizeActionKind::Lower)
    {
        collector->error(PassName,
                         "Legalization clause for opcode '{}' must declare at least one type constraint",
                         instName);
        success = false;
    }

    // Kind-specific target and symbol validation
    switch (clause.m_kind)
    {
        case LegalizeActionKind::WidenScalar:
        case LegalizeActionKind::NarrowScalar:
        case LegalizeActionKind::Bitcast:
        {
            ensureNoCustomRules();
            ensureNoLibcall();
            ensureNoLowerHandler();

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
            const auto *targetData = targetSym->getIf<Symbols::TypeSymbol>();

            if (!validateBitwidths(collector,
                                   table,
                                   clause.m_kind,
                                   targetData,
                                   outClause.m_types,
                                   targetTypeId.m_sourceRef))
            {
                success = false;
            }
            break;
        }

        case LegalizeActionKind::Libcall:
        {
            ensureNoCustomRules();
            ensureNoTargetType();
            ensureNoLowerHandler();

            if (!clause.m_libcallSymbol.has_value() || clause.m_libcallSymbol->m_node.empty())
            {
                collector->error(PassName,
                                 "LIBCALL action requires a runtime library function symbol ('>> \"symbol\"')");
                return false;
            }

            outClause.m_libcallSymbol = clause.m_libcallSymbol->m_node;
            break;
        }

        case LegalizeActionKind::Lower:
        {
            ensureNoCustomRules();
            ensureNoTargetType();
            ensureNoLibcall();

            if (!clause.m_lowerHandler.has_value() || clause.m_lowerHandler->m_node.empty())
            {
                collector->error(PassName, "LOWER action requires a target lowering handler ('>> TargetLowering')");
                return false;
            }

            outClause.m_lowerHandler = clause.m_lowerHandler->m_node;
            break;
        }

        case LegalizeActionKind::Custom:
        {
            ensureNoTargetType();
            ensureNoLibcall();
            ensureNoLowerHandler();

            if (!clause.m_types.empty())
            {
                collector->error(PassName, "CUSTOM clause does not accept input types");
                success = false;
            }

            if (!clause.m_customRules.has_value() || clause.m_customRules->empty())
            {
                collector->error(PassName, "CUSTOM clause requires at least one custom rule ('>> RuleName')");
                success = false;
            }
            else
            {
                std::pmr::vector<SymbolId> resolvedRules(table->getAllocator());
                resolvedRules.reserve(clause.m_customRules->size());

                for (const auto &rule : *clause.m_customRules)
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

        case LegalizeActionKind::Legal:
        case LegalizeActionKind::Unsupported:
        {
            ensureNoTargetType();
            ensureNoLibcall();
            ensureNoLowerHandler();
            ensureNoCustomRules();
            break;
        }
    }

    return success;
}

/**
 * Resolves a type constraint to a declared type symbol and an optional operand index.
 */
bool LegalizeActionPass::resolveConstraint(DiagnosticCollector *collector,
                                           SymbolTable *table,
                                           const DSL::Ast::LegalizeActionDef::TypeConstraint &constraint,
                                           Symbols::LegalizeActionConstraintSymbol &outConstraint)
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