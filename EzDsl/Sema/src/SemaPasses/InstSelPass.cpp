#include "Ast/InstructionDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/InstSelPass.h"

constexpr auto PassName = "Sema::InstSelPass";

bool InstSelPass::run(DiagnosticCollector *collector, SymbolTable *table, DSL::Ast::InstSelDef::ISelDefFile *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(PassName, "Running semantic validation for instruction selection definitions");

    bool success = true;

    // Phase 1: Declare and validate Addressing Modes
    for (const auto &addrMode : file->m_addrModes)
    {
        if (!processAddrMode(collector, table, addrMode))
        {
            success = false;
        }
    }

    // Phase 2: Declare and validate ISel Patterns
    for (const auto &pattern : file->m_patterns)
    {
        if (!processPattern(collector, table, pattern))
        {
            success = false;
        }
    }

    return success;
}

bool InstSelPass::processAddrMode(DiagnosticCollector *collector,
                                  SymbolTable *table,
                                  const DSL::Ast::InstSelDef::AddrModeDef &def)
{
    const auto &nameIdent = def.m_name;

    if (def.m_variants.empty())
    {
        collector->error(PassName, "Addressing mode '{}' must declare at least one variant", nameIdent.m_node)
                << nameIdent.m_sourceRef;
        return false;
    }

    Sema::Symbols::AddrModeSymbol addrModeSym{
        .m_name = nameIdent.m_node,
        .m_parameters = std::pmr::vector<Sema::Symbols::TargetOperandSymbol>{ table->getAllocator() },
        .m_variants = std::pmr::vector<Sema::Symbols::AddrModeVariantSymbol>{ table->getAllocator() }
    };

    std::unordered_set<std::string_view> seenParams;

    // 1. Resolve and validate formal parameters
    for (const auto &param : def.m_parameters)
    {
        const auto &paramName = param.m_name.m_node;
        if (!seenParams.insert(paramName).second)
        {
            collector->error(PassName,
                             "Addressing mode '{}': Duplicate parameter name '{}'",
                             nameIdent.m_node,
                             paramName)
                    << param.m_name.m_sourceRef;
            return false;
        }

        const bool isImm = (param.m_typeOrClass.m_node == "imm" || param.m_typeOrClass.m_node == "simm" ||
                            param.m_typeOrClass.m_node == "uimm");

        SymbolId resolvedId = InvalidSymbolId;
        auto opKind =
                isImm ? DSL::Ast::InstDef::InstOperandKind::Immediate : DSL::Ast::InstDef::InstOperandKind::Register;

        if (isImm)
        {
            if (param.m_typeParam.has_value())
            {
                Symbol *typeSym = table->getSymByName(param.m_typeParam->m_node);
                if (!typeSym || typeSym->getType() != SymbolType::Type)
                {
                    collector->error(PassName,
                                     "Addressing mode '{}', parameter '{}': Unknown type '{}'",
                                     nameIdent.m_node,
                                     paramName,
                                     param.m_typeParam->m_node)
                            << param.m_typeParam->m_sourceRef;
                    return false;
                }
                resolvedId = typeSym->getId();
            }
        }
        else
        {
            Symbol *classSym = table->getSymByName(param.m_typeOrClass.m_node);
            if (!classSym || classSym->getType() != SymbolType::RegisterClass)
            {
                collector->error(PassName,
                                 "Addressing mode '{}', parameter '{}': Unknown register class '{}'",
                                 nameIdent.m_node,
                                 paramName,
                                 param.m_typeOrClass.m_node)
                        << param.m_typeOrClass.m_sourceRef;
                return false;
            }
            resolvedId = classSym->getId();
        }

        addrModeSym.m_parameters.push_back(
                Sema::Symbols::TargetOperandSymbol{ .m_kind = opKind,
                                                    .m_typeOrClassId = resolvedId,
                                                    .m_name = paramName,
                                                    .m_dir = DSL::Ast::InstDef::InstOperandDir::ArgIn });
    }

    // 2. Declare the addressing mode symbol
    SymbolId addrModeId = table->declareSym(nameIdent.m_sourceRef,
                                            SymbolFlags::IsDefined,
                                            SymbolType::AddrMode,
                                            std::move(addrModeSym),
                                            nameIdent.m_node);

    if (addrModeId == InvalidSymbolId)
    {
        collector->error(PassName, "Redefinition of addressing mode '{}'", nameIdent.m_node) << nameIdent.m_sourceRef;
        return false;
    }

    Symbol *registeredSym = table->getSymById(addrModeId);
    auto *addrModeData = registeredSym ? registeredSym->getIf<Sema::Symbols::AddrModeSymbol>() : nullptr;
    if (!addrModeData)
    {
        return false;
    }

    // 3. Process variants in a dedicated scope containing formal parameters
    table->enterScope(nameIdent.m_node);

    for (const auto &param : def.m_parameters)
    {
        table->declareSym(param.m_name.m_sourceRef,
                          SymbolFlags::IsDefined,
                          SymbolType::SsaVariable,
                          std::monostate{},
                          param.m_name.m_node);
    }

    std::unordered_set<std::string_view> seenVariants;
    bool success = true;

    for (const auto &variant : def.m_variants)
    {
        const auto &varName = variant.m_variantName.m_node;
        if (!seenVariants.insert(varName).second)
        {
            collector->error(PassName, "Addressing mode '{}': Duplicate variant name '{}'", nameIdent.m_node, varName)
                    << variant.m_variantName.m_sourceRef;
            success = false;
            continue;
        }

        Sema::Symbols::AddrModeVariantSymbol variantSym{
            .m_name = varName,
            .m_matchPatterns = std::pmr::vector<Sema::Symbols::RuleInstructionSymbol>{ table->getAllocator() }
        };

        if (!processAddrModeVariant(collector, table, variant, nameIdent.m_node, variantSym))
        {
            success = false;
            continue;
        }

        addrModeData->m_variants.push_back(std::move(variantSym));
    }

    table->exitScope();
    return success;
}

bool InstSelPass::processAddrModeVariant(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::InstSelDef::AddrModeVariant &variant,
                                         std::string_view addrModeName,
                                         Sema::Symbols::AddrModeVariantSymbol &outVariant)
{
    if (variant.m_matchPatterns.empty())
    {
        collector->error(PassName,
                         "Addressing mode '{}', variant '{}': Must declare at least one match pattern",
                         addrModeName,
                         variant.m_variantName.m_node)
                << variant.m_variantName.m_sourceRef;
        return false;
    }

    table->enterScope(variant.m_variantName.m_node);
    bool success = true;

    for (const auto &inst : variant.m_matchPatterns)
    {
        Sema::Symbols::RuleInstructionSymbol instSym{ .m_opcode = inst.m_opcode.m_node,
                                                      .m_operands = std::pmr::vector<Sema::Symbols::RuleOperandSymbol>{
                                                              table->getAllocator() } };

        if (!processMatchInstruction(collector, table, inst, variant.m_variantName.m_node, instSym))
        {
            success = false;
            continue;
        }

        outVariant.m_matchPatterns.push_back(std::move(instSym));
    }

    for (const auto &pred : variant.m_predicates)
    {
        if (!processPredicate(collector, table, pred, variant.m_variantName.m_node))
        {
            success = false;
        }
    }

    table->exitScope();
    return success;
}

bool InstSelPass::processPattern(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 const DSL::Ast::InstSelDef::ISelPattern &pattern)
{
    const auto &nameIdent = pattern.m_patternName;

    if (pattern.m_matchPatterns.empty())
    {
        collector->error(PassName, "Pattern '{}' must declare at least one match instruction", nameIdent.m_node)
                << nameIdent.m_sourceRef;
        return false;
    }

    if (pattern.m_emitSequence.empty())
    {
        collector->error(PassName, "Pattern '{}' must declare at least one emit instruction", nameIdent.m_node)
                << nameIdent.m_sourceRef;
        return false;
    }

    uint32_t costVal = 1;
    if (pattern.m_cost.has_value())
    {
        if (pattern.m_cost->m_node < 0)
        {
            collector->error(PassName,
                             "Pattern '{}': Cost cannot be negative ({})",
                             nameIdent.m_node,
                             pattern.m_cost->m_node)
                    << pattern.m_cost->m_sourceRef;
            return false;
        }
        costVal = static_cast<uint32_t>(pattern.m_cost->m_node);
    }

    Sema::Symbols::ISelPatternSymbol patternSym{
        .m_patternName = nameIdent.m_node,
        .m_matchPatterns = std::pmr::vector<Sema::Symbols::RuleInstructionSymbol>{ table->getAllocator() },
        .m_emitSequence = std::pmr::vector<Sema::Symbols::RuleInstructionSymbol>{ table->getAllocator() },
        .m_cost = costVal
    };

    SymbolId patSymId = table->declareSym(nameIdent.m_sourceRef,
                                          SymbolFlags::IsDefined,
                                          SymbolType::ISelPattern,
                                          std::move(patternSym),
                                          nameIdent.m_node);

    if (patSymId == InvalidSymbolId)
    {
        collector->error(PassName, "Redefinition of instruction selection pattern '{}'", nameIdent.m_node)
                << nameIdent.m_sourceRef;
        return false;
    }

    Symbol *registeredSym = table->getSymById(patSymId);
    auto *patternData = registeredSym ? registeredSym->getIf<Sema::Symbols::ISelPatternSymbol>() : nullptr;
    if (!patternData)
    {
        return false;
    }

    table->enterScope(nameIdent.m_node);
    bool success = true;

    // 1. Process match sequence (Validated against Generic IR instruction definitions)
    for (const auto &matchInst : pattern.m_matchPatterns)
    {
        Sema::Symbols::RuleInstructionSymbol instSym{ .m_opcode = matchInst.m_opcode.m_node,
                                                      .m_operands = std::pmr::vector<Sema::Symbols::RuleOperandSymbol>{
                                                              table->getAllocator() } };

        if (!processMatchInstruction(collector, table, matchInst, nameIdent.m_node, instSym))
        {
            success = false;
            continue;
        }

        patternData->m_matchPatterns.push_back(std::move(instSym));
    }

    // 2. Process 'when' predicates
    for (const auto &predicate : pattern.m_predicates)
    {
        if (!processPredicate(collector, table, predicate, nameIdent.m_node))
        {
            success = false;
        }
    }

    // 3. Process emit sequence (Validated against Target hardware instruction definitions)
    for (const auto &emitInst : pattern.m_emitSequence)
    {
        Sema::Symbols::RuleInstructionSymbol instSym{ .m_opcode = emitInst.m_opcode.m_node,
                                                      .m_operands = std::pmr::vector<Sema::Symbols::RuleOperandSymbol>{
                                                              table->getAllocator() } };

        if (!processEmitInstruction(collector, table, emitInst, nameIdent.m_node, instSym))
        {
            success = false;
            continue;
        }

        patternData->m_emitSequence.push_back(std::move(instSym));
    }

    table->exitScope();

    collector->trace(PassName, "Defined ISel pattern '{}' (cost: {})", nameIdent.m_node, patternData->m_cost);
    return success;
}

bool InstSelPass::processMatchInstruction(DiagnosticCollector *collector,
                                          SymbolTable *table,
                                          const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                          std::string_view contextName,
                                          Sema::Symbols::RuleInstructionSymbol &outInst)
{
    // Special case: Single-operand addressing mode match (e.g., `GPR:$base;`)
    if (inst.m_operands.size() == 1 &&
        (inst.m_opcode.m_node == inst.m_operands[0].m_name.m_node ||
         (inst.m_operands[0].m_type.has_value() && inst.m_opcode.m_node == inst.m_operands[0].m_type->m_node)))
    {
        Symbol *sym = table->getSymByName(inst.m_opcode.m_node);
        if (sym && (sym->getType() == SymbolType::RegisterClass || sym->getType() == SymbolType::Type))
        {
            Sema::Symbols::RuleOperandSymbol opSym;
            if (!resolveRuleOperand(collector,
                                    table,
                                    inst.m_operands[0],
                                    contextName,
                                    /*isMatchPattern=*/true,
                                    opSym))
            {
                return false;
            }
            outInst.m_operands.push_back(opSym);
            return true;
        }
    }

    // 1. Generic IR Opcode resolution
    Symbol *opcodeSym = table->getSymByName(inst.m_opcode.m_node);
    if (!opcodeSym || opcodeSym->getType() != SymbolType::IrInstruction)
    {
        collector->error(PassName,
                         "{}: Unknown or undefined generic IR opcode '{}' in match pattern",
                         contextName,
                         inst.m_opcode.m_node)
                << inst.m_opcode.m_sourceRef;
        return false;
    }

    const auto *irData = opcodeSym->getIf<Sema::Symbols::IrInstructionSymbol>();
    if (!irData)
    {
        return false;
    }

    // 2. Check that the operand count matches the IR instruction definition
    if (inst.m_operands.size() != irData->m_operands.size())
    {
        collector->error(PassName,
                         "{}: Generic IR opcode '{}' expects {} operands, but {} were provided",
                         contextName,
                         inst.m_opcode.m_node,
                         irData->m_operands.size(),
                         inst.m_operands.size())
                << inst.m_opcode.m_sourceRef;
        return false;
    }

    // 3. Resolve and validate individual operands
    bool success = true;
    for (size_t i = 0; i < inst.m_operands.size(); ++i)
    {
        const auto &operand = inst.m_operands[i];
        Sema::Symbols::RuleOperandSymbol opSym;
        if (!resolveRuleOperand(collector, table, operand, contextName, /*isMatchPattern=*/true, opSym))
        {
            success = false;
            continue;
        }

        outInst.m_operands.push_back(opSym);
    }

    return success;
}

bool InstSelPass::processEmitInstruction(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::LegalizeRuleDef::RuleInstruction &inst,
                                         std::string_view patternName,
                                         Sema::Symbols::RuleInstructionSymbol &outInst)
{
    // 1. Target instruction opcode resolution
    Symbol *opcodeSym = table->getSymByName(inst.m_opcode.m_node);
    if (!opcodeSym || opcodeSym->getType() != SymbolType::Instruction)
    {
        collector->error(PassName,
                         "Pattern '{}': Unknown or undefined target instruction '{}' in emit sequence",
                         patternName,
                         inst.m_opcode.m_node)
                << inst.m_opcode.m_sourceRef;
        return false;
    }

    const auto *targetData = opcodeSym->getIf<Sema::Symbols::TargetInstructionSymbol>();
    if (!targetData)
    {
        return false;
    }

    // 2. Check that the argument count matches the target instruction formal parameters
    if (inst.m_operands.size() != targetData->m_args.size())
    {
        collector->error(PassName,
                         "Pattern '{}': Target instruction '{}' expects {} arguments, but {} were provided",
                         patternName,
                         inst.m_opcode.m_node,
                         targetData->m_args.size(),
                         inst.m_operands.size())
                << inst.m_opcode.m_sourceRef;
        return false;
    }

    // 3. Resolve and check operand type/kind compatibility
    bool success = true;
    for (size_t i = 0; i < inst.m_operands.size(); ++i)
    {
        const auto &operand = inst.m_operands[i];
        const auto &expectedArg = targetData->m_args[i];

        Sema::Symbols::RuleOperandSymbol opSym;
        if (!resolveRuleOperand(collector, table, operand, patternName, /*isMatchPattern=*/false, opSym))
        {
            success = false;
            continue;
        }

        // Register argument verification
        if (expectedArg.m_kind == DSL::Ast::InstDef::InstOperandKind::Register)
        {
            if (operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral)
            {
                collector->error(PassName,
                                 "Pattern '{}', instruction '{}', operand '{}': Expected register operand for "
                                 "'{}', got immediate literal",
                                 patternName,
                                 inst.m_opcode.m_node,
                                 operand.m_name.m_node,
                                 expectedArg.m_name)
                        << operand.m_name.m_sourceRef;
                success = false;
            }
            else if (opSym.m_typeOrClassId.has_value() && expectedArg.m_typeOrClassId != InvalidSymbolId)
            {
                Symbol *actualSym = table->getSymById(*opSym.m_typeOrClassId);
                Symbol *expectedSym = table->getSymById(expectedArg.m_typeOrClassId);
                if (actualSym && expectedSym && actualSym->getId() != expectedSym->getId())
                {
                    // Allow addressing mode symbols to match instruction address operands
                    if (actualSym->getType() != SymbolType::AddrMode)
                    {
                        collector->error(PassName,
                                         "Pattern '{}', instruction '{}', operand '{}': Register class/type "
                                         "mismatch (expected '{}', got '{}')",
                                         patternName,
                                         inst.m_opcode.m_node,
                                         operand.m_name.m_node,
                                         expectedSym->getName(),
                                         actualSym->getName())
                                << operand.m_name.m_sourceRef;
                        success = false;
                    }
                }
            }
        }
        // Immediate argument verification
        else if (expectedArg.m_kind == DSL::Ast::InstDef::InstOperandKind::Immediate)
        {
            if (opSym.m_typeOrClassId.has_value())
            {
                Symbol *actualSym = table->getSymById(*opSym.m_typeOrClassId);
                if (actualSym && actualSym->getType() == SymbolType::RegisterClass)
                {
                    collector->error(PassName,
                                     "Pattern '{}', instruction '{}', operand '{}': Expected immediate operand for "
                                     "'{}', got register class '{}'",
                                     patternName,
                                     inst.m_opcode.m_node,
                                     operand.m_name.m_node,
                                     expectedArg.m_name,
                                     actualSym->getName())
                            << operand.m_name.m_sourceRef;
                    success = false;
                }
                else if (actualSym && expectedArg.m_typeOrClassId != InvalidSymbolId)
                {
                    Symbol *expectedSym = table->getSymById(expectedArg.m_typeOrClassId);
                    if (expectedSym && actualSym->getId() != expectedSym->getId())
                    {
                        collector->error(PassName,
                                         "Pattern '{}', instruction '{}', operand '{}': Immediate type mismatch "
                                         "(expected '{}', got '{}')",
                                         patternName,
                                         inst.m_opcode.m_node,
                                         operand.m_name.m_node,
                                         expectedSym->getName(),
                                         actualSym->getName())
                                << operand.m_name.m_sourceRef;
                        success = false;
                    }
                }
            }
        }

        outInst.m_operands.push_back(opSym);
    }

    return success;
}

bool InstSelPass::processPredicate(DiagnosticCollector *collector,
                                   SymbolTable *table,
                                   const DSL::Ast::LegalizeRuleDef::RulePredicate &predicate,
                                   std::string_view contextName)
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
                                 "{}: Undefined variable '${}' in predicate '{}'",
                                 contextName,
                                 ident.m_node,
                                 predicate.m_predicateName.m_node)
                        << ident.m_sourceRef;
                success = false;
            }
        }
    }

    return success;
}

bool InstSelPass::resolveRuleOperand(DiagnosticCollector *collector,
                                     SymbolTable *table,
                                     const DSL::Ast::LegalizeRuleDef::RuleOperand &operand,
                                     std::string_view contextName,
                                     bool isMatchPattern,
                                     Sema::Symbols::RuleOperandSymbol &outOperand)
{
    outOperand.m_kind = operand.m_kind;
    outOperand.m_name = operand.m_name.m_node;
    outOperand.m_typeOrClassId = std::nullopt;
    outOperand.m_immLiteral = std::nullopt;

    // 1. Literal Integer Constant
    if (operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateLiteral)
    {
        if (operand.m_immLiteral.has_value())
        {
            outOperand.m_immLiteral = operand.m_immLiteral->m_node;
        }
        return true;
    }

    // 2. Custom Transform (e.g. log2($c))
    if (operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::CustomTransform)
    {
        if (isMatchPattern)
        {
            collector->error(PassName,
                             "{}: Custom transform '{}' cannot appear inside a match pattern",
                             contextName,
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
                                 "{}: Undefined variable '${}' passed to transform '{}'",
                                 contextName,
                                 argIdent.m_node,
                                 operand.m_name.m_node)
                        << argIdent.m_sourceRef;
                success = false;
            }
        }
        return success;
    }

    // 3. Type, Register Class, or AddrMode Resolution
    if (operand.m_type.has_value())
    {
        const auto &typeName = *operand.m_type;
        const bool isImm = (typeName.m_node == "imm" || typeName.m_node == "simm" || typeName.m_node == "uimm");

        if (isImm)
        {
            if (operand.m_typeParam.has_value())
            {
                Symbol *paramSym = table->getSymByName(operand.m_typeParam->m_node);
                if (!paramSym || paramSym->getType() != SymbolType::Type)
                {
                    collector->error(PassName,
                                     "{}: Unknown immediate type '{}' on operand '${}'",
                                     contextName,
                                     operand.m_typeParam->m_node,
                                     operand.m_name.m_node)
                            << operand.m_typeParam->m_sourceRef;
                    return false;
                }
                outOperand.m_typeOrClassId = paramSym->getId();
            }
        }
        else
        {
            Symbol *sym = table->getSymByName(typeName.m_node);
            if (!sym ||
                (sym->getType() != SymbolType::RegisterClass && sym->getType() != SymbolType::Type &&
                 sym->getType() != SymbolType::AddrMode))
            {
                collector->error(PassName,
                                 "{}: Unknown register class, type, or addressing mode '{}' on operand '${}'",
                                 contextName,
                                 typeName.m_node,
                                 operand.m_name.m_node)
                        << typeName.m_sourceRef;
                return false;
            }
            outOperand.m_typeOrClassId = sym->getId();
        }
    }

    // 4. SSA Variable Registration
    if (operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::SsaRegister ||
        operand.m_kind == DSL::Ast::LegalizeRuleDef::OperandKind::ImmediateSymbol)
    {
        Symbol *existingVar = table->getSymByName(operand.m_name.m_node);
        if (!existingVar)
        {
            table->declareSym(operand.m_name.m_sourceRef,
                              SymbolFlags::IsDefined,
                              SymbolType::SsaVariable,
                              std::monostate{},
                              operand.m_name.m_node);
        }
    }

    return true;
}