#include "SemaPasses/CallingConvPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"

constexpr auto PassName = "Sema::CallingConvPass";

namespace
{
bool isPowerOfTwo(uint64_t val) noexcept { return val > 0 && (val & (val - 1)) == 0; }
}; // anonymous namespace

bool CallingConvPass::run(DiagnosticCollector *collector,
                          SymbolTable *table,
                          DSL::Ast::CallingConvDef::CallingConvDefFile *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    if (file->m_name.m_node.empty())
    {
        collector->error(PassName, "Calling convention declaration requires a valid name identifier");
        return false;
    }

    const auto &ccNameIdent = file->m_name;
    collector->trace(PassName, "Running semantic validation for calling convention '{}'", ccNameIdent.m_node);

    Sema::Symbols::CallingConvSymbol ccSym{
        .m_name = ccNameIdent.m_node,
        .m_stackAlign = 16,
        .m_stackDirection = file->m_stackDirection,
        .m_stackCleanup = file->m_stackCleanup,
        .m_shadowSpace = 0,
        .m_redZone = 0,
        .m_stackPointer = {},
        .m_framePointer = {},
        .m_calleeSaved = std::pmr::vector<Sema::Symbols::RegisterRefSymbol>{ table->getAllocator() },
        .m_callerSaved = std::pmr::vector<Sema::Symbols::RegisterRefSymbol>{ table->getAllocator() },
        .m_primitiveRules = std::pmr::vector<std::pair<SymbolId, std::string_view>>{ table->getAllocator() },
        .m_aggregateDef = std::nullopt,
        .m_passRules = std::pmr::vector<Sema::Symbols::DispatchRuleSymbol>{ table->getAllocator() },
        .m_returnRules = std::pmr::vector<Sema::Symbols::DispatchRuleSymbol>{ table->getAllocator() },
        .m_sretConfig = std::nullopt
    };

    bool success = true;

    // 1. Stack Alignment Validation
    int64_t align = file->m_stackAlign.m_node;
    if (align < 0 || align > 0 && !isPowerOfTwo(static_cast<uint64_t>(align)))
    {
        collector->error(PassName,
                         "Calling convention '{}': STACK_ALIGN ({}) must be a positive power of 2",
                         ccNameIdent.m_node,
                         align)
                << file->m_stackAlign.m_sourceRef;
        success = false;
    }
    else
    {
        ccSym.m_stackAlign = static_cast<uint32_t>(align);
    }

    // 2. Shadow Space Validation
    if (file->m_shadowSpace.m_node < 0)
    {
        collector->error(PassName,
                         "Calling convention '{}': SHADOW_SPACE ({}) cannot be negative",
                         ccNameIdent.m_node,
                         file->m_shadowSpace.m_node)
                << file->m_shadowSpace.m_sourceRef;
        success = false;
    }
    else
    {
        ccSym.m_shadowSpace = static_cast<uint32_t>(file->m_shadowSpace.m_node);
    }

    // 3. Red Zone Validation
    if (file->m_redZone.m_node < 0)
    {
        collector->error(PassName,
                         "Calling convention '{}': RED_ZONE ({}) cannot be negative",
                         ccNameIdent.m_node,
                         file->m_redZone.m_node)
                << file->m_redZone.m_sourceRef;
        success = false;
    }
    else
    {
        ccSym.m_redZone = static_cast<uint32_t>(file->m_redZone.m_node);
    }

    // 4. Stack Pointer Register Resolution
    auto &stackPtr = file->m_stackPointer;
    if (!stackPtr.m_className.m_node.empty() && !stackPtr.m_regName.m_node.empty())
    {
        if (!resolveRegisterRef(collector, table, stackPtr, "STACK_POINTER", ccSym.m_stackPointer))
        {
            success = false;
        }
    }

    // 5. Frame Pointer Register Resolution
    auto framePtr = file->m_framePointer;
    if (!framePtr.m_className.m_node.empty() && !file->m_framePointer.m_regName.m_node.empty())
    {
        if (!resolveRegisterRef(collector, table, file->m_framePointer, "FRAME_POINTER", ccSym.m_framePointer))
        {
            success = false;
        }
    }

    // 6. Callee-Saved Registers Resolution & Uniqueness
    std::unordered_set<SymbolId> calleeSavedIds;
    for (const auto &regRef : file->m_calleeSaved)
    {
        Sema::Symbols::RegisterRefSymbol refSym;
        if (resolveRegisterRef(collector, table, regRef, "CALLEE_SAVED", refSym))
        {
            if (!calleeSavedIds.insert(refSym.m_registerId).second)
            {
                collector->error(PassName,
                                 "Calling convention '{}': Duplicate CALLEE_SAVED register '{}:{}'",
                                 ccNameIdent.m_node,
                                 regRef.m_className.m_node,
                                 regRef.m_regName.m_node)
                        << regRef.m_regName.m_sourceRef;
                success = false;
            }
            else
            {
                ccSym.m_calleeSaved.push_back(refSym);
            }
        }
        else
        {
            success = false;
        }
    }

    // 7. Caller-Saved Registers Resolution & Disjointness Check
    std::unordered_set<SymbolId> callerSavedIds;
    for (const auto &regRef : file->m_callerSaved)
    {
        Sema::Symbols::RegisterRefSymbol refSym;
        if (resolveRegisterRef(collector, table, regRef, "CALLER_SAVED", refSym))
        {
            if (!callerSavedIds.insert(refSym.m_registerId).second)
            {
                collector->error(PassName,
                                 "Calling convention '{}': Duplicate CALLER_SAVED register '{}:{}'",
                                 ccNameIdent.m_node,
                                 regRef.m_className.m_node,
                                 regRef.m_regName.m_node)
                        << regRef.m_regName.m_sourceRef;
                success = false;
            }
            else if (calleeSavedIds.find(refSym.m_registerId) != calleeSavedIds.end())
            {
                collector->error(
                        PassName,
                        "Calling convention '{}': Register '{}:{}' cannot be both CALLEE_SAVED and CALLER_SAVED",
                        ccNameIdent.m_node,
                        regRef.m_className.m_node,
                        regRef.m_regName.m_node)
                        << regRef.m_regName.m_sourceRef;
                success = false;
            }
            else
            {
                ccSym.m_callerSaved.push_back(refSym);
            }
        }
        else
        {
            success = false;
        }
    }

    // 8. Classification Stage
    if (!processClassification(collector, table, file->m_classify, ccNameIdent.m_node, ccSym))
    {
        success = false;
    }

    // 9. Argument Dispatch (PASS Block)
    if (!processDispatchRules(collector, table, file->m_passRules, "PASS", ccNameIdent.m_node, ccSym.m_passRules))
    {
        success = false;
    }

    // 10. Return Value Dispatch (RETURN Block)
    if (!processDispatchRules(collector, table, file->m_returnRules, "RETURN", ccNameIdent.m_node, ccSym.m_returnRules))
    {
        success = false;
    }

    // 11. SRET Configuration
    if (file->m_sretConfig.has_value())
    {
        Sema::Symbols::SretConfigSymbol sretSym;
        if (processSretConfig(collector, table, *file->m_sretConfig, ccNameIdent.m_node, sretSym))
        {
            ccSym.m_sretConfig = sretSym;
        }
        else
        {
            success = false;
        }
    }

    // 12. Register Calling Convention Symbol in Current Scope
    SymbolId ccId = table->declareSym(ccNameIdent.m_sourceRef,
                                      SymbolFlags::IsDefined,
                                      SymbolType::CallingConv,
                                      std::move(ccSym),
                                      ccNameIdent.m_node);

    if (ccId == InvalidSymbolId)
    {
        collector->error(PassName, "Redefinition of calling convention '{}'", ccNameIdent.m_node)
                << ccNameIdent.m_sourceRef;
        return false;
    }

    collector->trace(PassName, "Successfully registered calling convention '{}'", ccNameIdent.m_node);
    return success;
}

bool CallingConvPass::resolveRegisterRef(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::CallingConvDef::RegisterRef &astRef,
                                         std::string_view context,
                                         Sema::Symbols::RegisterRefSymbol &outRef)
{
    const auto &className = astRef.m_className;
    const auto &regName = astRef.m_regName;

    // 1. Resolve Register Class Symbol
    Symbol *classSym = table->getSymByName(className.m_node);
    if (!classSym || classSym->getType() != SymbolType::RegisterClass)
    {
        collector->error(PassName,
                         "{}: Unknown register class '{}' in register reference '{}:{}'",
                         context,
                         className.m_node,
                         className.m_node,
                         regName.m_node)
                << className.m_sourceRef;
        return false;
    }

    // 2. Resolve Register Symbol
    Symbol *regSym = table->getSymByName(regName.m_node);
    if (!regSym || regSym->getType() != SymbolType::Register)
    {
        collector->error(PassName,
                         "{}: Unknown register '{}' in register reference '{}:{}'",
                         context,
                         regName.m_node,
                         className.m_node,
                         regName.m_node)
                << regName.m_sourceRef;
        return false;
    }

    // 3. Verify Membership in the Specified Register Class
    const auto *classData = classSym->getIf<Sema::Symbols::RegisterClassSymbol>();
    const auto *regData = regSym->getIf<Sema::Symbols::RegisterSymbol>();

    bool isMember = false;
    if (regData && regData->m_primaryClassId == classSym->getId())
    {
        isMember = true;
    }
    else if (classData)
    {
        isMember = std::find(classData->m_registers.begin(), classData->m_registers.end(), regSym->getId()) !=
                classData->m_registers.end();
    }

    if (!isMember)
    {
        collector->error(PassName,
                         "{}: Register '{}' does not belong to register class '{}'",
                         context,
                         regName.m_node,
                         className.m_node)
                << regName.m_sourceRef;
        return false;
    }

    outRef.m_classId = classSym->getId();
    outRef.m_registerId = regSym->getId();
    return true;
}

bool CallingConvPass::processClassification(DiagnosticCollector *collector,
                                            SymbolTable *table,
                                            const DSL::Ast::CallingConvDef::ClassifyBlock &classifyBlock,
                                            std::string_view ccName,
                                            Sema::Symbols::CallingConvSymbol &ccSym)
{
    bool success = true;
    std::unordered_set<SymbolId> classifiedTypes;

    // 1. Process Scalar TYPE Classifications
    for (const auto &primRule : classifyBlock.m_primitiveRules)
    {
        const auto &targetClass = primRule.m_targetClass;

        for (const auto &typeName : primRule.m_types)
        {
            Symbol *typeSym = table->getSymByName(typeName.m_node);
            if (!typeSym || typeSym->getType() != SymbolType::Type)
            {
                collector->error(PassName,
                                 "Calling convention '{}': Unknown type '{}' in TYPE classification rule",
                                 ccName,
                                 typeName.m_node)
                        << typeName.m_sourceRef;
                success = false;
                continue;
            }

            if (!classifiedTypes.insert(typeSym->getId()).second)
            {
                collector->error(PassName,
                                 "Calling convention '{}': Type '{}' is classified multiple times",
                                 ccName,
                                 typeName.m_node)
                        << typeName.m_sourceRef;
                success = false;
                continue;
            }

            ccSym.m_primitiveRules.push_back({ typeSym->getId(), targetClass.m_node });
        }
    }

    // 2. Process AGGREGATE Configuration
    if (classifyBlock.m_aggregateDef.has_value())
    {
        Sema::Symbols::AggregateClassifySymbol aggSym;
        if (processAggregateDef(collector, table, *classifyBlock.m_aggregateDef, ccName, aggSym))
        {
            ccSym.m_aggregateDef = std::move(aggSym);
        }
        else
        {
            success = false;
        }
    }

    return success;
}

bool CallingConvPass::processAggregateDef(DiagnosticCollector *collector,
                                          SymbolTable *table,
                                          const DSL::Ast::CallingConvDef::AggregateClassifyDef &aggDef,
                                          std::string_view ccName,
                                          Sema::Symbols::AggregateClassifySymbol &outAggSym)
{
    bool success = true;

    outAggSym.m_predicates = std::pmr::vector<Sema::Symbols::AggregatePredicateSymbol>{ table->getAllocator() };
    outAggSym.m_mergePrecedence = std::pmr::vector<std::string_view>{ table->getAllocator() };
    outAggSym.m_allocPolicy = aggDef.m_allocPolicy;
    outAggSym.m_chunkSize = std::nullopt;

    // Validate CHUNK_SIZE
    if (aggDef.m_chunkSize.has_value())
    {
        int64_t chunk = aggDef.m_chunkSize->m_node;
        if (chunk <= 0 || !isPowerOfTwo(static_cast<uint64_t>(chunk)))
        {
            collector->error(PassName,
                             "Calling convention '{}': CHUNK_SIZE ({}) must be a positive power of 2",
                             ccName,
                             chunk)
                    << aggDef.m_chunkSize->m_sourceRef;
            success = false;
        }
        else
        {
            outAggSym.m_chunkSize = chunk;
        }
    }

    // Merge Precedence List
    for (const auto &abiClass : aggDef.m_mergePrecedence)
    {
        outAggSym.m_mergePrecedence.push_back(abiClass.m_node);
    }

    // Predicates
    for (const auto &pred : aggDef.m_predicates)
    {
        Sema::Symbols::AggregatePredicateSymbol predSym{ .m_kind = pred.m_kind,
                                                         .m_size = std::nullopt,
                                                         .m_sizes = std::pmr::vector<int64_t>{ table->getAllocator() },
                                                         .m_homogeneousClass = std::nullopt,
                                                         .m_maxElements = std::nullopt,
                                                         .m_resultClass = pred.m_resultClass.m_node };

        if (pred.m_size.has_value())
        {
            if (pred.m_size->m_node < 0)
            {
                collector->error(PassName,
                                 "Calling convention '{}': Aggregate size threshold cannot be negative",
                                 ccName)
                        << pred.m_size->m_sourceRef;
                success = false;
            }
            predSym.m_size = pred.m_size->m_node;
        }

        for (const auto &sizeLit : pred.m_sizes)
        {
            if (sizeLit.m_node < 0)
            {
                collector->error(PassName,
                                 "Calling convention '{}': Aggregate size list item cannot be negative",
                                 ccName)
                        << sizeLit.m_sourceRef;
                success = false;
            }
            predSym.m_sizes.push_back(sizeLit.m_node);
        }

        if (pred.m_homogeneousClass.has_value())
        {
            predSym.m_homogeneousClass = pred.m_homogeneousClass->m_node;
        }

        if (pred.m_maxElements.has_value())
        {
            if (pred.m_maxElements->m_node <= 0)
            {
                collector->error(PassName,
                                 "Calling convention '{}': IF_HOMOGENEOUS MAX elements must be positive",
                                 ccName)
                        << pred.m_maxElements->m_sourceRef;
                success = false;
            }
            predSym.m_maxElements = pred.m_maxElements->m_node;
        }

        outAggSym.m_predicates.push_back(std::move(predSym));
    }

    return success;
}

bool CallingConvPass::processDispatchRules(DiagnosticCollector *collector,
                                           SymbolTable *table,
                                           const std::pmr::vector<DSL::Ast::CallingConvDef::DispatchRule> &rules,
                                           std::string_view context,
                                           std::string_view ccName,
                                           std::pmr::vector<Sema::Symbols::DispatchRuleSymbol> &outRules)
{
    bool success = true;
    std::unordered_set<std::string_view> seenClasses;

    for (const auto &rule : rules)
    {
        const auto &abiClassName = rule.m_abiClass;
        if (!seenClasses.insert(abiClassName.m_node).second)
        {
            collector->error(PassName,
                             "Calling convention '{}', {} block: Duplicate rule for ABI class '{}'",
                             ccName,
                             context,
                             abiClassName.m_node)
                    << abiClassName.m_sourceRef;
            success = false;
        }

        Sema::Symbols::LoweringActionSymbol actionSym;
        if (!processLoweringAction(collector, table, rule.m_action, context, ccName, actionSym))
        {
            success = false;
            continue;
        }

        outRules.push_back(Sema::Symbols::DispatchRuleSymbol{ .m_abiClass = abiClassName.m_node,
                                                              .m_action = std::move(actionSym) });
    }

    return success;
}

bool CallingConvPass::processLoweringAction(DiagnosticCollector *collector,
                                            SymbolTable *table,
                                            const DSL::Ast::CallingConvDef::LoweringAction &action,
                                            std::string_view context,
                                            std::string_view ccName,
                                            Sema::Symbols::LoweringActionSymbol &outAction)
{
    outAction.m_kind = action.m_kind;
    outAction.m_regAssignKind = action.m_regAssignKind;
    outAction.m_registers = std::pmr::vector<Sema::Symbols::RegisterRefSymbol>{ table->getAllocator() };
    outAction.m_targetClass = std::nullopt;
    outAction.m_stackFallbackAlign = std::nullopt;

    bool success = true;

    // 1. Register-Based Actions (REG_SEQ, REG_SLOTS)
    if (action.m_kind == DSL::Ast::CallingConvDef::LoweringActionKind::RegisterAssign)
    {
        if (action.m_registers.empty())
        {
            collector->error(PassName,
                             "Calling convention '{}', {} block: Register assignment requires at least one register",
                             ccName,
                             context);
            return false;
        }

        for (const auto &regRef : action.m_registers)
        {
            Sema::Symbols::RegisterRefSymbol refSym;
            if (resolveRegisterRef(collector, table, regRef, context, refSym))
            {
                outAction.m_registers.push_back(refSym);
            }
            else
            {
                success = false;
            }
        }
    }

    // 2. Class Delegation Actions (EXPAND_TO, PASS_AS_POINTER)
    if (action.m_targetClass.has_value())
    {
        if (action.m_targetClass->m_node.empty())
        {
            collector->error(PassName,
                             "Calling convention '{}', {} block: Target ABI class cannot be empty",
                             ccName,
                             context)
                    << action.m_targetClass->m_sourceRef;
            success = false;
        }
        else
        {
            outAction.m_targetClass = action.m_targetClass->m_node;
        }
    }

    // 3. Stack Placement and Alignment Fallback
    if (action.m_stackFallback.has_value() && action.m_stackFallback->m_alignment.has_value())
    {
        const auto &alignLit = *action.m_stackFallback->m_alignment;
        if (alignLit.m_node <= 0 || !isPowerOfTwo(static_cast<uint64_t>(alignLit.m_node)))
        {
            collector->error(PassName,
                             "Calling convention '{}', {} block: STACK alignment ({}) must be a positive power of 2",
                             ccName,
                             context,
                             alignLit.m_node)
                    << alignLit.m_sourceRef;
            success = false;
        }
        else
        {
            outAction.m_stackFallbackAlign = static_cast<uint32_t>(alignLit.m_node);
        }
    }

    return success;
}

bool CallingConvPass::processSretConfig(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        const DSL::Ast::CallingConvDef::SretConfig &config,
                                        std::string_view ccName,
                                        Sema::Symbols::SretConfigSymbol &outSret)
{
    bool success = true;

    outSret.m_consumesArgSlot = config.m_consumesArgSlot;
    outSret.m_returnReg = std::nullopt;

    // Validate PASS_IN_REG
    if (!resolveRegisterRef(collector, table, config.m_passInReg, "SRET_CONFIG PASS_IN_REG", outSret.m_passInReg))
    {
        success = false;
    }

    // Validate optional RETURN_REG
    if (config.m_returnReg.has_value())
    {
        Sema::Symbols::RegisterRefSymbol retRegSym;
        if (resolveRegisterRef(collector, table, *config.m_returnReg, "SRET_CONFIG RETURN_REG", retRegSym))
        {
            outSret.m_returnReg = retRegSym;
        }
        else
        {
            success = false;
        }
    }

    return success;
}