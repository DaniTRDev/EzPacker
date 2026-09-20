#include "AbiLowerer/MirAbiLowerer.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/CallLoweringState.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"
#include <string_view>

/// Stores the builder context used to emit the lowered ABI instructions.
MirAbiLowerer::MirAbiLowerer(MirBuilderContext *ctx) : m_ctx(ctx) {}

/**
 * Request describing one "assign a value to/from an ABI location" step. The four call/return
 * lowering routines build a spec and delegate the location-shaped emission to assignAbiLocation.
 */
struct MirAbiLowerer::AbiAssignSpec
{
    MirInstructionBuilder *m_iBuilder{ nullptr }; ///< Builder positioned at the anchor.
    MirOperandBuilder *m_oBuilder{ nullptr };     ///< Operand factory for physical regs/memory.
    MirFunction *m_func{ nullptr };               ///< Function owning any materialized stack copy.
    const ArgumentLocationDesc *m_loc{ nullptr }; ///< ABI location being assigned.
    MirOperand *m_value{ nullptr };               ///< The operand on the non-location side.
    SourceReference *m_sourceRef{ nullptr };      ///< Anchor source ref; null falls back to value's.
    const std::pmr::vector<MirInstruction *> *m_splitSources{ nullptr }; ///< Per-part source ops for return splits.
    AbiDirection m_direction{ AbiDirection::ValueToLocation };           ///< Transfer direction.
    AbiSplitKind m_splitKind{ AbiSplitKind::RegisterMove };              ///< Split handling mode.
    AbiIndirectKind m_indirectKind{ AbiIndirectKind::None };             ///< Indirect handling mode.
    bool m_stackSupported{ true };                                       ///< Whether a Stack location is legal here.
    bool m_indexed{ false };                                             ///< Append m_index to the generated names.
    size_t m_index{ 0 };                                                 ///< Argument/return ordinal for names.
    const char *m_diagTag{ "AbiLowerer" };                               ///< Diagnostic channel name.
    std::string_view m_registerNameBase;                                 ///< Base name for register locations.
    std::string_view m_splitNameBase;                                    ///< Base name for split locations.
    std::string_view m_indirectNameBase;                                 ///< Base name for indirect locations.
    std::string_view m_byValNameBase;                                    ///< Base name for by-value stack pointers.
};

/**
 * Emits the single "assign a value to/from an ABI location" step. The location kind and the
 * requested direction select the concrete MIR moves; the caller owns the diagnostics and any
 * location-kind-specific pre-checks (counts, missing sret parameter, ...).
 */
MirAbiLowerer::AbiAssignFailure MirAbiLowerer::assignAbiLocation(const AbiAssignSpec &spec)
{
    MirInstructionBuilder &iBuilder = *spec.m_iBuilder;
    MirOperandBuilder &oBuilder = *spec.m_oBuilder;
    const ArgumentLocationDesc &loc = *spec.m_loc;
    const bool toLocation = (spec.m_direction == AbiDirection::ValueToLocation);

    // Return-value transfers carry their own per-value source refs (null m_sourceRef); all other
    // transfers anchor diagnostics on the instruction that triggered the lowering.
    auto refFor = [&spec](MirOperand *value) -> SourceReference *
    { return spec.m_sourceRef ? spec.m_sourceRef : (value ? value->getSourceRef() : nullptr); };

    auto makeName = [&spec](std::string_view base) -> std::string
    {
        if (!spec.m_indexed)
            return std::string(base);
        return std::format("{}{}", base, spec.m_index);
    };

    switch (loc.getType())
    {
        case ArgLocationType::Register:
        {
            const RegLoc &reg = loc.getReg();
            MirRegister *physReg = oBuilder.buildPhysReg(spec.m_value->getMirType(),
                                                         reg.m_ref.getId(),
                                                         makeName(spec.m_registerNameBase),
                                                         reg.m_ref.getClass(),
                                                         refFor(spec.m_value));
            if (toLocation)
                iBuilder.MOV(physReg, spec.m_value);
            else
                iBuilder.MOV(spec.m_value, physReg);
            return AbiAssignFailure::None;
        }

        case ArgLocationType::Split:
        {
            const SplitLoc &split = loc.getSplit();

            if (spec.m_splitKind == AbiSplitKind::RegisterMove)
            {
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const auto &reg = split.m_parts[p].m_reg;
                    MirOperand *value = spec.m_value;
                    if (spec.m_splitSources && p < spec.m_splitSources->size())
                    {
                        value = (*spec.m_splitSources)[p]->getOperands()[1];
                    }
                    if (!value)
                    {
                        continue;
                    }
                    MirRegister *physReg = oBuilder.buildPhysReg(value->getMirType(),
                                                                 reg.getId(),
                                                                 makeName(spec.m_splitNameBase),
                                                                 reg.getClass(),
                                                                 refFor(value));
                    iBuilder.MOV(physReg, value);
                }
                return AbiAssignFailure::None;
            }

            MirRegister *base = spec.m_value->get<MirRegister>();
            if (!base)
            {
                return AbiAssignFailure::ValueNotRegister;
            }

            for (size_t p = 0; p < split.m_parts.size(); ++p)
            {
                const SplitPiece &piece = split.m_parts[p];
                if (spec.m_splitKind == AbiSplitKind::LoadIntoLocation)
                {
                    MirType *ptr = m_ctx->getTypeTable()->getPtr(piece.m_type);
                    MirRegister *physReg = oBuilder.buildPhysReg(ptr,
                                                                 piece.m_reg.getId(),
                                                                 makeName(spec.m_splitNameBase),
                                                                 piece.m_reg.getClass(),
                                                                 refFor(spec.m_value));
                    MirMemory *mem = oBuilder.buildMem(ptr, base, FlexInt(piece.m_offsetInParam), refFor(spec.m_value));
                    iBuilder.LOAD(refFor(spec.m_value), physReg, mem);
                }
                else // StoreFromLocation
                {
                    MirType *pieceType = piece.m_type ? piece.m_type : m_ctx->getTypeTable()->i32();
                    MirRegister *physReg = oBuilder.buildPhysReg(pieceType,
                                                                 piece.m_reg.getId(),
                                                                 makeName(spec.m_splitNameBase),
                                                                 piece.m_reg.getClass(),
                                                                 refFor(spec.m_value));
                    FlexInt pieceOffset(static_cast<int64_t>(piece.m_offsetInParam));
                    MirMemory *mem = oBuilder.buildMem(m_ctx->getTypeTable()->getPtr(pieceType),
                                                       base,
                                                       pieceOffset,
                                                       refFor(spec.m_value));
                    iBuilder.STORE(mem, physReg);
                }
            }
            return AbiAssignFailure::None;
        }

        case ArgLocationType::Indirect:
        {
            const IndirectLoc &indirect = loc.getIndirect();

            if (spec.m_indirectKind == AbiIndirectKind::None)
            {
                return AbiAssignFailure::None;
            }

            if (spec.m_indirectKind == AbiIndirectKind::SretCopy)
            {
                if (spec.m_func->getParameters().empty())
                {
                    return AbiAssignFailure::MissingSretParam;
                }

                MirRegister *sretPtrReg = spec.m_func->getParameters().front();
                auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, spec.m_diagTag);
                diag << sretPtrReg->getSourceRef() << "Indirect return needs CopyOnReg:";
                diag.appendNote("Target register ID: {}", indirect.m_pointerStorage.getId());

                MirRegister *phys = oBuilder.buildPhysReg(sretPtrReg->getMirType(),
                                                          indirect.m_pointerStorage.getId(),
                                                          makeName(spec.m_indirectNameBase),
                                                          indirect.m_pointerStorage.getClass(),
                                                          sretPtrReg->getSourceRef());
                iBuilder.MOV(sretPtrReg->getSourceRef(), phys, sretPtrReg);
                return AbiAssignFailure::None;
            }

            if (spec.m_indirectKind == AbiIndirectKind::ByValStack)
            {
                StackFrameObject *byValObj =
                        spec.m_func->getStackFrame()->createStaticStackObj(spec.m_value->getMirType());
                MirOperand *byValAddr = oBuilder.buildRef(byValObj, refFor(spec.m_value));
                iBuilder.STORE(byValAddr, spec.m_value);

                MirRegister *physReg = oBuilder.buildPhysReg(byValAddr->getMirType(),
                                                             indirect.m_pointerStorage.getId(),
                                                             makeName(spec.m_byValNameBase),
                                                             indirect.m_pointerStorage.getClass(),
                                                             refFor(spec.m_value));
                iBuilder.MOV(physReg, byValAddr);
                return AbiAssignFailure::None;
            }

            // Plain pointer move in either direction.
            MirType *ptrType =
                    toLocation ? spec.m_value->getMirType() : m_ctx->getTypeTable()->getPtr(spec.m_value->getMirType());
            MirRegister *physReg = oBuilder.buildPhysReg(ptrType,
                                                         indirect.m_pointerStorage.getId(),
                                                         makeName(spec.m_indirectNameBase),
                                                         indirect.m_pointerStorage.getClass(),
                                                         refFor(spec.m_value));
            if (toLocation)
                iBuilder.MOV(physReg, spec.m_value);
            else
                iBuilder.MOV(spec.m_value, physReg);
            return AbiAssignFailure::None;
        }

        case ArgLocationType::Stack:
        {
            if (!spec.m_stackSupported)
            {
                return AbiAssignFailure::UnsupportedLocation;
            }

            const StackLoc &stack = loc.getStack();
            MirOperand *stackAddr = oBuilder.buildRef(stack.m_object, refFor(spec.m_value));
            if (toLocation)
                iBuilder.STORE(stackAddr, spec.m_value);
            else
                iBuilder.LOAD(spec.m_value, stackAddr);
            return AbiAssignFailure::None;
        }

        default:
            return AbiAssignFailure::UnsupportedLocation;
    }
}

/**
 * Lowers an accumulated PUSH_RET group plus its RET by moving the pushed values into the
 * calling convention's return registers (or SRET pointer) and clearing the RET's token operand.
 */
bool MirAbiLowerer::processReturnBlock(CallingConvDesc *cc,
                                       MirBlock *targetBlock,
                                       MirFunction *func,
                                       MirType *retType,
                                       IntrusiveLinkedList<MirInstruction>::iterator it,
                                       std::pmr::vector<MirInstruction *> &pushRets)
{
    MirInstruction *retInstr = *it;
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);

    if (func->getReturnType()->getKind() == MirTypeKind::Void)
    {
        // Void methods do not need anything. // Clear the operands of the return (binding token).
        iBuilder.clearOperands(retInstr);
        return true;
    }

    CallLoweringState st(cc, m_ctx, func);
    ArgumentLocationDesc loc = cc->getReturnLoc(retType, &st);
    MirOperandBuilder oBuilder(m_ctx);

    // Return-specific pre-checks carry their own diagnostics.
    switch (loc.getType())
    {
        case ArgLocationType::Register:
            if (pushRets.size() != 1)
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Calling convention expects a single register location but multiple accumulated chunks were "
                           "encountered.";
                return false;
            }
            break;
        case ArgLocationType::Split:
            if (pushRets.size() > loc.getSplit().m_parts.size())
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Mismatched push count encountered for physical register split rules.";
                return false;
            }
            break;
        case ArgLocationType::Indirect:
            if (func->getParameters().empty())
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Indirect return requested, but the parameter list is empty. SRET pointer argument is "
                           "missing.";
                return false;
            }
            break;
        default:
            break;
    }

    AbiAssignSpec spec;
    spec.m_iBuilder = &iBuilder;
    spec.m_oBuilder = &oBuilder;
    spec.m_func = func;
    spec.m_loc = &loc;
    spec.m_value = pushRets.empty() ? nullptr : pushRets.front()->getOperands()[1];
    spec.m_splitSources = &pushRets;
    spec.m_direction = AbiDirection::ValueToLocation;
    spec.m_splitKind = AbiSplitKind::RegisterMove;
    spec.m_indirectKind = loc.getType() == ArgLocationType::Indirect && loc.getIndirect().m_copyOnReg
            ? AbiIndirectKind::SretCopy
            : AbiIndirectKind::None;
    spec.m_stackSupported = false;
    spec.m_diagTag = "ReturnAbiLowerer";
    spec.m_registerNameBase = "ret";
    spec.m_splitNameBase = "ret";
    spec.m_indirectNameBase = "copyReg";

    if (assignAbiLocation(spec) != AbiAssignFailure::None)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                << retInstr->getSourceRef() << "Unsupported target return assignment location strategy requested.";
        return false;
    }

    // Clear the operands of the return (binding token).
    iBuilder.clearOperands(retInstr);
    return true;
}

/**
 * Assigns each accumulated PUSH_ARG value to the location (register, split parts, by-value stack
 * copy, or indirect pointer) required by the calling convention, then clears the CALL token.
 */
bool MirAbiLowerer::processCallBlock(CallingConvDesc *cc,
                                     MirBlock *targetBlock,
                                     MirFunction *func,
                                     IntrusiveLinkedList<MirInstruction>::iterator it,
                                     std::pmr::vector<MirInstruction *> &pushArgs)
{
    MirInstruction *callInstr = *it;
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    // Track state of used physical registers & stack offsets during parameter assignment
    CallLoweringState callState(cc, m_ctx, func);

    /*
     * If the called function returns a value indirectly (e.g., large struct), the caller must allocate space on its
     * stack and pass a hidden first argument. This has already been done by the CallLegalizer, what it needs to be done
     * is to transform the actual PUSH_ARG bind, largetType %largeTypePtr into a mov arg0, largePtr.
     */

    for (size_t argIdx = 0; argIdx < pushArgs.size(); ++argIdx)
    {
        MirInstruction *pushArgInstr = pushArgs[argIdx];
        MirOperand *argVal = pushArgInstr->getOperands()[1];
        MirType *argType = argVal->getMirType();

        // Query Calling Convention for parameter placement
        ArgumentLocationDesc argLoc = cc->getArgLoc(argType, &callState);

        AbiAssignSpec spec;
        spec.m_iBuilder = &iBuilder;
        spec.m_oBuilder = &oBuilder;
        spec.m_func = func;
        spec.m_loc = &argLoc;
        spec.m_value = argVal;
        spec.m_sourceRef = pushArgInstr->getSourceRef();
        spec.m_direction = AbiDirection::ValueToLocation;
        spec.m_splitKind = AbiSplitKind::LoadIntoLocation;
        spec.m_indirectKind = (argLoc.getType() == ArgLocationType::Indirect && argLoc.getIndirect().m_isByVal)
                ? AbiIndirectKind::ByValStack
                : AbiIndirectKind::Plain;
        spec.m_indexed = true;
        spec.m_index = argIdx;
        spec.m_diagTag = "AbiLowerer";
        spec.m_registerNameBase = "arg";
        spec.m_splitNameBase = "splitArg";
        spec.m_indirectNameBase = "indirectArgPtr";
        spec.m_byValNameBase = "byValArgPtr";

        AbiAssignFailure failure = assignAbiLocation(spec);
        if (failure == AbiAssignFailure::ValueNotRegister)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                    << pushArgInstr->getSourceRef() << "Can't lower split variables that are not registers";
            return false;
        }
        if (failure == AbiAssignFailure::UnsupportedLocation)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                    << pushArgInstr->getSourceRef() << "Unsupported argument location strategy requested.";
            return false;
        }
    }

    // Clear token binding operand from the CALL instruction so it becomes a standard MIR call.
    iBuilder.clearOperand(callInstr, 0);
    return true;
}

/**
 * Extracts the call result from the calling convention's return registers immediately after the
 * CALL, storing split chunks into the destination struct or moving pointer results into vregs.
 */
bool MirAbiLowerer::processCallReturnBlock(CallingConvDesc *cc,
                                           MirBlock *targetBlock,
                                           MirFunction *func,
                                           IntrusiveLinkedList<MirInstruction>::iterator it,
                                           std::pmr::vector<MirInstruction *> &popRets)
{

    // If the called function produces no return value (or no POP_RET was bound), there is nothing to lower.
    if (popRets.empty())
    {
        return true;
    }

    // Insert return value extraction instructions AFTER the CALL instruction
    auto insertIt = std::next(it);
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, insertIt);
    MirOperandBuilder oBuilder(m_ctx);

    // Call state for querying the return location according to ABI rules
    CallLoweringState callState(cc, m_ctx, func);

    for (size_t retIdx = 0; retIdx < popRets.size(); ++retIdx)
    {
        MirInstruction *popRetInstr = popRets[retIdx];
        MirOperand *destVal = popRetInstr->getOperands()[1];
        MirType *retType = destVal->getMirType();

        // Query Calling Convention for return value location
        ArgumentLocationDesc retLoc = cc->getReturnLoc(retType, &callState);

        if (retLoc.getType() == ArgLocationType::Split)
        {
            if (popRets.size() > retLoc.getSplit().m_parts.size())
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                        << popRetInstr->getSourceRef()
                        << "Mismatched POP_RET count encountered for physical register split return rules.";
                return false;
            }
        }

        AbiAssignSpec spec;
        spec.m_iBuilder = &iBuilder;
        spec.m_oBuilder = &oBuilder;
        spec.m_func = func;
        spec.m_loc = &retLoc;
        spec.m_value = destVal;
        spec.m_sourceRef = popRetInstr->getSourceRef();
        spec.m_direction = AbiDirection::LocationToValue;
        spec.m_splitKind = AbiSplitKind::StoreFromLocation;
        spec.m_indirectKind =
                retLoc.getType() == ArgLocationType::Indirect ? AbiIndirectKind::Plain : AbiIndirectKind::None;
        spec.m_stackSupported = false;
        spec.m_indexed = true;
        spec.m_index = retIdx;
        spec.m_diagTag = "AbiLowerer";
        spec.m_registerNameBase = "call_ret";
        spec.m_splitNameBase = "call_splitRet";
        spec.m_indirectNameBase = "call_indirectRetPtr";

        AbiAssignFailure failure = assignAbiLocation(spec);
        if (failure == AbiAssignFailure::ValueNotRegister)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                    << popRetInstr->getSourceRef() << "Can't lower split return value into a non-register destination";
            return false;
        }
        if (failure != AbiAssignFailure::None)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                    << popRetInstr->getSourceRef() << "Unsupported call return location strategy requested.";
            return false;
        }
    }

    return true;
}

/**
 * Materializes incoming parameters at function entry: reads each POP_ARG value from its ABI
 * location (register, split chunks, indirect pointer, or incoming stack slot) into the vreg.
 */
bool MirAbiLowerer::processFunctionArguments(CallingConvDesc *cc,
                                             MirBlock *targetBlock,
                                             MirFunction *func,
                                             IntrusiveLinkedList<MirInstruction>::iterator it,
                                             std::pmr::vector<MirInstruction *> &popArgs)
{
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);

    // Insert parameter lowering instructions at the very top of the function's entry block
    if (popArgs.empty())
    {
        iBuilder.erase(*it);
        return true;
    }
    MirOperandBuilder oBuilder(m_ctx);

    // Track state of physical register allocations and incoming stack slot offsets
    CallLoweringState callState(cc, m_ctx, func);

    for (size_t argIdx = 0; argIdx < popArgs.size(); ++argIdx)
    {
        MirInstruction *popArgInstr = popArgs[argIdx];
        MirOperand *destVal = popArgInstr->getOperands()[1];
        MirType *argType = destVal->getMirType();

        // Query calling convention for parameter location
        ArgumentLocationDesc argLoc = cc->getArgLoc(argType, &callState);

        AbiAssignSpec spec;
        spec.m_iBuilder = &iBuilder;
        spec.m_oBuilder = &oBuilder;
        spec.m_func = func;
        spec.m_loc = &argLoc;
        spec.m_value = destVal;
        spec.m_sourceRef = popArgInstr->getSourceRef();
        spec.m_direction = AbiDirection::LocationToValue;
        spec.m_splitKind = AbiSplitKind::StoreFromLocation;
        spec.m_indirectKind =
                argLoc.getType() == ArgLocationType::Indirect ? AbiIndirectKind::Plain : AbiIndirectKind::None;
        spec.m_indexed = true;
        spec.m_index = argIdx;
        spec.m_diagTag = "AbiLowerer";
        spec.m_registerNameBase = "in_arg";
        spec.m_splitNameBase = "in_splitArg";
        spec.m_indirectNameBase = "in_indirectPtr";

        AbiAssignFailure failure = assignAbiLocation(spec);
        if (failure == AbiAssignFailure::ValueNotRegister)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                    << popArgInstr->getSourceRef() << "Can't lower split parameter into a non-register destination";
            return false;
        }
        if (failure != AbiAssignFailure::None)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                    << popArgInstr->getSourceRef() << "Unsupported function argument location strategy.";
            return false;
        }
    }

    iBuilder.erase(*it);
    return true;
}
