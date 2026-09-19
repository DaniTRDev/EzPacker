#include "SemaPasses/TargetInstPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetSymbols.h"
#include <unordered_map>
#include <unordered_set>

namespace
{
constexpr auto PassName = "Sema::TargetInstPass";

namespace Enc = DSL::Ast::TargetInstDef;

/**
 * Validates an optional ENCODING block against the instruction's declared operands.
 * Returns false when an integrity rule is violated.
 */
bool validateEncoding(DiagnosticCollector *collector,
                      SymbolTable *table,
                      const Enc::TargetInstDecl &inst,
                      const Enc::EncodingDecl &enc)
{
    SourceReference *ref = inst.m_instName.m_sourceRef;
    const std::string_view instName = inst.m_instName.m_node;

    if (enc.m_form == Enc::EncForm::None)
    {
        collector->error(PassName, "Target instruction '{}': ENCODING requires a valid form", instName) << ref;
        return false;
    }

    if (enc.m_opcode.empty() || enc.m_opcode.size() > 3)
    {
        collector->error(PassName, "Target instruction '{}': ENCODING opcode must contain 1..3 bytes", instName) << ref;
        return false;
    }

    if (enc.m_opcodeDigit.has_value() && enc.m_opcodeDigit.value() > 7)
    {
        collector->error(PassName, "Target instruction '{}': ENCODING opcode_digit must be in [0,7]", instName) << ref;
        return false;
    }

    std::unordered_set<std::string_view> declaredOperands;
    for (const auto &op : inst.m_operands)
    {
        declaredOperands.insert(op.m_name.m_node);
    }

    int regCount = 0;
    int rmRegCount = 0;
    int rmMemCount = 0;
    bool hasRel = false;
    bool hasCond = false;

    for (const auto &binding : enc.m_operands)
    {
        if (!declaredOperands.contains(binding.m_name.m_node))
        {
            collector->error(PassName,
                             "Target instruction '{}': ENCODING binds unknown operand '{}'",
                             instName,
                             binding.m_name.m_node)
                    << binding.m_name.m_sourceRef;
            return false;
        }

        switch (binding.m_slot)
        {
            case Enc::EncSlotKind::Reg:
                ++regCount;
                break;
            case Enc::EncSlotKind::RmReg:
                ++rmRegCount;
                break;
            case Enc::EncSlotKind::RmMem:
                ++rmMemCount;
                break;
            case Enc::EncSlotKind::Rel8:
            case Enc::EncSlotKind::Rel32:
                hasRel = true;
                break;
            case Enc::EncSlotKind::CondCode:
                hasCond = true;
                break;
            default:
                break;
        }
    }

    if (regCount > 1 || rmRegCount > 1 || rmMemCount > 1)
    {
        collector->error(PassName,
                         "Target instruction '{}': ENCODING has more than one slot of the same register/memory kind",
                         instName)
                << ref;
        return false;
    }

    const bool isJcc = (enc.m_form == Enc::EncForm::Jcc);
    const bool isSetcc = (enc.m_form == Enc::EncForm::Setcc);
    const bool isBranch =
            (enc.m_form == Enc::EncForm::Jcc || enc.m_form == Enc::EncForm::Jmp || enc.m_form == Enc::EncForm::Call);

    if (isJcc && !enc.m_condCode.has_value())
    {
        collector->error(PassName, "Target instruction '{}': Jcc ENCODING requires a condition code", instName) << ref;
        return false;
    }

    if (isSetcc && !enc.m_condCode.has_value())
    {
        collector->error(PassName, "Target instruction '{}': SETcc ENCODING requires a condition code", instName)
                << ref;
        return false;
    }

    if (isBranch && !hasRel && enc.m_form != Enc::EncForm::Jmp && enc.m_form != Enc::EncForm::Call)
    {
        collector->error(PassName, "Target instruction '{}': branch ENCODING requires a rel32 operand", instName)
                << ref;
        return false;
    }

    (void)hasCond;
    (void)table;
    return true;
}

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

    bool hasErrors = false;
    for (const auto &inst : file->m_instructions)
    {
        if (!validateInstruction(collector, table, inst))
        {
            hasErrors = true;
        }
    }

    return !hasErrors;
}

bool TargetInstPass::validateInstruction(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::TargetInstDef::TargetInstDecl &inst)
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

    std::optional<DSL::Ast::TargetInstDef::EncodingDecl> encoding;
    if (inst.m_encoding.has_value())
    {
        if (!validateEncoding(collector, table, inst, inst.m_encoding.value()))
        {
            return false;
        }
        encoding = inst.m_encoding;
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
