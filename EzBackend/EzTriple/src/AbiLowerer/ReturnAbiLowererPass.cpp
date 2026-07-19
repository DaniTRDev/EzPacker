#include "AbiLowerer/ReturnAbiLowererPass.h"

ReturnAbiLowerer::ReturnAbiLowerer(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *ReturnAbiLowerer::getName() const { return "ReturnAbiLowererPass"; }

MirPassIterationPlace ReturnAbiLowerer::getIterationPlace() const { return MirPassIterationPlace::Function; };

MirPassResult ReturnAbiLowerer::run(std::pmr::list<MirFunction *> &blockList,
                                    std::pmr::list<MirFunction *>::iterator it,
                                    MirPassManager *passManager)
{
    MirFunction *func = *it;
    CallingConvDesc *cc = func->getCallingConv();

    // Traverse all basic blocks inside the function to normalize exit parameters
    bool modifiedMir = false;
    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        MirId bindingToken = MIRID_INVALID;
        std::pmr::vector<MirInstruction *> returnBlock{ m_ctx->getGlobalAllocator() };

        for (auto instrIt = instructions.begin(); instrIt != instructions.end();)
        {
            MirInstruction *instr = *instrIt;

            if (instr->getOpCode() == MirInstructionOpCode::PUSH_RET)
            {
                size_t currentBindingToken = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                if (bindingToken == MIRID_INVALID)
                {
                    bindingToken = currentBindingToken;
                }
                else if (currentBindingToken != bindingToken)
                {
                    // This PUSH_RET is not from the current return block we are processing. This shouldn't happen, but
                    // stil...
                    ++instrIt;
                    continue;
                }

                returnBlock.push_back(instr);
                instrIt = instructions.erase(instrIt);

                modifiedMir = true;
                continue;
            }

            if (instr->getOpCode() == MirInstructionOpCode::RET)
            {
                if (bindingToken != MIRID_INVALID)
                {
                    if (!processReturnBlock(cc, block, func, func->getReturnType(), instrIt, returnBlock))
                    {
                        return { .m_modifiedMir = modifiedMir, .m_executed = true, .m_succeeded = false };
                    }

                    // Reset our tracking state indicators for subsequent return paths in this block
                    bindingToken = MIRID_INVALID;
                }

                // Standardize the RET instruction to be a terminal zero-operand instr.
                returnBlock.clear();
                instr->getOperands().clear();
                ++instrIt;

                continue;
            }

            // Normal instructions simply advance the scan index loop
            ++instrIt;
        }
    }

    return { .m_modifiedMir = modifiedMir, .m_executed = true, .m_succeeded = true };
}

void ReturnAbiLowerer::printResult() const {}

bool ReturnAbiLowerer::processReturnBlock(CallingConvDesc *cc,
                                          MirBlock *targetBlock,
                                          MirFunction *func,
                                          MirType *retType,
                                          std::pmr::list<MirInstruction *>::iterator it,
                                          std::pmr::vector<MirInstruction *> &retBlock)
{
    CallLoweringState st(cc->getCallerSavedGPRegs(), cc->getCallerSavedFPRegs());
    ArgumentLocationDesc loc = cc->getReturnLoc(retType, &st);
    MirInstruction *retInstr = *it;
    MirInstructionBuilder iBuilder(m_ctx, targetBlock, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(m_ctx);

    switch (loc.getType())
    {
        case ArgLocationType::Register:
        {
            // Standard non-expanded or single-register.
            if (retBlock.size() != 1)
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Calling convention expects a single register location but multiple accumulated chunks were "
                           "encountered.";
                return false;
            }

            MirOperand *srcVal = retBlock.front()->getOperands()[1];
            MirRegister *destVal =
                    oBuilder.buildPhysReg(srcVal->getMirType(), loc.getReg().m_regId, "ret", srcVal->getSourceRef());

            iBuilder.MOV(destVal, srcVal);
            break;
        }
        case ArgLocationType::Split:
        {
            const SplitLoc &split = loc.getSplit();

            // Case A: The upstream scalar expander already split this wide value into distinct, smaller sequential
            // PUSH_RET nodes.
            if (retBlock.size() == split.m_parts.size())
            {
                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    MirOperand *sliceVal = retBlock[p]->getOperands()[1];
                    MirRegister *destVal = oBuilder.buildPhysReg(sliceVal->getMirType(),
                                                                 split.m_parts[p].m_regId,
                                                                 "ret",
                                                                 sliceVal->getSourceRef());

                    iBuilder.MOV(destVal, sliceVal);
                }
            }
            // Case B: The value hasn't been expanded, but the ABI requires it split across registers.
            else if (retBlock.size() == 1)
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retBlock.front()->getSourceRef()
                        << "Calling convention dictates this value must be split across registers, but it hasn't been "
                           "unbundled during previous scalar legalization passes.";
                return false;
            }
            else
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Mismatched push count encountered for physical register split partitioning rules.";
                return false;
            }
            break;
        }
        case ArgLocationType::Indirect:
        {
            // Struct Return (SRET): Write out data directly into the caller-provided address pointer space.
            if (func->getParameters().empty())
            {
                m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                        << retInstr->getSourceRef()
                        << "Indirect return requested, but the parameter list is empty. SRET pointer argument is "
                           "missing.";
                return false;
            }

            MirRegister *sretPtrReg = func->getParameters().front();
            int64_t runningOffset = 0;

            for (auto *pushInstr : retBlock)
            {
                MirOperand *sliceVal = pushInstr->getOperands()[1];
                size_t sliceSize = sliceVal->getMirType()->getTotalSizeInBytes();

                // STORE sliceValType ptr[sretPtrReg + runningOffset], sliceVal
                iBuilder.STORE(pushInstr->getSourceRef(),
                               oBuilder.buildMem(sliceVal->getMirType(), sretPtrReg, FlexInt(runningOffset)),
                               sliceVal);

                runningOffset += static_cast<int64_t>(sliceSize);
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

    return true;
}