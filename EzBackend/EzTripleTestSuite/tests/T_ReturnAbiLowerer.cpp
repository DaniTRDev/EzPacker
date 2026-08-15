#include "AbiLowerer/AbiLowerer.h"

AbiLowerer::AbiLowerer(MirBuilderContext *ctx) : m_ctx(ctx) {}

bool AbiLowerer::processReturnBlock(CallingConvDesc *cc,
                                    MirBlock *targetBlock,
                                    MirFunction *func,
                                    MirType *retType,
                                    std::pmr::list<MirInstruction *>::iterator it,
                                    std::pmr::vector<MirInstruction *> &pushRets)
{
    MirInstruction *retInstr = *it;
    SourceReference *retSrcRef = retInstr->getSourceRef();

    if (func->getReturnType()->getKind() == MirTypeKind::Void)
    {
        retInstr->getOperands().clear();
        return true;
    }

    CallLoweringState st(cc, m_ctx, func);
    ArgumentLocationDesc loc = cc->getReturnLoc(retType, &st);
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    switch (loc.getType())
    {
        case ArgLocationType::Register:
        {
            const RegLoc &reg = loc.getReg();
            if (pushRets.size() != 1)
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retSrcRef
                        << "Calling convention expects a single register location but multiple chunks were "
                           "encountered.";
                return false;
            }

            MirOperand *srcVal = pushRets.front()->getOperands()[1];
            SourceReference *valSrcRef = srcVal->getSourceRef() ? srcVal->getSourceRef() : retSrcRef;

            MirRegister *destVal = oBuilder.buildPhysReg(srcVal->getMirType(),
                                                         reg.m_ref.getId(),
                                                         "ret",
                                                         reg.m_ref.getClass(),
                                                         valSrcRef);

            iBuilder.MOV(valSrcRef, destVal, srcVal);
            break;
        }

        case ArgLocationType::Split:
        {
            const SplitLoc &split = loc.getSplit();

            if (pushRets.size() == split.m_parts.size())
            {
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const auto &reg = split.m_parts[p].m_reg;
                    MirOperand *sliceVal = pushRets[p]->getOperands()[1];
                    SourceReference *sliceSrcRef = sliceVal->getSourceRef() ? sliceVal->getSourceRef() : retSrcRef;

                    MirRegister *destVal = oBuilder.buildPhysReg(sliceVal->getMirType(),
                                                                 reg.getId(),
                                                                 "ret",
                                                                 reg.getClass(),
                                                                 sliceSrcRef);

                    iBuilder.MOV(sliceSrcRef, destVal, sliceVal);
                }
            }
            else
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retSrcRef << "Mismatched push count encountered for physical register split return rules.";
                return false;
            }
            break;
        }

        case ArgLocationType::Indirect:
        {
            const IndirectLoc &indirect = loc.getIndirect();

            if (func->getParameters().empty())
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retSrcRef
                        << "Indirect return requested, but SRET pointer argument is missing from parameter list.";
                return false;
            }

            MirRegister *sretPtrReg = func->getParameters().front();
            SourceReference *sretSrcRef = sretPtrReg->getSourceRef() ? sretPtrReg->getSourceRef() : retSrcRef;

            if (indirect.m_copyOnReg)
            {
                MirRegister *phys = oBuilder.buildPhysReg(sretPtrReg->getMirType(),
                                                          indirect.m_pointerStorage.getId(),
                                                          "copyReg",
                                                          indirect.m_pointerStorage.getClass(),
                                                          sretSrcRef);
                iBuilder.MOV(sretSrcRef, phys, sretPtrReg);
            }
            break;
        }

        default:
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                    << retSrcRef << "Unsupported target return assignment location strategy.";
            return false;
        }
    }

    // Clean up lowered PUSH_RET pseudo-instructions
    for (MirInstruction *pushInst : pushRets)
    {
        auto pushIt = std::find(targetBlock->getInstructions().begin(), targetBlock->getInstructions().end(), pushInst);
        if (pushIt != targetBlock->getInstructions().end())
        {
            targetBlock->getInstructions().erase(pushIt);
        }
    }
    pushRets.clear();

    retInstr->getOperands().clear();
    return true;
}

bool AbiLowerer::processCallBlock(CallingConvDesc *cc,
                                  MirBlock *targetBlock,
                                  MirFunction *func,
                                  std::pmr::list<MirInstruction *>::iterator it,
                                  std::pmr::vector<MirInstruction *> &pushArgs)
{
    MirInstruction *callInstr = *it;
    SourceReference *callSrcRef = callInstr->getSourceRef();

    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    CallLoweringState callState(cc, m_ctx, func);

    for (size_t argIdx = 0; argIdx < pushArgs.size(); ++argIdx)
    {
        MirInstruction *pushArgInstr = pushArgs[argIdx];
        SourceReference *argSrcRef = pushArgInstr->getSourceRef() ? pushArgInstr->getSourceRef() : callSrcRef;
        MirOperand *argVal = pushArgInstr->getOperands()[1];
        MirType *argType = argVal->getMirType();

        ArgumentLocationDesc argLoc = cc->getArgLoc(argType, &callState);

        switch (argLoc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = argLoc.getReg();
                MirRegister *physReg = oBuilder.buildPhysReg(argType,
                                                             reg.m_ref.getId(),
                                                             std::format("arg{}", argIdx).c_str(),
                                                             reg.m_ref.getClass(),
                                                             argSrcRef);

                iBuilder.MOV(argSrcRef, physReg, argVal);
                break;
            }

            case ArgLocationType::Split:
            {
                MirRegister *argReg = argVal->get<MirRegister>();
                if (!argReg)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                            << argSrcRef << "Can't lower split variables that are not registers.";
                    return false;
                }

                const SplitLoc &split = argLoc.getSplit();
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const SplitPiece &piece = split.m_parts[p];
                    MirType *pieceType = piece.m_type ? piece.m_type : m_ctx->getTypeTable()->i64();
                    MirType *ptrType = m_ctx->getTypeTable()->getPtr(pieceType);

                    MirRegister *physReg = oBuilder.buildPhysReg(pieceType,
                                                                 piece.m_reg.getId(),
                                                                 std::format("splitArg{}", argIdx).c_str(),
                                                                 piece.m_reg.getClass(),
                                                                 argSrcRef);

                    MirMemory *mem = oBuilder.buildMem(ptrType,
                                                       argReg,
                                                       FlexInt(static_cast<int64_t>(piece.m_offsetInParam)),
                                                       argSrcRef);

                    iBuilder.LOAD(argSrcRef, physReg, mem);
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = argLoc.getIndirect();

                if (indirect.m_isByVal)
                {
                    StackFrameObject *byValObj = func->getStackFrame()->createStaticStackObj(argType);
                    MirOperand *byValAddr = oBuilder.buildRef(byValObj, argSrcRef);

                    iBuilder.STORE(argSrcRef, byValAddr, argVal);

                    MirRegister *physReg = oBuilder.buildPhysReg(byValAddr->getMirType(),
                                                                 indirect.m_pointerStorage.getId(),
                                                                 std::format("byValArgPtr{}", argIdx).c_str(),
                                                                 indirect.m_pointerStorage.getClass(),
                                                                 argSrcRef);

                    iBuilder.MOV(argSrcRef, physReg, byValAddr);
                }
                else
                {
                    MirRegister *physReg = oBuilder.buildPhysReg(argType,
                                                                 indirect.m_pointerStorage.getId(),
                                                                 std::format("indirectArgPtr{}", argIdx).c_str(),
                                                                 indirect.m_pointerStorage.getClass(),
                                                                 argSrcRef);

                    iBuilder.MOV(argSrcRef, physReg, argVal);
                }
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = argLoc.getStack();
                MirOperand *stackParamAddr = oBuilder.buildRef(stack.m_object, argSrcRef);

                iBuilder.STORE(argSrcRef, stackParamAddr, argVal);
                break;
            }

            default:
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                        << argSrcRef << "Unsupported argument location strategy requested.";
                return false;
            }
        }
    }

    // Clean up lowered PUSH_ARG pseudo-instructions
    for (MirInstruction *pushInst : pushArgs)
    {
        auto pushIt = std::find(targetBlock->getInstructions().begin(), targetBlock->getInstructions().end(), pushInst);
        if (pushIt != targetBlock->getInstructions().end())
        {
            targetBlock->getInstructions().erase(pushIt);
        }
    }
    pushArgs.clear();

    if (!callInstr->getOperands().empty())
    {
        callInstr->getOperands().erase(callInstr->getOperands().begin());
    }

    return true;
}

bool AbiLowerer::processCallReturnBlock(CallingConvDesc *cc,
                                        MirBlock *targetBlock,
                                        MirFunction *func,
                                        std::pmr::list<MirInstruction *>::iterator it,
                                        std::pmr::vector<MirInstruction *> &popRets)
{
    if (popRets.empty())
    {
        return true;
    }

    auto insertIt = std::next(it);
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, insertIt);
    MirOperandBuilder oBuilder(m_ctx);

    CallLoweringState callState(cc, m_ctx, func);

    for (size_t retIdx = 0; retIdx < popRets.size(); ++retIdx)
    {
        MirInstruction *popRetInstr = popRets[retIdx];
        SourceReference *popSrcRef = popRetInstr->getSourceRef();
        MirOperand *destVal = popRetInstr->getOperands()[1];
        MirType *retType = destVal->getMirType();

        ArgumentLocationDesc retLoc = cc->getReturnLoc(retType, &callState);

        switch (retLoc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = retLoc.getReg();
                MirRegister *physReg = oBuilder.buildPhysReg(retType,
                                                             reg.m_ref.getId(),
                                                             std::format("call_ret{}", retIdx).c_str(),
                                                             reg.m_ref.getClass(),
                                                             popSrcRef);

                iBuilder.MOV(popSrcRef, destVal, physReg);
                break;
            }

            case ArgLocationType::Split:
            {
                MirRegister *destReg = destVal->get<MirRegister>();
                if (!destReg)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                            << popSrcRef << "Can't lower split return value into a non-register destination.";
                    return false;
                }

                const SplitLoc &split = retLoc.getSplit();

                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const SplitPiece &piece = split.m_parts[p];
                    MirType *pieceType = piece.m_type ? piece.m_type : m_ctx->getTypeTable()->i64();

                    MirRegister *physReg = oBuilder.buildPhysReg(pieceType,
                                                                 piece.m_reg.getId(),
                                                                 std::format("call_splitRet{}", retIdx).c_str(),
                                                                 piece.m_reg.getClass(),
                                                                 popSrcRef);

                    FlexInt pieceOffset(static_cast<int64_t>(piece.m_offsetInParam));
                    MirMemory *mem = oBuilder.buildMem(m_ctx->getTypeTable()->getPtr(pieceType),
                                                       destReg,
                                                       pieceOffset,
                                                       popSrcRef);

                    iBuilder.STORE(popSrcRef, mem, physReg);
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = retLoc.getIndirect();

                MirRegister *physReg = oBuilder.buildPhysReg(m_ctx->getTypeTable()->getPtr(retType),
                                                             indirect.m_pointerStorage.getId(),
                                                             std::format("call_indirectRetPtr{}", retIdx).c_str(),
                                                             indirect.m_pointerStorage.getClass(),
                                                             popSrcRef);

                iBuilder.MOV(popSrcRef, destVal, physReg);
                break;
            }

            default:
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                        << popSrcRef << "Unsupported call return location strategy requested.";
                return false;
            }
        }
    }

    // Clean up lowered POP_RET pseudo-instructions
    for (MirInstruction *popInst : popRets)
    {
        auto popIt = std::find(targetBlock->getInstructions().begin(), targetBlock->getInstructions().end(), popInst);
        if (popIt != targetBlock->getInstructions().end())
        {
            targetBlock->getInstructions().erase(popIt);
        }
    }
    popRets.clear();

    return true;
}

bool AbiLowerer::processFunctionArguments(CallingConvDesc *cc,
                                          MirBlock *targetBlock,
                                          MirFunction *func,
                                          std::pmr::list<MirInstruction *>::iterator it,
                                          std::pmr::vector<MirInstruction *> &popArgs)
{
    if (popArgs.empty())
    {
        return true;
    }

    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    CallLoweringState callState(cc, m_ctx, func);

    for (size_t argIdx = 0; argIdx < popArgs.size(); ++argIdx)
    {
        MirInstruction *popArgInstr = popArgs[argIdx];
        SourceReference *argSrcRef = popArgInstr->getSourceRef();
        MirOperand *destVal = popArgInstr->getOperands()[1];
        MirType *argType = destVal->getMirType();

        ArgumentLocationDesc argLoc = cc->getArgLoc(argType, &callState);

        switch (argLoc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = argLoc.getReg();
                MirRegister *physReg = oBuilder.buildPhysReg(argType,
                                                             reg.m_ref.getId(),
                                                             std::format("in_arg{}", argIdx).c_str(),
                                                             reg.m_ref.getClass(),
                                                             argSrcRef);

                iBuilder.MOV(argSrcRef, destVal, physReg);
                break;
            }

            case ArgLocationType::Split:
            {
                MirRegister *destReg = destVal->get<MirRegister>();
                if (!destReg)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                            << argSrcRef << "Can't lower split parameter into a non-register destination.";
                    return false;
                }

                const SplitLoc &split = argLoc.getSplit();
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const SplitPiece &piece = split.m_parts[p];
                    MirType *pieceType = piece.m_type ? piece.m_type : m_ctx->getTypeTable()->i64();

                    MirRegister *physReg = oBuilder.buildPhysReg(pieceType,
                                                                 piece.m_reg.getId(),
                                                                 std::format("in_splitArg{}", argIdx).c_str(),
                                                                 piece.m_reg.getClass(),
                                                                 argSrcRef);

                    FlexInt pieceOffset(static_cast<int64_t>(piece.m_offsetInParam));
                    MirMemory *mem = oBuilder.buildMem(m_ctx->getTypeTable()->getPtr(pieceType),
                                                       destReg,
                                                       pieceOffset,
                                                       argSrcRef);

                    iBuilder.STORE(argSrcRef, mem, physReg);
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = argLoc.getIndirect();

                MirRegister *physReg = oBuilder.buildPhysReg(m_ctx->getTypeTable()->getPtr(argType),
                                                             indirect.m_pointerStorage.getId(),
                                                             std::format("in_indirectPtr{}", argIdx).c_str(),
                                                             indirect.m_pointerStorage.getClass(),
                                                             argSrcRef);

                iBuilder.MOV(argSrcRef, destVal, physReg);
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = argLoc.getStack();
                MirOperand *stackRef = oBuilder.buildRef(stack.m_object, argSrcRef);

                iBuilder.LOAD(argSrcRef, destVal, stackRef);
                break;
            }

            default:
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "AbiLowerer")
                        << argSrcRef << "Unsupported function argument location strategy.";
                return false;
            }
        }
    }

    // Clean up lowered POP_ARG pseudo-instructions
    for (MirInstruction *popInst : popArgs)
    {
        auto popIt = std::find(targetBlock->getInstructions().begin(), targetBlock->getInstructions().end(), popInst);
        if (popIt != targetBlock->getInstructions().end())
        {
            targetBlock->getInstructions().erase(popIt);
        }
    }
    popArgs.clear();

    // Erase the END_ARG instruction
    targetBlock->getInstructions().erase(it);
    return true;
}