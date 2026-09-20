#include "SemaPasses/TargetInstPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Encoding/EncodingDialect.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetSymbols.h"
#include <unordered_map>
#include <unordered_set>

namespace
{
constexpr auto PassName = "Sema::TargetInstPass";
} // namespace

bool TargetInstPass::run(DiagnosticCollector *collector,
                         SymbolTable *table,
                         DSL::Ast::TargetInstDef::TargetInstFile *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(PassName, "Running semantic validation for {} target instructions", file->m_instructions.size());

    const std::string_view targetName =
            file->m_targetName.has_value() ? file->m_targetName->m_node : std::string_view{};

    bool hasErrors = false;
    for (const auto &inst : file->m_instructions)
    {
        if (!validateInstruction(collector, table, inst, targetName))
        {
            hasErrors = true;
        }
    }

    return !hasErrors;
}

bool TargetInstPass::validateInstruction(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::TargetInstDef::TargetInstDecl &inst,
                                         std::string_view targetName)
{
    SourceReference *ref = inst.m_instName.m_sourceRef;

    if (table->getSymByName(inst.m_instName.m_node) != nullptr)
    {
        collector->error(PassName, "Target instruction '{}': Duplicate symbol declaration", inst.m_instName.m_node)
                << ref;
        return false;
    }

    std::unordered_set<std::string_view> seenOperandNames;
    std::pmr::vector<Symbols::TargetOperandSymbol> semaOperands{ table->getAllocator() };
    semaOperands.reserve(inst.m_operands.size());

    for (const auto &op : inst.m_operands)
    {
        if (!seenOperandNames.insert(op.m_name.m_node).second)
        {
            collector->error(PassName,
                             "Target instruction '{}': Duplicate operand name '{}'",
                             inst.m_instName.m_node,
                             op.m_name.m_node)
                    << op.m_name.m_sourceRef;
            return false;
        }

        semaOperands.push_back(Symbols::TargetOperandSymbol{ .m_regClassOrType = op.m_regClassOrType.m_node,
                                                             .m_name = op.m_name.m_node,
                                                             .m_direction = op.m_direction });
    }

    std::pmr::vector<std::string_view> flags{ table->getAllocator() };
    flags.reserve(inst.m_flags.size());
    for (const auto &f : inst.m_flags)
    {
        flags.push_back(f.m_node);
    }

    std::pmr::vector<std::string_view> implicitDefs{ table->getAllocator() };
    implicitDefs.reserve(inst.m_implicitDefs.size());
    for (const auto &d : inst.m_implicitDefs)
    {
        implicitDefs.push_back(d.m_node);
    }

    std::pmr::vector<std::string_view> implicitUses{ table->getAllocator() };
    implicitUses.reserve(inst.m_implicitUses.size());
    for (const auto &u : inst.m_implicitUses)
    {
        implicitUses.push_back(u.m_node);
    }

    std::string_view mnemonic = inst.m_mnemonic.has_value() ? inst.m_mnemonic->m_node : "";

    std::optional<DSL::Ast::Encoding::EncodingDecl> encoding;
    if (inst.m_encoding.has_value())
    {
        const auto &enc = inst.m_encoding.value();

        // Select the dialect: explicit backend key, then the file's target name, then the default.
        Sema::Encoding::EncodingDialect *dialect = nullptr;
        if (enc.m_backend.has_value())
        {
            dialect = Sema::Encoding::findEncodingDialect(enc.m_backend->m_node);
            if (!dialect)
            {
                collector->error(PassName,
                                 "Target instruction '{}': unknown encoding backend '{}'",
                                 inst.m_instName.m_node,
                                 enc.m_backend->m_node)
                        << enc.m_backend->m_sourceRef;
                return false;
            }
        }
        else
        {
            dialect = Sema::Encoding::findEncodingDialect(targetName);
            if (!dialect)
            {
                dialect = Sema::Encoding::getDefaultEncodingDialect();
            }
        }

        if (!dialect)
        {
            collector->error(PassName, "Target instruction '{}': no encoding dialect available", inst.m_instName.m_node)
                    << ref;
            return false;
        }

        if (!dialect->validate(enc, inst, collector))
        {
            return false;
        }
        encoding = enc;
    }

    Symbols::TargetInstructionSymbol data{ .m_name = inst.m_instName.m_node,
                                           .m_mnemonic = mnemonic,
                                           .m_operands = std::move(semaOperands),
                                           .m_flags = std::move(flags),
                                           .m_implicitDefs = std::move(implicitDefs),
                                           .m_implicitUses = std::move(implicitUses),
                                           .m_encoding = std::move(encoding) };

    SymbolId id = table->declareSym(ref, SymbolType::TargetInstruction, std::move(data), inst.m_instName.m_node);
    if (id == InvalidSymbolId)
    {
        collector->error(PassName, "Failed to declare target instruction '{}'", inst.m_instName.m_node) << ref;
        return false;
    }

    collector->trace(PassName, "Declared target instruction '{}'", inst.m_instName.m_node);
    return true;
}
