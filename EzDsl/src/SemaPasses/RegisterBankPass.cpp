#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "SemaPasses/RegisterBankPass.h"

constexpr auto PassName = "Sema::RegisterBankPass";

bool RegisterBankPass::run(DiagnosticCollector *collector, SymbolTable *table, DSL::Ast::TargetDef::TargetDef *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    const auto &targetName = file->m_name;
    collector->trace(PassName, "Running semantic validation for {} register banks", targetName.m_node);

    // Phase 1: Declare all banks, classes, and registers
    if (!declareBanks(collector, table, file))
    {
        return false;
    }

    // Phase 2: Resolve parent registers and sub-register bit offsets/sizes
    collector->trace(PassName, "Resolving register hierarchies");
    if (!resolveHierarchies(collector, table, file))
    {
        return false;
    }

    collector->trace(PassName, "Running cycle detection on sub-register graph");
    if (!detectRegisterCycles(collector, table))
    {
        return false;
    }

    return true;
}

bool RegisterBankPass::declareBanks(DiagnosticCollector *collector,
                                    SymbolTable *table,
                                    DSL::Ast::TargetDef::TargetDef *file)
{
    bool success = true;

    const auto &targetName = file->m_name;
    Symbol *targetSym = table->getSymByName(targetName.m_node);
    if (!targetSym)
    {
        Sema::Symbols::TargetSymbol targetData{ .m_name = targetName.m_node,
                                                .m_banks = std::pmr::vector<SymbolId>{ table->getAllocator() },
                                                .m_instructions = std::pmr::vector<SymbolId>{ table->getAllocator() },
                                                .m_callingConvs = std::pmr::vector<SymbolId>{ table->getAllocator() },
                                                .m_legalizeActions =
                                                        std::pmr::vector<SymbolId>{ table->getAllocator() },
                                                .m_iselPatterns =
                                                        std::pmr::vector<SymbolId>{ table->getAllocator() } };

        SymbolId targetSymId = table->declareSym(targetName.m_sourceRef,
                                                 SymbolFlags::IsDefined,
                                                 SymbolType::Target,
                                                 std::move(targetData),
                                                 targetName.m_node);
        targetSym = table->getSymById(targetSymId);
    }
    auto *targetSymData = targetSym ? targetSym->getIf<Sema::Symbols::TargetSymbol>() : nullptr;

    for (const auto &bank : file->m_regBanks)
    {
        const auto &bankNameIdentifier = bank.m_name;

        Sema::Symbols::RegisterBankSymbol bankSym{ .m_name = bankNameIdentifier.m_node,
                                                   .m_classes = std::pmr::vector<SymbolId>{ table->getAllocator() } };

        SymbolId bankSymId = table->declareSym(bankNameIdentifier.m_sourceRef,
                                               SymbolFlags::IsDefined,
                                               SymbolType::RegisterBank,
                                               std::move(bankSym),
                                               bankNameIdentifier.m_node);

        if (bankSymId == InvalidSymbolId)
        {
            collector->error(PassName, "Redefinition of register bank '{}'", bankNameIdentifier.m_node)
                    << bankNameIdentifier.m_sourceRef;
            success = false;
            continue;
        }

        if (targetSymData)
        {
            targetSymData->m_banks.push_back(bankSymId);
        }

        Symbol *registeredBank = table->getSymById(bankSymId);
        if (!runOnBank(collector, table, &bank, bankSymId))
        {
            collector->error(PassName, "Semantic errors encountered in register bank '{}'", bankNameIdentifier.m_node)
                    << bankNameIdentifier.m_sourceRef;
            success = false;
        }

        collector->trace(PassName, "Defined bank {}", registeredBank->getName());
    }

    return success;
}

bool RegisterBankPass::runOnBank(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 const DSL::Ast::TargetDef::TargetRegisterBank *bank,
                                 SymbolId bankSymId)
{
    bool success = true;
    Symbol *bankSymbol = table->getSymById(bankSymId);
    auto *bankData = bankSymbol ? bankSymbol->getIf<Sema::Symbols::RegisterBankSymbol>() : nullptr;

    for (const auto &_class : bank->m_classes)
    {
        const auto &classNameIdentifier = _class.m_name;

        Sema::Symbols::RegisterClassSymbol classSym{ .m_name = classNameIdentifier.m_node,
                                                     .m_bankId = bankSymId,
                                                     .m_registers =
                                                             std::pmr::vector<SymbolId>{ table->getAllocator() } };

        SymbolId classSymId = table->declareSym(classNameIdentifier.m_sourceRef,
                                                SymbolFlags::IsDefined,
                                                SymbolType::RegisterClass,
                                                std::move(classSym),
                                                classNameIdentifier.m_node);

        if (classSymId == InvalidSymbolId)
        {
            collector->error(PassName, "Redefinition of register class '{}'", classNameIdentifier.m_node)
                    << classNameIdentifier.m_sourceRef;
            success = false;
            continue;
        }

        Symbol *registeredClass = table->getSymById(classSymId);
        if (bankData)
        {
            bankData->m_classes.push_back(classSymId);
        }

        if (!runOnClass(collector, table, &_class, classSymId, bankSymId))
        {
            collector->error(PassName, "Semantic errors encountered in register class '{}'", classNameIdentifier.m_node)
                    << classNameIdentifier.m_sourceRef;
            success = false;
        }

        collector->trace(PassName, "Defined class {}", registeredClass->getName());
    }

    return success;
}

bool RegisterBankPass::runOnClass(DiagnosticCollector *collector,
                                  SymbolTable *table,
                                  const DSL::Ast::TargetDef::TargetRegisterClass *_class,
                                  SymbolId classSymId,
                                  SymbolId bankSymId)
{
    bool success = true;
    Symbol *classSymbol = table->getSymById(classSymId);
    auto *classData = classSymbol ? classSymbol->getIf<Sema::Symbols::RegisterClassSymbol>() : nullptr;

    for (const auto &reg : _class->m_registers)
    {
        const auto &regNameIdentifier = reg.m_name;

        Sema::Symbols::RegisterSymbol regSym{ .m_name = regNameIdentifier.m_node,
                                              .m_parentId = std::nullopt,
                                              .m_bitSize = static_cast<uint32_t>(reg.m_size.m_node),
                                              .m_bitOffset = static_cast<uint32_t>(reg.m_offset.m_node),
                                              .m_primaryClassId = classSymId };

        SymbolId regSymId = table->declareSym(regNameIdentifier.m_sourceRef,
                                              SymbolFlags::IsDefined,
                                              SymbolType::Register,
                                              std::move(regSym),
                                              regNameIdentifier.m_node);

        if (regSymId == InvalidSymbolId)
        {
            collector->error(PassName, "Redefinition of register '{}'", regNameIdentifier.m_node)
                    << regNameIdentifier.m_sourceRef;
            success = false;
            continue;
        }

        Symbol *registeredReg = table->getSymById(regSymId);
        if (classData)
        {
            classData->m_registers.push_back(regSymId);
        }

        collector->trace(PassName, "Defined register {}", registeredReg->getName());
    }

    return success;
}

bool RegisterBankPass::resolveHierarchies(DiagnosticCollector *collector,
                                          SymbolTable *table,
                                          DSL::Ast::TargetDef::TargetDef *file)
{
    bool success = true;

    for (const auto &bank : file->m_regBanks)
    {
        for (const auto &_class : bank.m_classes)
        {
            for (const auto &reg : _class.m_registers)
            {
                // If register has no parent, it is a root register
                if (reg.m_parentName.m_node.empty())
                {
                    continue;
                }

                Symbol *regSym = table->getSymByName(reg.m_name.m_node);
                Symbol *parentSym = table->getSymByName(reg.m_parentName.m_node);

                if (!parentSym || parentSym->getType() != SymbolType::Register)
                {
                    collector->error(PassName,
                                     "Register '{}' specifies unknown parent register '{}'",
                                     reg.m_name.m_node,
                                     reg.m_parentName.m_node)
                            << reg.m_parentName.m_sourceRef;
                    success = false;
                    continue;
                }

                auto *regData = regSym ? regSym->getIf<Sema::Symbols::RegisterSymbol>() : nullptr;
                auto *parentData = parentSym->getIf<Sema::Symbols::RegisterSymbol>();

                if (!regData || !parentData)
                {
                    continue;
                }

                // Sub-register bit range check
                if (regData->m_bitOffset + regData->m_bitSize > parentData->m_bitSize)
                {
                    collector->error(
                            PassName,
                            "Sub-register '{}' [offset: {}, size: {}] exceeds parent register '{}' bitwidth ({})",
                            reg.m_name.m_node,
                            regData->m_bitOffset,
                            regData->m_bitSize,
                            parentData->m_name,
                            parentData->m_bitSize)
                            << reg.m_offset.m_sourceRef;
                    success = false;
                    continue;
                }

                // Bind resolved parent SymbolId
                regData->m_parentId = parentSym->getId();
            }
        }
    }

    return success;
}

bool RegisterBankPass::detectRegisterCycles(DiagnosticCollector *collector, SymbolTable *table)
{
    std::pmr::unordered_map<SymbolId, std::pmr::vector<SymbolId>> parentToChildren(table->getAllocator());
    std::pmr::vector<SymbolId> allRegIds{ table->getAllocator() };

    for (const Symbol *sym : table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::Register)
        {
            SymbolId regId = sym->getId();
            allRegIds.push_back(regId);

            const auto *regData = sym->getIf<Sema::Symbols::RegisterSymbol>();
            if (regData && regData->m_parentId.has_value())
            {
                parentToChildren[*regData->m_parentId].push_back(regId);
            }
        }
    }

    // 1.2 Run DFS Cycle Detection (White = 0/Unvisited, Gray = 1/Visiting, Black = 2/Visited)
    enum class NodeColor : uint8_t
    {
        White = 0,
        Gray,
        Black
    };
    std::unordered_map<SymbolId, NodeColor> colors;
    for (SymbolId id : allRegIds)
    {
        colors[id] = NodeColor::White;
    }

    bool hasCycle = false;

    auto dfs = [&](auto &self, SymbolId currentId) -> void
    {
        colors[currentId] = NodeColor::Gray;

        auto it = parentToChildren.find(currentId);
        if (it != parentToChildren.end())
        {
            for (SymbolId childId : it->second)
            {
                if (colors[childId] == NodeColor::Gray)
                {
                    Symbol *childSym = table->getSymById(childId);
                    Symbol *parentSym = table->getSymById(currentId);

                    collector->error(PassName,
                                     "Circular sub-register dependency detected: '{}' and '{}' form a cycle",
                                     parentSym->getName(),
                                     childSym->getName())
                            << childSym->getSourceRef();
                    hasCycle = true;
                }
                else if (colors[childId] == NodeColor::White)
                {
                    self(self, childId);
                }
            }
        }

        colors[currentId] = NodeColor::Black;
    };

    for (SymbolId regId : allRegIds)
    {
        if (colors[regId] == NodeColor::White)
        {
            dfs(dfs, regId);
        }
    }

    return !hasCycle;
}