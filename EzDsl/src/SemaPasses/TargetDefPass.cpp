#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"
#include "SemaPasses/TargetDefPass.h"

constexpr auto PassName = "Sema::TargetDefPass";

bool TargetDefPass::run(DiagnosticCollector *collector, SymbolTable *table, DSL::Ast::TargetDef::TargetDef *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    const auto &targetName = file->m_name;
    collector->trace(PassName, "Declaring target architecture symbol '{}'", targetName.m_node);

    // Check for collision with existing symbols in the symbol table
    Symbol *existingSym = table->getSymByName(targetName.m_node);
    if (existingSym)
    {
        if (existingSym->getType() == SymbolType::Target)
        {
            collector->error(PassName, "Redefinition of target architecture '{}'", targetName.m_node)
                    << targetName.m_sourceRef;
            return false;
        }

        collector->error(PassName, "Target architecture name '{}' collides with existing symbol", targetName.m_node)
                << targetName.m_sourceRef;
            return false;
    }

    Sema::Symbols::TargetSymbol targetData{
        .m_name = targetName.m_node,
        .m_banks = std::pmr::vector<SymbolId>{ table->getAllocator() },
        .m_instructions = std::pmr::vector<SymbolId>{ table->getAllocator() },
        .m_callingConvs = std::pmr::vector<SymbolId>{ table->getAllocator() },
        .m_legalizeActions = std::pmr::vector<SymbolId>{ table->getAllocator() },
        .m_iselPatterns = std::pmr::vector<SymbolId>{ table->getAllocator() }
    };

    SymbolId targetSymId = table->declareSym(targetName.m_sourceRef,
                                             SymbolFlags::IsDefined,
                                             SymbolType::Target,
                                             std::move(targetData),
                                             targetName.m_node);

    if (targetSymId == InvalidSymbolId)
    {
        collector->error(PassName, "Failed to declare target architecture symbol '{}'", targetName.m_node)
                << targetName.m_sourceRef;
        return false;
    }

    return true;
}
