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

/// Stores the builder context used to emit the lowered ABI instructions.
MirAbiLowerer::MirAbiLowerer(MirBuilderContext *ctx) : m_ctx(ctx) {}

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

    switch (loc.getType())
    {
        case ArgLocationType::Register:
        {
            // Standard single-register.
            const RegLoc &reg = loc.getReg();
            if (pushRets.size() != 1)
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Calling convention expects a single register location but multiple accumulated chunks were "
                           "encountered.";
                return false;
            }

            MirOperand *srcVal = pushRets.front()->getOperands()[1];
            MirRegister *destVal = oBuilder.buildPhysReg(srcVal->getMirType(),
                                                         reg.m_ref.getId(),
                                                         "ret",
                                                         reg.m_ref.getClass(),
                                                         srcVal->getSourceRef());

            iBuilder.MOV(destVal, srcVal);
            break;
        }
        case ArgLocationType::Split:
        {
            const SplitLoc &split = loc.getSplit();
            if (pushRets.size() <= split.m_parts.size())
            {
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const auto &reg = split.m_parts[p].m_reg;
                    MirOperand *sliceVal = pushRets[p]->getOperands()[1];
                    MirRegister *destVal = oBuilder.buildPhysReg(sliceVal->getMirType(),
                                                                 reg.getId(),
                                                                 "ret",
                                                                 reg.getClass(),
                                                                 sliceVal->getSourceRef());

                    iBuilder.MOV(destVal, sliceVal);
                }
            }
            else
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Mismatched push count encountered for physical register split rules.";
                return false;
            }
            break;
        }
        case ArgLocationType::Indirect:
        {
            // Struct Return (SRET): Write out data directly into the caller-provided address pointer space.
            const IndirectLoc &indirect = loc.getIndirect();

            if (func->getParameters().empty())
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Indirect return requested, but the parameter list is empty. SRET pointer argument is "
                           "missing.";
                return false;
            }

            MirRegister *sretPtrReg = func->getParameters().front();

            if (indirect.m_copyOnReg)
            {
                auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "ReturnAbiLowerer");
                diag << sretPtrReg->getSourceRef() << "Indirect return needs CopyOnReg:";
                diag.appendNote("Target register ID: {}", indirect.m_pointerStorage.getId());

                MirRegister *phys = oBuilder.buildPhysReg(sretPtrReg->getMirType(),
                                                          indirect.m_pointerStorage.getId(),
                                                          "copyReg",
                                                          indirect.m_pointerStorage.getClass(),
                                                          sretPtrReg->getSourceRef());
                iBuilder.MOV(sretPtrReg->getSourceRef(), phys, sretPtrReg);
            }

            break;
        }
        default:
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                    << retInstr->getSourceRef() << "Unsupported target return assignment location strategy requested.";
            return false;
        }
    }

    // Clear the operands of the return (binding token).; // Clear the operands of the return.
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

        switch (argLoc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = argLoc.getReg();
                MirRegister *physReg = oBuilder.buildPhysReg(argType,
                                                             reg.m_ref.getId(),
                                                             std::format("arg{}", argIdx),
                                                             reg.m_ref.getClass(),
                                                             pushArgInstr->getSourceRef());

                // Emit: MOV physReg, argVal
                iBuilder.MOV(physReg, argVal);
                break;
            }

            case ArgLocationType::Split:
            {
                MirRegister *argReg = argVal->get<MirRegister>();
                if (!argReg)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                            << pushArgInstr->getSourceRef() << "Can't lower split variables that are not registers";
                    return false;
                }

                const SplitLoc &split = argLoc.getSplit();
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const SplitPiece &piece = split.m_parts[p];

                    // Build destination physical register for this split chunk
                    const auto &t = m_ctx->getTypeTable();
                    MirType *ptr = t->getPtr(piece.m_type);
                    MirRegister *physReg = oBuilder.buildPhysReg(ptr,
                                                                 piece.m_reg.getId(),
                                                                 std::format("splitArg{}", argIdx),
                                                                 piece.m_reg.getClass(),
                                                                 pushArgInstr->getSourceRef());
                    MirMemory *mem = oBuilder.buildMem(ptr,
                                                       argReg,
                                                       FlexInt(piece.m_offsetInParam),
                                                       pushArgInstr->getSourceRef());
                    iBuilder.LOAD(pushArgInstr->getSourceRef(), physReg, mem);
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = argLoc.getIndirect();

                if (indirect.m_isByVal)
                {
                    StackFrameObject *byValObj = func->getStackFrame()->createStaticStackObj(argType);
                    MirOperand *byValAddr = oBuilder.buildRef(byValObj, pushArgInstr->getSourceRef());

                    // Store value into the stack copy
                    iBuilder.STORE(byValAddr, argVal);

                    // Pass pointer to the stack copy in the target physical register
                    MirRegister *physReg = oBuilder.buildPhysReg(byValAddr->getMirType(),
                                                                 indirect.m_pointerStorage.getId(),
                                                                 std::format("byValArgPtr{}", argIdx),
                                                                 indirect.m_pointerStorage.getClass(),
                                                                 pushArgInstr->getSourceRef());
                    iBuilder.MOV(physReg, byValAddr);
                }
                else
                {
                    // Direct pointer pass. This case should not trigger as this is a regular Register loc, but just in
                    // case.
                    MirRegister *physReg = oBuilder.buildPhysReg(argType,
                                                                 indirect.m_pointerStorage.getId(),
                                                                 std::format("indirectArgPtr{}", argIdx),
                                                                 indirect.m_pointerStorage.getClass(),
                                                                 pushArgInstr->getSourceRef());
                    iBuilder.MOV(physReg, argVal);
                }
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = argLoc.getStack();
                MirOperand *stackParamAddr = oBuilder.buildRef(stack.m_object, pushArgInstr->getSourceRef());

                // Store value to outgoing stack argument area
                iBuilder.STORE(stackParamAddr, argVal);
                break;
            }

            default:
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                        << pushArgInstr->getSourceRef() << "Unsupported argument location strategy requested.";
                return false;
            }
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

        switch (retLoc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = retLoc.getReg();
                MirRegister *physReg = oBuilder.buildPhysReg(retType,
                                                             reg.m_ref.getId(),
                                                             std::format("call_ret{}", retIdx),
                                                             reg.m_ref.getClass(),
                                                             popRetInstr->getSourceRef());

                // Emit: MOV destVReg, physReg (Extract physical return register into virtual register)
                iBuilder.MOV(destVal, physReg);
                break;
            }

            case ArgLocationType::Split:
            {
                MirRegister *destReg = destVal->get<MirRegister>();
                if (!destReg)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                            << popRetInstr->getSourceRef()
                            << "Can't lower split return value into a non-register destination";
                    return false;
                }

                const SplitLoc &split = retLoc.getSplit();

                // Upstream scalar legalizer might have expanded POP_RET into separate chunks
                if (popRets.size() <= split.m_parts.size())
                {
                    for (size_t p = 0; p < split.m_parts.size(); ++p)
                    {
                        const SplitPiece &piece = split.m_parts[p];
                        MirType *pieceType = piece.m_type ? piece.m_type : m_ctx->getTypeTable()->i32();

                        MirRegister *physReg = oBuilder.buildPhysReg(pieceType,
                                                                     piece.m_reg.getId(),
                                                                     std::format("call_splitRet{}", retIdx),
                                                                     piece.m_reg.getClass(),
                                                                     popRetInstr->getSourceRef());

                        // Store incoming physical return chunk into struct byte offset
                        FlexInt pieceOffset(static_cast<int64_t>(piece.m_offsetInParam));
                        MirMemory *mem = oBuilder.buildMem(m_ctx->getTypeTable()->getPtr(pieceType),
                                                           destReg,
                                                           pieceOffset,
                                                           popRetInstr->getSourceRef());

                        iBuilder.STORE(mem, physReg);
                    }
                }
                else
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                            << popRetInstr->getSourceRef()
                            << "Mismatched POP_RET count encountered for physical register split return rules.";
                    return false;
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = retLoc.getIndirect();

                // Physical register containing the pointer to the indirect return storage (or return pointer register)
                MirRegister *physReg = oBuilder.buildPhysReg(m_ctx->getTypeTable()->getPtr(retType),
                                                             indirect.m_pointerStorage.getId(),
                                                             std::format("call_indirectRetPtr{}", retIdx),
                                                             indirect.m_pointerStorage.getClass(),
                                                             popRetInstr->getSourceRef());

                // Move indirect return address/data pointer into target virtual register
                iBuilder.MOV(destVal, physReg);
                break;
            }

            default:
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                        << popRetInstr->getSourceRef() << "Unsupported call return location strategy requested.";
                return false;
            }
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

        switch (argLoc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = argLoc.getReg();
                MirRegister *physReg = oBuilder.buildPhysReg(argType,
                                                             reg.m_ref.getId(),
                                                             std::format("in_arg{}", argIdx),
                                                             reg.m_ref.getClass(),
                                                             popArgInstr->getSourceRef());

                // Emit: MOV destVReg, physReg (Read physical parameter into virtual register)
                iBuilder.MOV(destVal, physReg);
                break;
            }

            case ArgLocationType::Split:
            {
                MirRegister *destReg = destVal->get<MirRegister>();
                if (!destReg)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                            << popArgInstr->getSourceRef()
                            << "Can't lower split parameter into a non-register destination";
                    return false;
                }

                const SplitLoc &split = argLoc.getSplit();
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const SplitPiece &piece = split.m_parts[p];

                    // Read incoming physical register chunk
                    MirType *pieceType = piece.m_type;
                    MirRegister *physReg = oBuilder.buildPhysReg(pieceType,
                                                                 piece.m_reg.getId(),
                                                                 std::format("in_splitArg{}", argIdx),
                                                                 piece.m_reg.getClass(),
                                                                 popArgInstr->getSourceRef());

                    // Store incoming physical chunk into memory offset of destination struct
                    FlexInt pieceOffset(static_cast<int64_t>(piece.m_offsetInParam));
                    MirMemory *mem = oBuilder.buildMem(m_ctx->getTypeTable()->getPtr(pieceType),
                                                       destReg,
                                                       pieceOffset,
                                                       popArgInstr->getSourceRef());

                    iBuilder.STORE(mem, physReg);
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = argLoc.getIndirect();

                // Physical register containing the pointer to the indirect argument
                MirRegister *physReg = oBuilder.buildPhysReg(m_ctx->getTypeTable()->getPtr(argType),
                                                             indirect.m_pointerStorage.getId(),
                                                             std::format("in_indirectPtr{}", argIdx),
                                                             indirect.m_pointerStorage.getClass(),
                                                             popArgInstr->getSourceRef());

                // Direct pointer: Copy incoming physical pointer into destination virtual register
                iBuilder.MOV(destVal, physReg);
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = argLoc.getStack();

                // Parameter sits in incoming stack frame slot
                MirOperand *stackRef = oBuilder.buildRef(stack.m_object, popArgInstr->getSourceRef());

                // Read value from stack slot into virtual register
                iBuilder.LOAD(destVal, stackRef);
                break;
            }

            default:
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                        << popArgInstr->getSourceRef() << "Unsupported function argument location strategy.";
                return false;
            }
        }
    }

    iBuilder.erase(*it);
    return true;
}