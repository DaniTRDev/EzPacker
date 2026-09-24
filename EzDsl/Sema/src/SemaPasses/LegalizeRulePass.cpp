#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/LegalizeRulePass.h"
#include "SemaPasses/PassDriver.h"

constexpr auto PassName = "Sema::LegalizeRulePass";

namespace
{
using namespace DSL::Ast::LegalizeRuleDef;

/**
 * Checks that an identifier refers to an SSA or immediate variable declared earlier in the rule
 * scope, reporting an error otherwise.
 */
bool validateSsaVariable(DiagnosticCollector *collector,
                         SymbolTable *table,
                         const DSL::Ast::Common::Identifier &ident,
                         std::string_view contextMsg,
                         std::string_view ruleName)
{
    Symbol *varSym = table->getSymByName(ident.m_node);
    if (!varSym || (varSym->getType() != SymbolType::SsaVariable && varSym->getType() != SymbolType::ImmediateVariable))
    {
        collector->error(PassName,
                         "Undefined SSA variable '${}' in {} of rule '{}'",
                         ident.m_node,
                         contextMsg,
                         ruleName)
                << ident.m_sourceRef;
        return false;
    }
    return true;
}

/**
 * Ensures an actual rule operand is compatible with the IR operand slot it fills, checking
 * dataflow direction and immediate-versus-register compatibility.
 */
bool validateOperandAgainstIrDef(DiagnosticCollector *collector,
                                 const Symbols::IrOperandSymbol &expectedOp,
                                 const DSL::Ast::LegalizeRuleDef::RuleInstructionOperand &actualOp,
                                 size_t opIndex,
                                 std::string_view opcodeName,
                                 std::string_view ruleName,
                                 bool isMatchPattern)
{
    const char *context = isMatchPattern ? "match clause" : "emit clause";
    const auto &opSourceRef = actualOp.m_name.m_sourceRef;

    // Dataflow Direction: An output operand slot cannot receive literals or custom transforms
    const bool isOutSlot = (expectedOp.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgOut);
    if (isOutSlot)
    {
        if (actualOp.m_kind == RuleOperandKind::ImmediateLiteral)
        {
            collector->error(
                    PassName,
                    "Operand {} of '{}' in {} of rule '{}' is an OUT parameter and cannot receive an immediate literal",
                    opIndex,
                    opcodeName,
                    context,
                    ruleName)
                    << opSourceRef;
            return false;
        }

        if (actualOp.m_kind == RuleOperandKind::CustomTransform)
        {
            collector->error(PassName,
                             "Operand {} of '{}' in {} of rule '{}' is an OUT parameter and cannot receive a transform "
                             "expression",
                             opIndex,
                             opcodeName,
                             context,
                             ruleName)
                    << opSourceRef;
            return false;
        }
    }

    // Type Compatibility: Immediate Slot vs Register Slot
    const uint16_t opTypeMask = static_cast<uint16_t>(expectedOp.m_type);
    const bool acceptsImm = (opTypeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::Immediate)) != 0;
    const bool acceptsReg = (opTypeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::Register)) != 0;

    if (actualOp.m_kind == RuleOperandKind::SsaRegister && !acceptsReg)
    {
        // Slot expects an immediate, but got an SSA Virtual Register
        collector->error(PassName,
                         "Operand {} ('{}') of instruction '{}' in {} expects an immediate value, but got SSA "
                         "register '${}' in rule '{}'",
                         opIndex,
                         expectedOp.m_name,
                         opcodeName,
                         context,
                         actualOp.m_name.m_node,
                         ruleName)
                << opSourceRef;
        return false;
    }

    if ((actualOp.m_kind == RuleOperandKind::ImmediateLiteral || actualOp.m_kind == RuleOperandKind::ImmediateSymbol) &&
        !acceptsImm)
    {
        collector->error(PassName,
                         "Operand {} ('{}') of instruction '{}' in {} expects a register operand, but got "
                         "immediate in rule '{}'",
                         opIndex,
                         expectedOp.m_name,
                         opcodeName,
                         context,
                         ruleName)
                << opSourceRef;
        return false;
    }

    return true;
}
} // namespace

bool LegalizeRulePass::run(DiagnosticCollector *collector,
                           SymbolTable *table,
                           DSL::Ast::LegalizeRuleDef::LegalizeRuleFile *file)
{
    if (!Sema::preparePass(collector, table, file, PassName))
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
                                   const DSL::Ast::LegalizeRuleDef::LegalizeRule &rule)
{
    const auto &ruleNameIdent = rule.m_ruleName;
    const auto &ruleName = ruleNameIdent.m_node;

    // Structural requirements
    if (rule.m_matchClauses.empty() || rule.m_emitClauses.empty())
    {
        const char *missingKind = rule.m_matchClauses.empty() ? "match" : "emit";
        collector->error(PassName, "Legalization rule '{}' must declare at least one {} clause", ruleName, missingKind)
                << ruleNameIdent.m_sourceRef;
        return false;
    }

    // Declare the rule symbol in the enclosing scope
    Symbols::LegalizeRuleSymbol ruleSym{
        .m_ruleName = ruleName,
        .m_matchPatterns = std::pmr::vector<Symbols::LegalizeRuleInstructionSymbol>(table->getAllocator()),
        .m_predicates = std::pmr::vector<Symbols::LegalizeRulePredicateSymbol>(table->getAllocator()),
        .m_expansionSequence = std::pmr::vector<Symbols::LegalizeRuleInstructionSymbol>(table->getAllocator())
    };

    SymbolId ruleSymId =
            table->declareSym(ruleNameIdent.m_sourceRef, SymbolType::LegalizeRule, std::move(ruleSym), ruleName);

    if (ruleSymId == InvalidSymbolId)
    {
        collector->error(PassName, "Redefinition of legalization rewrite rule '{}'", ruleName)
                << ruleNameIdent.m_sourceRef;
        return false;
    }

    Symbol *registeredRule = table->getSymById(ruleSymId);
    auto *ruleData = registeredRule ? registeredRule->getIf<Symbols::LegalizeRuleSymbol>() : nullptr;
    if (!ruleData)
    {
        return false;
    }

    bool success = true;
    table->enterScope(ruleName);

    // Validate match clauses & bind defined SSA variables
    for (const auto &matchInst : rule.m_matchClauses)
    {
        Symbols::LegalizeRuleInstructionSymbol instSym{
            .m_opcode = matchInst.m_opcode.m_node,
            .m_operands = std::pmr::vector<Symbols::LegalizeRuleOperandSymbol>(table->getAllocator())
        };

        if (!processInstruction(collector, table, matchInst, ruleName, /*isMatchPattern=*/true, instSym))
        {
            success = false;
            continue;
        }

        ruleData->m_matchPatterns.push_back(std::move(instSym));
    }

    // Validate 'when' predicates against declared SSA variables
    for (const auto &predicate : rule.m_whenClauses)
    {
        if (!processPredicate(collector, table, predicate, ruleName))
        {
            success = false;
        }

        Symbols::LegalizeRulePredicateSymbol predSym{
            .m_name = predicate.m_predicateName.m_node,
            .m_args = std::pmr::vector<std::variant<std::string_view, int64_t>>(table->getAllocator())
        };
        for (const auto &arg : predicate.m_arguments)
        {
            if (const auto *ident = std::get_if<DSL::Ast::Common::Identifier>(&arg))
            {
                predSym.m_args.push_back(ident->m_node);
            }
            else if (const auto *lit = std::get_if<DSL::Ast::Common::IntegerLiteral>(&arg))
            {
                predSym.m_args.push_back(lit->m_node);
            }
        }
        ruleData->m_predicates.push_back(std::move(predSym));
    }

    // Validate emit clause instructions and operand usages
    for (const auto &emitInst : rule.m_emitClauses)
    {
        Symbols::LegalizeRuleInstructionSymbol instSym{
            .m_opcode = emitInst.m_opcode.m_node,
            .m_operands = std::pmr::vector<Symbols::LegalizeRuleOperandSymbol>(table->getAllocator())
        };

        if (!processInstruction(collector, table, emitInst, ruleName, /*isMatchPattern=*/false, instSym))
        {
            success = false;
            continue;
        }

        ruleData->m_expansionSequence.push_back(std::move(instSym));
    }

    table->exitScope();
    collector->trace(PassName,
                     "Defined legalization rewrite rule '{}' ({} match clauses, {} emit clauses)",
                     ruleName,
                     ruleData->m_matchPatterns.size(),
                     ruleData->m_expansionSequence.size());

    return success;
}

bool LegalizeRulePass::processInstruction(DiagnosticCollector *collector,
                                          SymbolTable *table,
                                          const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                          std::string_view ruleName,
                                          bool isMatchPattern,
                                          Symbols::LegalizeRuleInstructionSymbol &outInst)
{
    const auto &opcodeIdent = inst.m_opcode;
    Symbol *opcodeSym = table->getSymByName(opcodeIdent.m_node);
    if (!opcodeSym || opcodeSym->getType() != SymbolType::IrInstruction)
    {
        const char *context = isMatchPattern ? "match clause" : "emit clause";
        collector->error(PassName,
                         "Unknown or undefined IR opcode '{}' in {} of rule '{}'",
                         opcodeIdent.m_node,
                         context,
                         ruleName)
                << opcodeIdent.m_sourceRef;
        return false;
    }

    const auto *irInstDef = opcodeSym->getIf<Symbols::IrInstructionSymbol>();
    if (!irInstDef)
    {
        return false;
    }

    // Arity Check: Verify operand count against IR instruction definition
    const size_t expectedArity = irInstDef->m_operands.size();
    const size_t actualArity = inst.m_operands.size();

    if (expectedArity != actualArity)
    {
        const char *context = isMatchPattern ? "match clause" : "emit clause";
        collector->error(PassName,
                         "Instruction '{}' in {} of rule '{}' expects {} operands, but got {}",
                         opcodeIdent.m_node,
                         context,
                         ruleName,
                         expectedArity,
                         actualArity)
                << opcodeIdent.m_sourceRef;
        return false;
    }

    // Resolve individual operands and validate against instruction definition signature
    bool success = true;
    for (size_t i = 0; i < actualArity; ++i)
    {
        const auto &operand = inst.m_operands[i];
        const auto &expectedOp = irInstDef->m_operands[i];

        if (!validateOperandAgainstIrDef(collector,
                                         expectedOp,
                                         operand,
                                         i,
                                         opcodeIdent.m_node,
                                         ruleName,
                                         isMatchPattern))
        {
            success = false;
        }

        Symbols::LegalizeRuleOperandSymbol opSym;
        if (!resolveOperand(collector, table, operand, ruleName, isMatchPattern, opSym))
        {
            success = false;
            continue;
        }

        outInst.m_operands.push_back(std::move(opSym));
    }

    return success;
}

bool LegalizeRulePass::processPredicate(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        const DSL::Ast::LegalizeRuleDef::RuleWhen &predicate,
                                        std::string_view ruleName)
{
    if (predicate.m_predicateName.m_node == "hasExtension" || predicate.m_predicateName.m_node == "hasFeature")
    {
        if (predicate.m_arguments.empty())
        {
            collector->error(PassName,
                             "Predicate '{}' in rule '{}' requires an extension name argument",
                             predicate.m_predicateName.m_node,
                             ruleName)
                    << predicate.m_predicateName.m_sourceRef;
            return false;
        }
        return true;
    }

    bool success = true;
    std::string contextMsg = "predicate '" + std::string(predicate.m_predicateName.m_node) + "'";

    for (const auto &arg : predicate.m_arguments)
    {
        if (const auto *ident = std::get_if<DSL::Ast::Common::Identifier>(&arg))
        {
            if (!validateSsaVariable(collector, table, *ident, contextMsg, ruleName))
            {
                success = false;
            }
        }
    }

    return success;
}

bool LegalizeRulePass::resolveOperand(DiagnosticCollector *collector,
                                      SymbolTable *table,
                                      const DSL::Ast::LegalizeRuleDef::RuleInstructionOperand &operand,
                                      std::string_view ruleName,
                                      bool isMatchPattern,
                                      Symbols::LegalizeRuleOperandSymbol &outOperand)
{
    outOperand.m_kind = static_cast<DSL::Ast::LegalizeRuleDef::RuleOperandKind>(operand.m_kind);
    outOperand.m_name = operand.m_name.m_node;
    outOperand.m_typeOrClassId = std::nullopt;
    outOperand.m_immLiteral = std::nullopt;
    outOperand.m_callArgs = std::pmr::vector<std::string_view>(table->getAllocator());

    // Literal Operand
    if (operand.m_kind == RuleOperandKind::ImmediateLiteral)
    {
        if (operand.m_immLiteral.has_value())
        {
            outOperand.m_immLiteral = operand.m_immLiteral->m_node;
        }
        return true;
    }

    // Custom Transform (e.g., log2($src))
    if (operand.m_kind == RuleOperandKind::CustomTransform)
    {
        for (const auto &argIdent : operand.m_callArgs)
        {
            outOperand.m_callArgs.push_back(argIdent.m_node);
        }

        if (isMatchPattern)
        {
            collector->error(PassName, "Custom transform '{}' cannot be used in a match clause", operand.m_name.m_node)
                    << operand.m_name.m_sourceRef;
            return false;
        }

        bool success = true;
        std::string contextMsg = "transform '" + std::string(operand.m_name.m_node) + "'";
        for (const auto &argIdent : operand.m_callArgs)
        {
            if (!validateSsaVariable(collector, table, argIdent, contextMsg, ruleName))
            {
                success = false;
            }
        }
        return success;
    }

    // Type Resolution for Typed SSA / Parameterized Immediate Types
    const auto &typeToResolve = operand.m_typeParam.has_value()
            ? operand.m_typeParam
            : (operand.m_kind != RuleOperandKind::ImmediateSymbol ? operand.m_type : std::nullopt);
    if (typeToResolve.has_value())
    {
        const auto &typeName = *typeToResolve;
        Symbol *typeSym = table->getSymByName(typeName.m_node);
        if (!typeSym || typeSym->getType() != SymbolType::Type)
        {
            collector->error(PassName,
                             "Unknown type '{}' on operand '${}' in rule '{}'",
                             typeName.m_node,
                             operand.m_name.m_node,
                             ruleName)
                    << typeName.m_sourceRef;
            return false;
        }
        outOperand.m_typeOrClassId = typeSym->getId();
    }

    // SSA Variable Registration / Usage
    if (operand.m_kind == RuleOperandKind::SsaRegister || operand.m_kind == RuleOperandKind::ImmediateSymbol)
    {
        if (!table->getSymByName(operand.m_name.m_node))
        {
            table->declareSym(operand.m_name.m_sourceRef,
                              operand.m_kind == RuleOperandKind::SsaRegister ? SymbolType::SsaVariable
                                                                             : SymbolType::ImmediateVariable,
                              std::monostate{},
                              operand.m_name.m_node);
        }
    }

    return true;
}