#include "SemaPasses/IrInstructionPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/IrInstructionSymbol.h"

bool IrInstructionPass::run(DiagnosticCollector *collector,
                            SymbolTable *table,
                            DSL::Ast::IrInstDef::IrInstDefFile *file)
{
    constexpr auto passName = "Sema::IrInstructionPass";

    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(passName, "Running semantic validation for {} IR instructions", file->m_instructions.size());

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

bool IrInstructionPass::validateOperands(DiagnosticCollector *collector,
                                         const DSL::Ast::IrInstDef::IrInstDecl &inst,
                                         size_t &numIn,
                                         size_t &numOut)
{
    constexpr auto passName = "Sema::IrInstructionPass";
    bool valid = true;
    SourceReference *ref = inst.m_name.m_sourceRef;
    std::unordered_set<std::string_view> seenOperandNames;
    numIn = 0;
    numOut = 0;

    for (const auto &op : inst.m_operands)
    {
        // 1. Check for duplicate operand identifiers within the same signature
        if (!seenOperandNames.insert(op.m_name.m_node).second)
        {
            collector->error(passName,
                             "Instruction '{}': Duplicate operand name '{}'",
                             inst.m_name.m_node,
                             op.m_name.m_node)
                    << ref;
            valid = false;
        }

        // 2. Immediates, Symbols, and Block references are read-only and cannot be outputs
        const bool isImmutableValue = (op.m_type == DSL::Ast::IrInstDef::IrOperandType::Integer ||
                                       op.m_type == DSL::Ast::IrInstDef::IrOperandType::FloatingPoint ||
                                       op.m_type == DSL::Ast::IrInstDef::IrOperandType::Immediate ||
                                       op.m_type == DSL::Ast::IrInstDef::IrOperandType::Reference ||
                                       op.m_type == DSL::Ast::IrInstDef::IrOperandType::RuntimeSymbol);

        if (isImmutableValue &&
            (op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgOut ||
             op.m_dir == DSL::Ast::IrInstDef::IrOperandDir::ArgInOut))
        {
            collector->error(passName,
                             "Instruction '{}': Immediate/Reference operand '{}' cannot be marked OUT or INOUT",
                             inst.m_name.m_node,
                             op.m_name.m_node)
                    << ref;
            valid = false;
        }

        // 3. Tally dataflow directions
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

bool IrInstructionPass::validateFlagsAndCategory(DiagnosticCollector *collector,
                                                 const DSL::Ast::IrInstDef::IrInstDecl &inst,
                                                 DSL::Ast::IrInstDef::IrInstFlag combinedFlags,
                                                 size_t numIn,
                                                 size_t numOut)
{
    using namespace DSL::Ast::IrInstDef;
    constexpr auto passName = "Sema::IrInstructionPass";
    bool valid = true;
    SourceReference *ref = inst.m_name.m_sourceRef;

    auto hasFlag = [&](IrInstFlag flag)
    { return (static_cast<uint32_t>(combinedFlags) & static_cast<uint32_t>(flag)) != 0; };

    // 1. Mandatory category specification
    if (inst.m_body.m_category == IrInstCategory::Invalid)
    {
        collector->error(passName, "Instruction '{}': Missing or Invalid CATEGORY", inst.m_name.m_node) << ref;
        valid = false;
    }

    // 2. Control flow terminator consistency
    const bool isBranch = hasFlag(IrInstFlag::IsBranch);
    const bool isReturn = hasFlag(IrInstFlag::IsReturn);
    const bool isCall = hasFlag(IrInstFlag::IsCall);
    const bool isTerminator = hasFlag(IrInstFlag::IsTerminator);

    if ((isBranch || isReturn) && !isTerminator)
    {
        collector->error(passName,
                         "Instruction '{}': Flag 'IsBranch' or 'IsReturn' requires 'IsTerminator'",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // 3. Mutually exclusive control flow roles
    if ((isBranch && isCall) || (isBranch && isReturn) || (isCall && isReturn))
    {
        collector->error(passName,
                         "Instruction '{}': Flags 'IsBranch', 'IsCall', and 'IsReturn' are mutually exclusive",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // 4. Commutativity validation
    if (hasFlag(IrInstFlag::IsCommutative) && numIn < 2)
    {
        collector->error(passName,
                         "Instruction '{}': Flag 'IsCommutative' requires at least 2 IN operands (found {})",
                         inst.m_name.m_node,
                         numIn)
                << ref;
        valid = false;
    }

    // 5. Size constraint mutual exclusivity
    const int sizeFlagsCount = (hasFlag(IrInstFlag::SizeMatch) ? 1 : 0) + (hasFlag(IrInstFlag::DestLarger) ? 1 : 0) +
            (hasFlag(IrInstFlag::DestSmaller) ? 1 : 0);
    if (sizeFlagsCount > 1)
    {
        collector->error(passName,
                         "Instruction '{}': Flags 'SizeMatch', 'DestLarger', and 'DestSmaller' are mutually exclusive",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // 6. Casting operand requirements
    if ((hasFlag(IrInstFlag::DestLarger) || hasFlag(IrInstFlag::DestSmaller)) && (numIn < 1 || numOut < 1))
    {
        collector->error(passName,
                         "Instruction '{}': Casting operations ('DestLarger'/'DestSmaller') require at least 1 IN and "
                         "1 OUT operand",
                         inst.m_name.m_node)
                << ref;
        valid = false;
    }

    // 7. Dead computation guard: Pure ALU/Casting ops must write to an output
    const bool isPureCompute =
            (inst.m_body.m_category == IrInstCategory::Arithmetic ||
             inst.m_body.m_category == IrInstCategory::Bitwise || inst.m_body.m_category == IrInstCategory::Compare ||
             inst.m_body.m_category == IrInstCategory::Casting);

    if (isPureCompute && numOut == 0 && !hasFlag(IrInstFlag::HasSideEffect))
    {
        collector->error(passName,
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
    constexpr auto passName = "Sema::IrInstructionPass";
    SourceReference *ref = inst.m_name.m_sourceRef;

    // 1. Symbol collision check
    if (table->getSymByName(inst.m_name.m_node) != nullptr)
    {
        collector->error(passName, "Duplicate IR instruction symbol '{}'", inst.m_name.m_node) << ref;
        return false;
    }

    // 2. Validate operands
    size_t numIn = 0;
    size_t numOut = 0;
    bool operandsValid = validateOperands(collector, inst, numIn, numOut);

    // 3. Aggregate instruction flags
    uint32_t rawFlags = 0;
    for (const auto &flag : inst.m_body.m_flags)
    {
        rawFlags |= static_cast<uint32_t>(flag);
    }
    auto combinedFlags = static_cast<DSL::Ast::IrInstDef::IrInstFlag>(rawFlags);

    // 4. Validate category and flag invariants
    bool flagsValid = validateFlagsAndCategory(collector, inst, combinedFlags, numIn, numOut);

    if (!operandsValid || !flagsValid)
    {
        return false;
    }

    // 5. Register instruction symbol in the SymbolTable
    Sema::Symbols::IrInstructionSymbol data{ .m_category = inst.m_body.m_category,
                                             .m_tier = inst.m_body.m_tier,
                                             .m_flags = combinedFlags,
                                             .m_operands = inst.m_operands };

    SymbolId id = table->declareSym(ref,
                                    SymbolFlags::IsDefined,
                                    SymbolType::IrInstruction,
                                    std::move(data),
                                    inst.m_name.m_node);

    if (id == InvalidSymbolId)
    {
        collector->error(passName, "Duplicate IR instruction symbol declaration '{}'", inst.m_name.m_node) << ref;
        return false;
    }

    collector->trace(passName, "Added IR instruction: {}", inst.m_name.m_node);
    return true;
}