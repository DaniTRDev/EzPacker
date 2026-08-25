#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeRulePass.h"
#include "Sema/Symbols/Symbols.h"

constexpr auto PassName = "Sema::LegalizeRulePass";

bool LegalizeRulePass::run(DiagnosticCollector *collector,
                           SymbolTable *table,
                           DSL::Ast::LegalizeRuleDef::TargetLegalizeRuleDef *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(PassName, "Running semantic validation for legalization rewrite rules");

    bool success = true;
    for (const auto &rule : file->m_rules)
    {
        if (!processRule(collector, table, rule))
        {
            success = false;
        }
    }

    return success;
}

bool LegalizeRulePass::processRule(DiagnosticCollector *collector,
                                   SymbolTable *table,
                                   const DSL::Ast::LegalizeRuleDef::LegalizeRewriteRule &rule)
{
    const auto &ruleNameIdent = rule.m_ruleName;

    // 1. Structural requirements
    if (rule.m_matchPatterns.empty())
    {
        collector->error(PassName,
                         "Legalization rule '{}' must declare at least one match instruction",
                         ruleNameIdent.m_node)
                << ruleNameIdent.m_sourceRef;
        return false;
    }

    if (rule.m_expansionSequence.empty())
    {
        collector->error(PassName,
                         "Legalization rule '{}' must declare at least one expansion instruction",
                         ruleNameIdent.m_node)
                << ruleNameIdent.m_sourceRef;
        return false;
    }

    // 2. Declare the rule in the current scope
    Sema::Symbols::LegalizeRewriteRuleSymbol ruleSym{
        .m_ruleName = ruleNameIdent.m_node,
        .m_matchPatterns = std::pmr::vector<Sema::Symbols::RuleInstructionSymbol>{ table->getAllocator() },
        .m_expansionSequence = std::pmr::vector<Sema::Symbols::RuleInstructionSymbol>{ table->getAllocator() }
    };

    SymbolId ruleSymId = table->declareSym(ruleNameIdent.m_sourceRef,
                                           SymbolFlags::IsDefined,
                                           SymbolType::ISelPattern,
                                           std::move(ruleSym),
                                           ruleNameIdent.m_node);

    if (ruleSymId == InvalidSymbolId)
    {
        collector->error(PassName, "Redefinition of legalization rewrite rule '{}'", ruleNameIdent.m_node)
                << ruleNameIdent.m_sourceRef;
        return false;
    }

    Symbol *registeredRule = table->getSymById(ruleSymId);
    auto *ruleData = registeredRule ? registeredRule->getIf<Sema::Symbols::LegalizeRewriteRuleSymbol>() : nullptr;
    if (!ruleData)
    {
        return false;
    }

    // 3. Enter dedicated scope for rule SSA variables
    table->enterScope(ruleNameIdent.m_node);

    bool success = true;

    // 4. Validate match patterns & bind defined SSA variables
    for (const auto &matchInst : rule.m_matchPatterns)
    {
        Sema::Symbols::RuleInstructionSymbol instSym{ .m_opcode = matchInst.m_opcode.m_node,
                                                      .m_operands = std::pmr::vector<Sema::Symbols::RuleOperandSymbol>{
                                                              table->getAllocator() } };

        if (!processMatchPattern(collector, table, matchInst, ruleNameIdent.m_node, instSym))
        {
            success = false;
            continue;
        }

        ruleData->m_matchPatterns.push_back(std::move(instSym));
    }

    // 5. Validate 'when' predicates against declared SSA variables
    for (const auto &predicate : rule.m_predicates)
    {
        if (!processPredicate(collector, table, predicate, ruleNameIdent.m_node))
        {
            success = false;
        }
    }

    // 6. Validate expand sequence instructions and operand usages
    for (const auto &expandInst : rule.m_expansionSequence)
    {
        Sema::Symbols::RuleInstructionSymbol instSym{ .m_opcode = expandInst.m_opcode.m_node,
                                                      .m_operands = std::pmr::vector<Sema::Symbols::RuleOperandSymbol>{
                                                              table->getAllocator() } };

        if (!processExpandInstruction(collector, table, expandInst, ruleNameIdent.m_node, instSym))
        {
            success = false;
            continue;
        }

        ruleData->m_expansionSequence.push_back(std::move(instSym));
    }

    table->exitScope();

    collector->trace(PassName,
                     "Defined legalization rewrite rule '{}' ({} match patterns, {} expand instructions)",
                     ruleNameIdent.m_node,
                     ruleData->m_matchPatterns.size(),
                     ruleData->m_expansionSequence.size());

    return success;
}

bool LegalizeRulePass::processMatchPattern(DiagnosticCollector *collector,
                                           SymbolTable *table,
                                           const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                           std::string_view ruleName,
                                           Sema::Symbols::RuleInstructionSymbol &outInst)
{
    // Opcode validation against defined IrInstructions
    Symbol *opcodeSym = table->getSymByName(inst.m_opcode.m_node);
    if (!opcodeSym || opcodeSym->getType() != SymbolType::IrInstruction)
    {
        collector->error(PassName,
                         "Unknown or undefined IR opcode '{}' in match pattern of rule '{}'",
                         inst.m_opcode.m_node,
                         ruleName)
                << inst.m_opcode.m_sourceRef;
        return false;
    }

    bool success = true;
    for (const auto &operand : inst.m_operands)
    {
        Sema::Symbols::RuleOperandSymbol opSym;
        if (!resolveOperand(collector, table, operand, ruleName, /*isMatchPattern=*/true, opSym))
        {
            success = false;
            continue;
        }
        outInst.m_operands.push_back(opSym);
    }

    return success;
}

bool LegalizeRulePass::processPredicate(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        const DSL::Ast::LegalizeRuleDef::RulePredicate &predicate,
                                        std::string_view ruleName)
{
    bool success = true;

    for (const auto &arg : predicate.m_arguments)
    {
        if (std::holds_alternative<DSL::Ast::Common::Identifier>(arg))
        {
            const auto &ident = std::get<DSL::Ast::Common::Identifier>(arg);
            Symbol *varSym = table->getSymByName(ident.m_node);

            if (!varSym || varSym->getType() != SymbolType::SsaVariable)
            {
                collector->error(PassName,
                                 "Undefined SSA variable '${}' in predicate '{}' of rule '{}'",
                                 ident.m_node,
                                 predicate.m_predicateName.m_node,
                                 ruleName)
                        << ident.m_sourceRef;
                success = false;
            }
        }
    }

    return success;
}

bool LegalizeRulePass::processExpandInstruction(DiagnosticCollector *collector,
                                                SymbolTable *table,
                                                const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                                std::string_view ruleName,
                                                Sema::Symbols::RuleInstructionSymbol &outInst)
{
    // Validate expansion opcode
    Symbol *opcodeSym = table->getSymByName(inst.m_opcode.m_node);
    if (!opcodeSym || opcodeSym->getType() != SymbolType::IrInstruction)
    {
        collector->error(PassName,
                         "Unknown or undefined IR opcode '{}' in expansion sequence of rule '{}'",
                         inst.m_opcode.m_node,
                         ruleName)
                << inst.m_opcode.m_sourceRef;
        return false;
    }

    bool success = true;
    for (const auto &operand : inst.m_operands)
    {
        Sema::Symbols::RuleOperandSymbol opSym;
        if (!resolveOperand(collector, table, operand, ruleName, /*isMatchPattern=*/false, opSym))
        {
            success = false;
            continue;
        }
        outInst.m_operands.push_back(opSym);
    }

    return success;
}

bool LegalizeRulePass::resolveOperand(DiagnosticCollector *collector,
                                      SymbolTable *table,
                                      const DSL::Ast::LegalizeRuleDef::RuleOperand &operand,
                                      std::string_view ruleName,
                                      bool isMatchPattern,
                                      Sema::Symbols::RuleOperandSymbol &outOperand)
{
    outOperand.m_kind = operand.m_kind;
    outOperand.m_name = operand.m_name.m_node;
    outOperand.m_typeOrClassId = std::nullopt;
    outOperand.m_immLiteral = std::nullopt;

    // 1. Literal Operand
    if (operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral)
    {
        if (operand.m_immLiteral.has_value())
        {
            outOperand.m_immLiteral = operand.m_immLiteral->m_node;
        }
        return true;
    }

    // 2. Custom Transform (e.g. log2($src))
    if (operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform)
    {
        if (isMatchPattern)
        {
            collector->error(PassName,
                             "Custom transform '{}' cannot be used as an input match pattern",
                             operand.m_name.m_node)
                    << operand.m_name.m_sourceRef;
            return false;
        }

        bool success = true;
        for (const auto &argIdent : operand.m_callArgs)
        {
            Symbol *argSym = table->getSymByName(argIdent.m_node);
            if (!argSym || argSym->getType() != SymbolType::SsaVariable)
            {
                collector->error(PassName,
                                 "Undefined SSA variable '${}' passed to transform '{}' in rule '{}'",
                                 argIdent.m_node,
                                 operand.m_name.m_node,
                                 ruleName)
                        << argIdent.m_sourceRef;
                success = false;
            }
        }
        return success;
    }

    // 3. Type Resolution for Typed SSA / Immediate Symbols
    if (operand.m_type.has_value())
    {
        const auto &typeName = *operand.m_type;
        // Ignore pseudo-types like imm/simm/uimm unless defined as actual types
        if (operand.m_kind != DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol)
        {
            Symbol *typeSym = table->getSymByName(typeName.m_node);
            if (!typeSym || typeSym->getType() != SymbolType::Type)
            {
                collector->error(PassName,
                                 "Unknown type '{}' on SSA variable '${}' in rule '{}'",
                                 typeName.m_node,
                                 operand.m_name.m_node,
                                 ruleName)
                        << typeName.m_sourceRef;
                return false;
            }
            outOperand.m_typeOrClassId = typeSym->getId();
        }
    }

    // 4. SSA Variable Registration and Lookup
    if (operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister ||
        operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol)
    {
        Symbol *existingVar = table->getSymByName(operand.m_name.m_node);

        if (isMatchPattern)
        {
            if (!existingVar)
            {
                table->declareSym(operand.m_name.m_sourceRef,
                                  SymbolFlags::IsDefined,
                                  SymbolType::SsaVariable,
                                  std::monostate{},
                                  operand.m_name.m_node);
            }
        }
        else
        {
            // If in expand block, declare if it introduces a new temporary, or resolve existing
            if (!existingVar)
            {
                table->declareSym(operand.m_name.m_sourceRef,
                                  SymbolFlags::IsDefined,
                                  SymbolType::SsaVariable,
                                  std::monostate{},
                                  operand.m_name.m_node);
            }
        }
    }

    return true;
}