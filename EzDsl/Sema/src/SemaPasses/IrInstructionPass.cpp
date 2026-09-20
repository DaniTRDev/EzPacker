#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbol.h"
#include "SemaPasses/IrInstructionPass.h"
#include "SemaPasses/PassDriver.h"

constexpr auto PassName = "Sema::IrInstructionPass"; // Pass identifier used in diagnostics.

bool IrInstructionPass::run(DiagnosticCollector *collector,
                            SymbolTable *table,
                            DSL::Ast::IrInstDef::IrInstDefFile *file)
{
    if (!Sema::preparePass(collector, table, file, PassName))
    {
        return false;
    }

    collector->trace(PassName, "Running semantic validation for {} IR instructions", file->m_instructions.size());

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

/**
 * Validates the operand signature and tallies input/output counts for later flag checks.
 */
bool IrInstructionPass::validateOperands(DiagnosticCollector *collector,
                                         const DSL::Ast::IrInstDef::IrInstDecl &inst,
                                         size_t &numIn,
                                         size_t &numOut)
{
    bool valid = true;
    SourceReference *ref = inst.m_name.m_sourceRef;
    std::unordered_set<std::string_view> seenOperandNames;
    numIn = 0;
    numOut = 0;

    for (const auto &op : inst.m_operands)
    {
        // Check for duplicate operand identifiers within the same signature
        if (!seenOperandNames.insert(op.m_name.m_node).second)
        {
            collector->error(PassName,
                             "Instruction '{}': Duplicate operand name '{}'",
                             inst.m_name.m_node,
                             op.m_name.m_node)
                    << ref;
            valid = false;
        }

        // Immediates are read-only and cannot be outputs
        const bool isImmutableValue = op.m_type == DSL::Ast::IrInstDef::IrOperandType::Immediate;

        if (isImmutableValue &&
            (op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgOut ||
             op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgInOut))
        {
            collector->error(PassName,
                             "Instruction '{}': Immediate operand '{}' cannot be marked OUT or INOUT",
                             inst.m_name.m_node,
                             op.m_name.m_node)
                    << ref;
            valid = false;
        }

        // Tally dataflow directions
        if (op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgIn ||
            op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgInOut)
        {
            ++numIn;
        }
        if (op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgOut ||
            op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgInOut)
        {
            ++numOut;
        }
    }

    return valid;
}

/**
 * Enforces category/flag invariants that cannot be expressed by the grammar itself.
 */
bool IrInstructionPass::validateFlagsAndCategory(DiagnosticCollector *collector,
                                                 const DSL::Ast::IrInstDef::IrInstDecl &inst,
                                                 DSL::Ast::IrInstDef::IrInstFlag combinedFlags,
                                                 size_t numIn,
                                                 size_t numOut)
{
    using namespace DSL::Ast::IrInstDef;
    bool valid = true;
    SourceReference *ref = inst.m_name.m_sourceRef;

    // Convenience predicate over the aggregated flag bitmask.
    auto hasFlag = [&](IrInstFlag flag)
    { return (static_cast<uint32_t>(combinedFlags) & static_cast<uint32_t>(flag)) != 0; };

    if (inst.m_body.m_category == IrInstCategory::Invalid)
    {
        collector->error(PassName, "Instruction '{}': Missing or Invalid CATEGORY", inst.m_name.m_node) << ref;
        valid = false;
    }

    // 2. Control flow terminator consistency
    const bool isBranch = hasFlag(IrInstFlag::IsBranch);
    const bool isReturn = hasFlag(IrInstFlag::IsReturn);
    const bool isCall = hasFlag(IrInstFlag::IsCall);
    const bool isTerminator = hasFlag(IrInstFlag::IsTerminator);

    if ((isBranch || isReturn) && !isTerminator)
    {
        collector->error(PassName,
                         "Instruction '{}': Flag 'IsBranch' or 'IsReturn' requires 'IsTerminator'",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // Mutually exclusive control flow roles
    if ((isBranch && isCall) || (isBranch && isReturn) || (isCall && isReturn))
    {
        collector->error(PassName,
                         "Instruction '{}': Flags 'IsBranch', 'IsCall', and 'IsReturn' are mutually exclusive",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // Commutativity validation
    if (hasFlag(IrInstFlag::IsCommutative) && numIn < 2)
    {
        collector->error(PassName,
                         "Instruction '{}': Flag 'IsCommutative' requires at least 2 IN operands (found {})",
                         inst.m_name.m_node,
                         numIn)
                << ref;
        valid = false;
    }

    // Size constraint mutual exclusivity
    const int sizeFlagsCount = (hasFlag(IrInstFlag::SizeMatch) ? 1 : 0) + (hasFlag(IrInstFlag::DestLarger) ? 1 : 0) +
            (hasFlag(IrInstFlag::DestSmaller) ? 1 : 0);
    if (sizeFlagsCount > 1)
    {
        collector->error(PassName,
                         "Instruction '{}': Flags 'SizeMatch', 'DestLarger', and 'DestSmaller' are mutually exclusive",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // 6. Casting operand requirements
    if ((hasFlag(IrInstFlag::DestLarger) || hasFlag(IrInstFlag::DestSmaller)) && (numIn < 1 || numOut < 1))
    {
        collector->error(PassName,
                         "Instruction '{}': Casting operations ('DestLarger'/'DestSmaller') require at least 1 IN and "
                         "1 OUT operand",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // Dead computation guard: Pure ALU/Casting ops must write to an output
    const bool isPureCompute =
            (inst.m_body.m_category == IrInstCategory::Arithmetic ||
             inst.m_body.m_category == IrInstCategory::Bitwise || inst.m_body.m_category == IrInstCategory::Compare ||
             inst.m_body.m_category == IrInstCategory::Casting);

    if (isPureCompute && numOut == 0 && !hasFlag(IrInstFlag::HasSideEffect))
    {
        collector->error(PassName,
                         "Instruction '{}': Pure computational instruction produces no output register and lacks "
                         "'HasSideEffect'",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    return valid;
}

bool IrInstructionPass::validateInstruction(DiagnosticCollector *collector,
                                            SymbolTable *table,
                                            const DSL::Ast::IrInstDef::IrInstDecl &inst)
{
    SourceReference *ref = inst.m_name.m_sourceRef;

    // Pre-declaration collision check
    if (Symbol *sym = table->getSymByName(inst.m_name.m_node); sym != nullptr)
    {
        auto err = collector->error(PassName, "Duplicate IR instruction symbol '{}'", inst.m_name.m_node);
        err << ref;
        err.appendNote(sym->getSourceRef(), "Previous decl");

        return false;
    }

    // Validate operand list
    size_t numIn = 0;
    size_t numOut = 0;
    bool operandsValid = validateOperands(collector, inst, numIn, numOut);

    // Aggregate flags into a bitmask
    uint32_t rawFlags = 0;
    for (const auto &flag : inst.m_body.m_flags)
        rawFlags |= static_cast<uint32_t>(flag);

    auto combinedFlags = static_cast<DSL::Ast::IrInstDef::IrInstFlag>(rawFlags);

    // Validate category and flag semantics
    bool flagsValid = validateFlagsAndCategory(collector, inst, combinedFlags, numIn, numOut);

    if (!operandsValid || !flagsValid)
    {
        return false;
    }

    // Convert AST operands to resolved semantic operand symbols
    std::pmr::vector<Symbols::IrOperandSymbol> semaOperands{ table->getAllocator() };
    semaOperands.reserve(inst.m_operands.size());

    for (const auto &op : inst.m_operands)
    {
        semaOperands.push_back(
                Symbols::IrOperandSymbol{ .m_type = op.m_type, .m_name = op.m_name.m_node, .m_dir = op.m_dir });
    }

    Symbols::IrInstructionSymbol data{ .m_name = inst.m_name.m_node,
                                       .m_category = inst.m_body.m_category,
                                       .m_tier = inst.m_body.m_tier,
                                       .m_flags = combinedFlags,
                                       .m_operands = std::move(semaOperands) };

    SymbolId id = table->declareSym(ref, SymbolType::IrInstruction, std::move(data), inst.m_name.m_node);

    if (id == InvalidSymbolId)
    {
        collector->error(PassName, "Duplicate IR instruction symbol declaration '{}'", inst.m_name.m_node) << ref;
        return false;
    }

    collector->trace(PassName, "Added IR instruction: {}", inst.m_name.m_node);
    return true;
}