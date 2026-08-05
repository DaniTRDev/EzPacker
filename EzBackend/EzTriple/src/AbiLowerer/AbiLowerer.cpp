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
    if (func->getReturnType()->getKind() == MirTypeKind::Void)
    {
        // Void methods do not need anything.
        retInstr->getOperands().clear(); // Clear the operands of the return (binding token).

        return true;
    }

    CallLoweringState st(cc, m_ctx);
    ArgumentLocationDesc loc = cc->getReturnLoc(retType, &st);
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    switch (loc.getType())
    {
        case ArgLocationType::Register:
        {
            // Standard non-expanded or single-register.
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
            MirRegister *destVal =
                    oBuilder.buildPhysReg(srcVal->getMirType(), reg.m_ref.getId(), "ret", srcVal->getSourceRef());

            iBuilder.MOV(destVal, srcVal);
            break;
        }
        case ArgLocationType::Split:
        {
            const SplitLoc &split = loc.getSplit();

            // Case A: The upstream scalar expander already split this wide value into distinct, smaller sequential
            // PUSH_RET nodes.
            if (pushRets.size() <= split.m_parts.size())
            {
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    MirOperand *sliceVal = pushRets[p]->getOperands()[1];
                    MirRegister *destVal = oBuilder.buildPhysReg(sliceVal->getMirType(),
                                                                 split.m_parts[p].m_reg.getId(),
                                                                 "ret",
                                                                 sliceVal->getSourceRef());

                    iBuilder.MOV(destVal, sliceVal);
                }
            }
            // Case B: The value hasn't been expanded, but the ABI requires it split across registers.
            else if (pushRets.size() == 1)
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << pushRets.front()->getSourceRef()
                        << "Calling convention dictates this value must be split across registers, but it hasn't been "
                           "expanded during previous scalar legalization passes.";
                return false;
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
                diag.appendNote(std::format("Target register ID: {}", indirect.m_pointerStorage.getId()).c_str(),
                                nullptr);

                MirRegister *phys = oBuilder.buildPhysReg(sretPtrReg->getMirType(),
                                                          indirect.m_pointerStorage.getId(),
                                                          "copyReg",
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

    retInstr->getOperands().clear(); // Clear the operands of the return.
    return true;
}

bool AbiLowerer::processCallBlock(CallingConvDesc *cc,
                                  MirBlock *targetBlock,
                                  MirFunction *func,
                                  std::pmr::list<MirInstruction *>::iterator it,
                                  std::pmr::vector<MirInstruction *> &pushArgs)
{
    MirInstruction *callInstr = *it;
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    // Track state of used physical registers & stack offsets during parameter assignment
    CallLoweringState callState(cc, m_ctx);

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
                                                             std::format("arg{}", argIdx).c_str(),
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
                                                                 std::format("splitArg{}", argIdx).c_str(),
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
                    StackFrameObject *byValObj = func->getStackFrame()->createLocalStackObj(argType);
                    MirOperand *byValAddr = oBuilder.buildRef(byValObj, pushArgInstr->getSourceRef());

                    // Store value into the stack copy
                    iBuilder.STORE(byValAddr, argVal);

                    // Pass pointer to the stack copy in the target physical register
                    MirRegister *physReg = oBuilder.buildPhysReg(byValAddr->getMirType(),
                                                                 indirect.m_pointerStorage.getId(),
                                                                 std::format("byValArgPtr{}", argIdx).c_str(),
                                                                 pushArgInstr->getSourceRef());
                    iBuilder.MOV(physReg, byValAddr);
                }
                else
                {
                    // Direct pointer pass. This case should not trigger as this is a regular Register loc, but just in
                    // case.
                    MirRegister *physReg = oBuilder.buildPhysReg(argType,
                                                                 indirect.m_pointerStorage.getId(),
                                                                 std::format("indirectArgPtr{}", argIdx).c_str(),
                                                                 pushArgInstr->getSourceRef());
                    iBuilder.MOV(physReg, argVal);
                }
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = argLoc.getStack();

                // Allocate slot in outgoing parameter stack frame
                StackFrameObject *stackParamObj = func->getStackFrame()->createStackParam(argType, stack.m_frameOffset);
                MirOperand *stackParamAddr = oBuilder.buildRef(stackParamObj, pushArgInstr->getSourceRef());

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

    // Clear token binding operands from the CALL instruction so it becomes a standard MIR call.
    callInstr->getOperands().erase(callInstr->getOperands().begin());

    return true;
}

bool AbiLowerer::processCallReturnBlock(CallingConvDesc *cc,
                                        MirBlock *targetBlock,
                                        MirFunction *func,
                                        std::pmr::list<MirInstruction *>::iterator it,
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
    CallLoweringState callState(cc, m_ctx);

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
                                                             std::format("call_ret{}", retIdx).c_str(),
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
                                                                     std::format("call_splitRet{}", retIdx).c_str(),
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
                                                             std::format("call_indirectRetPtr{}", retIdx).c_str(),
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

bool AbiLowerer::processFunctionArguments(CallingConvDesc *cc,
                                          MirBlock *targetBlock,
                                          MirFunction *func,
                                          std::pmr::list<MirInstruction *>::iterator it,
                                          std::pmr::vector<MirInstruction *> &popArgs)
{
    // Insert parameter lowering instructions at the very top of the function's entry block
    if (popArgs.empty())
    {
        return true;
    }

    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    // Track state of physical register allocations and incoming stack slot offsets
    CallLoweringState callState(cc, m_ctx);

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
                                                             std::format("in_arg{}", argIdx).c_str(),
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
                                                                 std::format("in_splitArg{}", argIdx).c_str(),
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
                                                             std::format("in_indirectPtr{}", argIdx).c_str(),
                                                             popArgInstr->getSourceRef());

                // Direct pointer: Copy incoming physical pointer into destination virtual register
                iBuilder.MOV(destVal, physReg);
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = argLoc.getStack();

                // Parameter sits in incoming stack frame slot
                StackFrameObject *incomingStackObj =
                        func->getStackFrame()->createStackParam(argType, stack.m_frameOffset);
                MirOperand *stackRef = oBuilder.buildRef(incomingStackObj, popArgInstr->getSourceRef());

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

    targetBlock->getInstructions().erase(it);
    return true;
}