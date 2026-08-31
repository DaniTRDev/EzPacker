#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeActionPass.h"
#include "Sema/Symbol.h"
#include "SourceManager/GenericSourceManager.h"

constexpr auto PassName = "Sema::LegalizeActionPass";

namespace
{
using namespace DSL::Ast::LegalizeActionDef;

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
} // namespace

bool LegalizeActionPass::run(DiagnosticCollector *collector,
                             SymbolTable *table,
                             DSL::Ast::LegalizeActionDef::LegalizeActionFile *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(PassName, "Running semantic validation for legalization actions");

    bool success = true;
    for (const auto &decl : file->m_legalizeInstrDecls)
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
                                                const DSL::Ast::LegalizeActionDef::LegalizeInstructionDecl &decl)
{
    const auto &instIdentifier = decl.m_instName;

    // Verify that the target IR opcode is defined and is a valid IR Instruction
    Symbol *irSym = table->getSymByName(instIdentifier.m_node);
    if (!irSym || irSym->getType() != SymbolType::IrInstruction)
    {
        collector->error(PassName, "Unknown or undefined IR opcode '{}'", instIdentifier.m_node)
                << instIdentifier.m_sourceRef;
        return false;
    }

    // Declare the legalization action symbol for this opcode
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
    auto *actionData = registeredSymbol ? registeredSymbol->getIf<Symbols::LegalizeActionSymbol>() : nullptr;
    if (!actionData)
    {
        return false;
    }

    bool success = true;
    size_t maxOperandIndex = 0;

    for (const auto &clause : decl.m_actionClauses)
    {
        Symbols::LegalizeActionClauseSymbol clauseSym{
            .m_kind = clause.m_kind,
            .m_types = std::pmr::vector<Symbols::LegalizeActionConstraintSymbol>{ table->getAllocator() },
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

    actionData->m_maxOperandIndex = maxOperandIndex;

    collector->trace(PassName,
                     "Registered {} legalization action clauses for opcode '{}'",
                     actionData->m_clauses.size(),
                     instIdentifier.m_node);

    return success;
}

bool LegalizeActionPass::processClause(DiagnosticCollector *collector,
                                       SymbolTable *table,
                                       const DSL::Ast::LegalizeActionDef::LegalizeActionClause &clause,
                                       std::string_view instName,
                                       Symbols::LegalizeActionClauseSymbol &outClause,
                                       size_t &maxOperandIndex)
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

    auto ensureNoCustomRules = [&]()
    {
        if (clause.m_customRules.has_value())
        {
            collector->error(PassName, "Action does not accept custom rule targets");
            success = false;
        }
    };

    // Resolve and validate matched type constraints
    for (const auto &typeConstraint : clause.m_types)
    {
        Symbols::LegalizeActionConstraintSymbol constraintSym;
        if (!resolveConstraint(collector, table, typeConstraint, constraintSym))
        {
            success = false;
            continue;
        }

        if (constraintSym.m_operandIndex.has_value())
        {
            maxOperandIndex = std::max(maxOperandIndex, static_cast<size_t>(*constraintSym.m_operandIndex));
        }

        outClause.m_types.push_back(constraintSym);
    }

    if (outClause.m_types.empty() && clause.m_kind != LegalizeActionKind::Custom)
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

            if (!clause.m_libcallSymbol.has_value() || clause.m_libcallSymbol->m_node.empty())
            {
                collector->error(PassName,
                                 "LIBCALL action requires a runtime library function symbol ('>> \"symbol\"')");
                return false;
            }

            outClause.m_libcallSymbol = clause.m_libcallSymbol->m_node;
            break;
        }

        case LegalizeActionKind::Custom:
        {
            ensureNoTargetType();
            ensureNoLibcall();

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
            ensureNoCustomRules();
            break;
        }
    }

    return success;
}

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