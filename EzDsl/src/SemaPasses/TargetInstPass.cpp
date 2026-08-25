#include "SemaPasses/TargetInstPass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/Symbols.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

constexpr auto PassName = "Sema::InstructionDefPass";

namespace
{

struct OperandLookupInfo
{
    SymbolId m_typeOrClassId{ InvalidSymbolId };
    uint32_t m_bitWidth{ 0 };
};

uint64_t foldConstantOp(DSL::Ast::InstDef::BitExprOp op, uint64_t lhs, std::optional<uint64_t> rhs, uint16_t bitWidth)
{
    uint64_t mask = (bitWidth >= 64) ? ~0ULL : ((1ULL << bitWidth) - 1);
    uint64_t result = 0;

    switch (op)
    {
        case DSL::Ast::InstDef::BitExprOp::Add:
            result = lhs + (rhs ? *rhs : 0);
            break;
        case DSL::Ast::InstDef::BitExprOp::Sub:
            result = lhs - (rhs ? *rhs : 0);
            break;
        case DSL::Ast::InstDef::BitExprOp::And:
            result = lhs & (rhs ? *rhs : ~0ULL);
            break;
        case DSL::Ast::InstDef::BitExprOp::Or:
            result = lhs | (rhs ? *rhs : 0);
            break;
        case DSL::Ast::InstDef::BitExprOp::Xor:
            result = lhs ^ (rhs ? *rhs : 0);
            break;
        case DSL::Ast::InstDef::BitExprOp::Shl:
            result = lhs << (rhs ? *rhs : 0);
            break;
        case DSL::Ast::InstDef::BitExprOp::Shr:
            result = lhs >> (rhs ? *rhs : 0);
            break;
        case DSL::Ast::InstDef::BitExprOp::Not:
            result = ~lhs;
            break;
    }

    return result & mask;
}

std::optional<Sema::Symbols::ResolvedBitExprValue>
resolveBitExpr(DiagnosticCollector *collector,
               const DSL::Ast::InstDef::BitExprValues &expr,
               uint16_t targetBitWidth,
               std::string_view contextName,
               const std::unordered_map<std::string_view, OperandLookupInfo> *operands,
               SourceReference *sourceRef)
{
    // 1. Literal Integer
    if (const auto *lit = std::get_if<DSL::Ast::Common::IntegerLiteral>(&expr))
    {
        uint64_t val = static_cast<uint64_t>(lit->m_node);
        if (targetBitWidth < 64 && (val >> targetBitWidth) != 0)
        {
            collector->error(PassName,
                             "{}: Constant literal {:#x} exceeds bit-slice width ({} bits)",
                             contextName,
                             lit->m_node,
                             targetBitWidth)
                    << lit->m_sourceRef;
            return std::nullopt;
        }
        return Sema::Symbols::ResolvedBitExprValue(val);
    }

    // 2. Direct Identifier
    if (const auto *ident = std::get_if<DSL::Ast::Common::Identifier>(&expr))
    {
        if (!operands)
        {
            collector->error(PassName,
                             "{}: Identifier '{}' cannot be used in a format field default expression",
                             contextName,
                             ident->m_node)
                    << ident->m_sourceRef;
            return std::nullopt;
        }

        auto it = operands->find(ident->m_node);
        if (it == operands->end())
        {
            collector->error(PassName, "{}: Unknown operand identifier '{}'", contextName, ident->m_node)
                    << ident->m_sourceRef;
            return std::nullopt;
        }

        return Sema::Symbols::ResolvedBitExprValue(Sema::Symbols::SlicedOperandRef{
                .m_operandName = ident->m_node,
                .m_operandTypeId = it->second.m_typeOrClassId,
                .m_slice = { .m_from = 0, .m_to = static_cast<uint16_t>(targetBitWidth - 1) } });
    }

    // 3. Sliced Identifier (e.g. imm12[0:4])
    if (const auto *sliced = std::get_if<DSL::Ast::InstDef::SlicedIdentifier>(&expr))
    {
        if (!operands)
        {
            collector->error(PassName,
                             "{}: Sliced identifier '{}' cannot be used in a format field default expression",
                             contextName,
                             sliced->m_name.m_node)
                    << sliced->m_name.m_sourceRef;
            return std::nullopt;
        }

        auto it = operands->find(sliced->m_name.m_node);
        if (it == operands->end())
        {
            collector->error(PassName, "{}: Unknown operand '{}'", contextName, sliced->m_name.m_node)
                    << sliced->m_name.m_sourceRef;
            return std::nullopt;
        }

        uint16_t sliceMax = std::max(sliced->m_slice.m_from, sliced->m_slice.m_to);
        if (it->second.m_bitWidth > 0 && sliceMax >= it->second.m_bitWidth)
        {
            collector->error(PassName,
                             "{}: Slice [{}:{}] exceeds operand '{}' bitwidth ({})",
                             contextName,
                             sliced->m_slice.m_from,
                             sliced->m_slice.m_to,
                             sliced->m_name.m_node,
                             it->second.m_bitWidth)
                    << sliced->m_name.m_sourceRef;
            return std::nullopt;
        }

        return Sema::Symbols::ResolvedBitExprValue(Sema::Symbols::SlicedOperandRef{
                .m_operandName = sliced->m_name.m_node,
                .m_operandTypeId = it->second.m_typeOrClassId,
                .m_slice = { .m_from = sliced->m_slice.m_from, .m_to = sliced->m_slice.m_to } });
    }

    // 4. Composite Binary/Unary BitExpression
    if (const auto *subExprPtr = std::get_if<std::shared_ptr<DSL::Ast::InstDef::BitExpression>>(&expr))
    {
        const auto &subExpr = *subExprPtr;
        auto lhsResolved = resolveBitExpr(collector, subExpr->m_lhs, targetBitWidth, contextName, operands, sourceRef);
        if (!lhsResolved)
        {
            return std::nullopt;
        }

        std::optional<Sema::Symbols::ResolvedBitExprValue> rhsResolved = std::nullopt;
        if (subExpr->m_rhs)
        {
            rhsResolved = resolveBitExpr(collector, *subExpr->m_rhs, targetBitWidth, contextName, operands, sourceRef);
            if (!rhsResolved)
            {
                return std::nullopt;
            }
        }

        // Constant Folding Optimization
        const bool lhsIsConst = std::holds_alternative<uint64_t>(*lhsResolved);
        const bool rhsIsConst = !rhsResolved || std::holds_alternative<uint64_t>(*rhsResolved);

        if (lhsIsConst && rhsIsConst)
        {
            uint64_t lVal = std::get<uint64_t>(*lhsResolved);
            std::optional<uint64_t> rVal =
                    rhsResolved ? std::make_optional(std::get<uint64_t>(*rhsResolved)) : std::nullopt;
            uint64_t folded = foldConstantOp(subExpr->m_op, lVal, rVal, targetBitWidth);
            return Sema::Symbols::ResolvedBitExprValue(folded);
        }

        auto resolvedNode = std::make_shared<Sema::Symbols::ResolvedBitExpr>(
                Sema::Symbols::ResolvedBitExpr{ .m_lhs = std::move(*lhsResolved),
                                                .m_op = subExpr->m_op,
                                                .m_rhs = std::move(rhsResolved) });

        return Sema::Symbols::ResolvedBitExprValue(std::move(resolvedNode));
    }

    return std::nullopt;
}

} // anonymous namespace

bool InstructionDefPass::run(DiagnosticCollector *collector, SymbolTable *table, DSL::Ast::InstDef::InstDefFile *file)
{
    if (!collector || !table || !file)
    {
        return false;
    }

    collector->trace(PassName,
                     "Running instruction definition semantic analysis (Formats: {}, Instructions: {})",
                     file->m_formats.size(),
                     file->m_instructions.size());

    // Phase 1: Declare and validate all instruction encoding formats
    if (!declareFormats(collector, table, file))
    {
        return false;
    }

    // Phase 2: Declare, resolve and validate target instructions
    if (!declareInstructions(collector, table, file))
    {
        return false;
    }

    return true;
}

bool InstructionDefPass::declareFormats(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        DSL::Ast::InstDef::InstDefFile *file)
{
    bool success = true;

    for (const auto &fmt : file->m_formats)

    {
        const auto &nameIdent = fmt.m_name;

        if (fmt.m_bitWidth == 0 || fmt.m_bitWidth > 128)
        {
            collector->error(PassName,
                             "Format '{}' bitwidth ({}) must be between 1 and 128 bits",
                             nameIdent.m_node,
                             fmt.m_bitWidth)
                    << nameIdent.m_sourceRef;
            success = false;
            continue;
        }

        Sema::Symbols::InstructionFormatSymbol fmtSym{ .m_name = nameIdent.m_node,
                                                       .m_bitWidth = fmt.m_bitWidth,
                                                       .m_fields = std::pmr::vector<Sema::Symbols::FormatFieldSymbol>{
                                                               table->getAllocator() } };

        SymbolId fmtSymId = table->declareSym(nameIdent.m_sourceRef,
                                              SymbolFlags::IsDefined,
                                              SymbolType::InstructionFormat,
                                              std::move(fmtSym),
                                              nameIdent.m_node);

        if (fmtSymId == InvalidSymbolId)
        {
            collector->error(PassName, "Redefinition of instruction format '{}'", nameIdent.m_node)
                    << nameIdent.m_sourceRef;
            success = false;
            continue;
        }

        if (!validateFormat(collector, table, fmt, fmtSymId))
        {
            success = false;
        }

        collector->trace(PassName, "Defined instruction format '{}'", nameIdent.m_node);
    }

    return success;
}

bool InstructionDefPass::validateFormat(DiagnosticCollector *collector,
                                        SymbolTable *table,
                                        const DSL::Ast::InstDef::InstFormatDecl &formatDecl,
                                        SymbolId formatSymId)
{
    bool valid = true;
    Symbol *formatSymbol = table->getSymById(formatSymId);
    auto *formatData = formatSymbol ? formatSymbol->getIf<Sema::Symbols::InstructionFormatSymbol>() : nullptr;

    std::unordered_set<std::string_view> seenFields;
    std::vector<std::string_view> bitOccupancy(formatDecl.m_bitWidth, "");

    for (const auto &field : formatDecl.m_fields)
    {
        const auto &fieldName = field.m_name.m_node;
        SourceReference *fieldRef = field.m_name.m_sourceRef;

        // 1. Duplicate field names
        if (!seenFields.insert(fieldName).second)
        {
            collector->error(PassName, "Format '{}': Duplicate field name '{}'", formatDecl.m_name.m_node, fieldName)

                    << fieldRef;
            valid = false;
        }

        uint16_t minBit = std::min(field.m_slice.m_from, field.m_slice.m_to);
        uint16_t maxBit = std::max(field.m_slice.m_from, field.m_slice.m_to);
        uint16_t fieldBitWidth = maxBit - minBit + 1;

        // 2. Out-of-bounds bit slices
        if (maxBit >= formatDecl.m_bitWidth)

        {
            collector->error(PassName,
                             "Format '{}', field '{}': Bit range [{}:{}] exceeds format bitwidth ({})",
                             formatDecl.m_name.m_node,
                             fieldName,
                             field.m_slice.m_from,
                             field.m_slice.m_to,
                             formatDecl.m_bitWidth)
                    << fieldRef;
            valid = false;
            continue;
        }

        // 3. Overlapping bit fields
        for (uint16_t b = minBit; b <= maxBit; ++b)
        {
            if (!bitOccupancy[b].empty())
            {
                collector->error(PassName,
                                 "Format '{}': Bit {} in field '{}' collides with field '{}'",
                                 formatDecl.m_name.m_node,
                                 b,
                                 fieldName,
                                 bitOccupancy[b])
                        << fieldRef;
                valid = false;
                break;
            }
            bitOccupancy[b] = fieldName;
        }

        // 4. Resolve and validate field default expression (if declared)
        std::optional<Sema::Symbols::ResolvedBitExprValue> resolvedDefault = std::nullopt;
        if (field.m_defaultValue)

        {
            resolvedDefault =
                    resolveBitExpr(collector, *field.m_defaultValue, fieldBitWidth, fieldName, nullptr, fieldRef);
            if (!resolvedDefault)
            {
                valid = false;
            }
        }

        // 5. Record resolved field symbol
        if (formatData)
        {
            formatData->m_fields.push_back(Sema::Symbols::FormatFieldSymbol{
                    .m_name = fieldName,
                    .m_slice = Sema::Symbols::BitSlice{ .m_from = field.m_slice.m_from, .m_to = field.m_slice.m_to },
                    .m_defaultValue = std::move(resolvedDefault) });
        }
    }

    return valid;
}

bool InstructionDefPass::declareInstructions(DiagnosticCollector *collector,
                                             SymbolTable *table,
                                             DSL::Ast::InstDef::InstDefFile *file)
{
    bool success = true;

    for (const auto &inst : file->m_instructions)

    {
        if (!validateInstruction(collector, table, inst))
        {
            success = false;
        }
    }

    return success;
}

bool InstructionDefPass::validateInstruction(DiagnosticCollector *collector,
                                             SymbolTable *table,
                                             const DSL::Ast::InstDef::InstDecl &instDecl)
{
    const auto &instName = instDecl.m_header.m_name;
    SourceReference *ref = instName.m_sourceRef;

    // 1. Symbol collision check
    if (table->getSymByName(instName.m_node) != nullptr)
    {
        collector->error(PassName, "Duplicate instruction symbol '{}'", instName.m_node) << ref;
        return false;
    }

    // 2. Validate format reference
    Symbol *formatSym = table->getSymByName(instDecl.m_header.m_formatName.m_node);
    if (!formatSym || formatSym->getType() != SymbolType::InstructionFormat)
    {
        collector->error(PassName,
                         "Instruction '{}' references unknown instruction format '{}'",
                         instName.m_node,
                         instDecl.m_header.m_formatName.m_node)
                << instDecl.m_header.m_formatName.m_sourceRef;
        return false;
    }

    const auto *formatData = formatSym->getIf<Sema::Symbols::InstructionFormatSymbol>();

    // 3. Resolve Operands
    std::pmr::vector<Sema::Symbols::TargetOperandSymbol> resolvedArgs{ table->getAllocator() };
    std::pmr::vector<Sema::Symbols::TargetOperandSymbol> resolvedImplicitArgs{ table->getAllocator() };

    if (!resolveOperands(collector, table, instDecl, resolvedArgs, resolvedImplicitArgs))
    {
        return false;
    }

    // 4. Validate bitfield assignments against format and resolve expressions
    std::pmr::vector<Sema::Symbols::FieldAssignmentSymbol> resolvedAssignments{ table->getAllocator() };
    if (formatData && !validateFieldAssignments(collector, table, instDecl, *formatData, resolvedAssignments))
    {
        return false;
    }

    // 5. Aggregate and validate flags
    uint32_t flagsMask = 0;
    if (!aggregateAndValidateFlags(collector, flagsMask, instDecl))
    {
        return false;
    }

    // 6. Register instruction symbol in SymbolTable
    Sema::Symbols::TargetInstructionSymbol data{ .m_name = instName.m_node,
                                                 .m_formatId = formatSym->getId(),
                                                 .m_fieldAssignments = std::move(resolvedAssignments),
                                                 .m_args = std::move(resolvedArgs),
                                                 .m_implicitArgs = std::move(resolvedImplicitArgs),
                                                 .m_asmTemplate = instDecl.m_body.m_asmTemplate,
                                                 .m_latency =
                                                         instDecl.m_body.m_latency == 0 ? 1 : instDecl.m_body.m_latency,
                                                 .m_flagsMask = flagsMask };

    SymbolId id =
            table->declareSym(ref, SymbolFlags::IsDefined, SymbolType::Instruction, std::move(data), instName.m_node);

    if (id == InvalidSymbolId)
    {
        collector->error(PassName, "Failed to declare instruction symbol '{}'", instName.m_node) << ref;
        return false;
    }

    collector->trace(PassName, "Defined instruction: {}", instName.m_node);
    return true;
}

bool InstructionDefPass::resolveOperands(DiagnosticCollector *collector,
                                         SymbolTable *table,
                                         const DSL::Ast::InstDef::InstDecl &instDecl,
                                         std::pmr::vector<Sema::Symbols::TargetOperandSymbol> &outArgs,
                                         std::pmr::vector<Sema::Symbols::TargetOperandSymbol> &outImplicitArgs)
{
    bool valid = true;
    std::unordered_set<std::string_view> seenOperandNames;

    auto processOp = [&](const DSL::Ast::InstDef::InstOperand &op, bool isImplicit) -> bool
    {
        const auto &name = op.m_name.m_node;
        SourceReference *opRef = op.m_name.m_sourceRef;

        if (!seenOperandNames.insert(name).second)
        {
            collector->error(PassName,
                             "Instruction '{}': Duplicate operand name '{}'",
                             instDecl.m_header.m_name.m_node,
                             name)
                    << opRef;
            return false;
        }

        SymbolId resolvedTypeId = InvalidSymbolId;

        if (op.m_kind == DSL::Ast::InstDef::InstOperandKind::Register)

        {
            Symbol *classSym = table->getSymByName(op.m_typeOrClass.m_node);
            if (!classSym || classSym->getType() != SymbolType::RegisterClass)
            {
                collector->error(PassName,
                                 "Instruction '{}', operand '{}': Unknown register class '{}'",
                                 instDecl.m_header.m_name.m_node,
                                 name,
                                 op.m_typeOrClass.m_node)
                        << op.m_typeOrClass.m_sourceRef;
                return false;
            }
            resolvedTypeId = classSym->getId();
        }
        else // Immediate
        {
            if (op.m_dir == DSL::Ast::InstDef::InstOperandDir::ArgOut ||
                op.m_dir == DSL::Ast::InstDef::InstOperandDir::ArgInOut)

            {
                collector->error(PassName,
                                 "Instruction '{}', operand '{}': Immediates cannot be OUT/INOUT",
                                 instDecl.m_header.m_name.m_node,
                                 name)
                        << opRef;
                return false;
            }

            if (op.m_typeParam.has_value())
            {
                Symbol *typeSym = table->getSymByName(op.m_typeParam->m_node);
                if (!typeSym || typeSym->getType() != SymbolType::Type)
                {
                    collector->error(PassName,
                                     "Instruction '{}', operand '{}': Unknown immediate type '{}'",
                                     instDecl.m_header.m_name.m_node,
                                     name,
                                     op.m_typeParam->m_node)
                            << op.m_typeParam->m_sourceRef;
                    return false;
                }
                resolvedTypeId = typeSym->getId();
            }
        }

        Sema::Symbols::TargetOperandSymbol resolvedOp{ .m_kind = op.m_kind,
                                                       .m_typeOrClassId = resolvedTypeId,
                                                       .m_name = name,
                                                       .m_dir = op.m_dir };

        if (isImplicit)
        {
            outImplicitArgs.push_back(resolvedOp);
        }
        else
        {
            outArgs.push_back(resolvedOp);
        }

        return true;
    };

    for (const auto &op : instDecl.m_header.m_args)

    {
        if (!processOp(op, false))
        {
            valid = false;
        }
    }

    for (const auto &op : instDecl.m_body.m_implicitArgs)
    {
        if (!processOp(op, true))
        {
            valid = false;
        }
    }

    return valid;
}

bool InstructionDefPass::validateFieldAssignments(
        DiagnosticCollector *collector,
        SymbolTable *table,
        const DSL::Ast::InstDef::InstDecl &instDecl,
        const Sema::Symbols::InstructionFormatSymbol &formatSym,
        std::pmr::vector<Sema::Symbols::FieldAssignmentSymbol> &outAssignments)
{
    bool valid = true;
    const auto &instName = instDecl.m_header.m_name.m_node;

    std::unordered_map<std::string_view, const Sema::Symbols::FormatFieldSymbol *> fieldMap;
    for (const auto &field : formatSym.m_fields)
    {
        fieldMap[field.m_name] = &field;
    }

    // Collect operand lookup info for RHS expression validation
    std::unordered_map<std::string_view, OperandLookupInfo> operandMap;
    auto collectOps = [&](const auto &operands)
    {
        for (const auto &op : operands)
        {
            uint32_t width = 0;
            SymbolId typeId = InvalidSymbolId;
            if (op.m_kind == DSL::Ast::InstDef::InstOperandKind::Register)

            {
                if (Symbol *rcSym = table->getSymByName(op.m_typeOrClass.m_node))
                {
                    typeId = rcSym->getId();
                }
            }
            else if (op.m_typeParam)
            {
                if (Symbol *tySym = table->getSymByName(op.m_typeParam->m_node))

                {
                    typeId = tySym->getId();
                    if (const auto *tyData = tySym->getIf<Sema::Symbols::TypeSymbol>())
                    {
                        width = tyData->m_bitWidth;
                    }
                }
            }
            operandMap[op.m_name.m_node] = { .m_typeOrClassId = typeId, .m_bitWidth = width };
        }
    };
    collectOps(instDecl.m_header.m_args);
    collectOps(instDecl.m_body.m_implicitArgs);

    std::unordered_map<std::string_view, uint64_t> fieldBitMasks;

    for (const auto &assign : instDecl.m_body.m_assigns)

    {
        const auto &lhsName = assign.m_lhs.m_node;
        SourceReference *lhsRef = assign.m_lhs.m_sourceRef;

        // 1. Verify that the assigned field exists in the format
        auto it = fieldMap.find(lhsName);
        if (it == fieldMap.end())
        {
            collector->error(PassName,
                             "Instruction '{}': Field '{}' is not declared in format '{}'",
                             instName,
                             lhsName,
                             formatSym.m_name)
                    << lhsRef;
            valid = false;
            continue;
        }

        uint16_t fieldBitWidth = std::max(it->second->m_slice.m_from, it->second->m_slice.m_to) -
                std::min(it->second->m_slice.m_from, it->second->m_slice.m_to) + 1;

        // 2. Validate bit slice on the LHS field if present
        Sema::Symbols::BitSlice targetSlice{ .m_from = 0, .m_to = static_cast<uint16_t>(fieldBitWidth - 1) };
        if (assign.m_lhsSlice.has_value())

        {
            uint16_t maxSliceBit = std::max(assign.m_lhsSlice->m_from, assign.m_lhsSlice->m_to);
            if (maxSliceBit >= fieldBitWidth)
            {
                collector->error(PassName,
                                 "Instruction '{}': Slice [{}:{}] exceeds width of field '{}' ({} bits)",
                                 instName,
                                 assign.m_lhsSlice->m_from,
                                 assign.m_lhsSlice->m_to,
                                 lhsName,
                                 fieldBitWidth)
                        << lhsRef;
                valid = false;
                continue;
            }
            targetSlice =
                    Sema::Symbols::BitSlice{ .m_from = assign.m_lhsSlice->m_from, .m_to = assign.m_lhsSlice->m_to };
        }

        uint16_t sliceWidth =
                std::max(targetSlice.m_from, targetSlice.m_to) - std::min(targetSlice.m_from, targetSlice.m_to) + 1;

        // 3. Check for overlapping assignments on the same field
        uint64_t sliceMask = 0;
        uint16_t minBit = std::min(targetSlice.m_from, targetSlice.m_to);
        uint16_t maxBit = std::max(targetSlice.m_from, targetSlice.m_to);
        for (uint16_t b = minBit; b <= maxBit; ++b)
        {
            sliceMask |= (1ULL << b);
        }

        if ((fieldBitMasks[lhsName] & sliceMask) != 0)
        {
            collector->error(PassName,
                             "Instruction '{}': Overlapping bit assignment for field '{}' at [{}:{}]",
                             instName,
                             lhsName,
                             targetSlice.m_from,
                             targetSlice.m_to)
                    << lhsRef;
            valid = false;
        }
        fieldBitMasks[lhsName] |= sliceMask;

        // 4. Resolve and fold RHS BitExpression
        auto resolvedRhs = resolveBitExpr(collector, assign.m_rhs, sliceWidth, instName, &operandMap, lhsRef);
        if (!resolvedRhs)
        {
            valid = false;
            continue;
        }

        outAssignments.push_back(Sema::Symbols::FieldAssignmentSymbol{ .m_fieldName = lhsName,
                                                                       .m_fieldSlice = targetSlice,
                                                                       .m_value = std::move(*resolvedRhs) });
    }

    return valid;
}

bool InstructionDefPass::aggregateAndValidateFlags(DiagnosticCollector *collector,
                                                   uint32_t &out,
                                                   const DSL::Ast::InstDef::InstDecl &instDecl)
{
    using namespace DSL::Ast::InstDef;

    out = 0;
    for (const auto &flag : instDecl.m_body.m_flags)
    {
        out |= (1u << static_cast<uint32_t>(flag));
    }

    auto hasFlag = [&](InstFlag f) { return (out & (1u << static_cast<uint32_t>(f))) != 0; };

    const bool isBranch = hasFlag(InstFlag::IsBranch);
    const bool isReturn = hasFlag(InstFlag::IsReturn);
    const bool isCall = hasFlag(InstFlag::IsCall);
    const bool isTerminator = hasFlag(InstFlag::IsTerminator);

    SourceReference *ref = instDecl.m_header.m_name.m_sourceRef;
    bool valid = true;

    if ((isBranch || isReturn) && !isTerminator)
    {
        collector->error(PassName,
                         "Instruction '{}': Flag 'IsBranch' or 'IsReturn' requires 'IsTerminator'",
                         instDecl.m_header.m_name.m_node)
                << ref;
        valid = false;
    }

    if ((isBranch && isCall) || (isBranch && isReturn) || (isCall && isReturn))
    {
        collector->error(PassName,
                         "Instruction '{}': Flags 'IsBranch', 'IsCall', and 'IsReturn' are mutually exclusive",
                         instDecl.m_header.m_name.m_node)
                << ref;
        valid = false;
    }

    return valid;
}