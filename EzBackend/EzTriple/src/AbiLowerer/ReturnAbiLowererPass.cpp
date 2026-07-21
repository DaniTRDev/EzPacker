#include "AbiLowerer/ReturnAbiLowererPass.h"

ReturnAbiLowerer::ReturnAbiLowerer(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *ReturnAbiLowerer::getName() const { return "ReturnAbiLowererPass"; }

MirPassIterationPlace ReturnAbiLowerer::getIterationPlace() const { return MirPassIterationPlace::Function; };

MirPassResult ReturnAbiLowerer::run(std::pmr::list<MirFunction *> &funcList,
                                    std::pmr::list<MirFunction *>::iterator it,
                                    MirPassManager *passManager)
{
    bool modifiedMir = false;
    MirFunction *func = *it;
    CallingConvDesc *cc = func->getCallingConv();

    // Use PMR map keyed by the binding token register ID (MirId)
    m_unloweredReturns = std::pmr::map<MirId, UnloweredReturnBlock>{ m_ctx->getGlobalAllocator() };

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto instrIt = instructions.begin(); instrIt != instructions.end();)
        {
            MirInstruction *instr = *instrIt;
            if (instr->getOpCode() == MirInstructionOpCode::PUSH_RET)
            {
                MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                auto [mapIt, inserted] = m_unloweredReturns.try_emplace(tokenId, m_ctx->getGlobalAllocator());
                mapIt->second.m_pushRetInstrs.push_back(instr);

                // Erase PUSH_RET from block and safely advance iterator
                instrIt = instructions.erase(instrIt);
                modifiedMir = true;

                continue;
            }
            else if (instr->getOpCode() == MirInstructionOpCode::RET)
            {
                if (!instr->getOperands().empty())
                {
                    MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();

                    auto [mapIt, inserted] = m_unloweredReturns.try_emplace(tokenId, m_ctx->getGlobalAllocator());

                    mapIt->second.m_targetBlock = block;
                    mapIt->second.m_retIt = instrIt;
                    mapIt->second.m_hasRet = true;
                }

                ++instrIt;
                continue;
            }

            ++instrIt;
        }
    }

    for (auto &[tokenId, retBlock] : m_unloweredReturns)
    {
        if (!retBlock.m_hasRet)
        {
            SourceReference *errRef = retBlock.m_pushRetInstrs.empty()
                    ? func->getSourceRef()
                    : retBlock.m_pushRetInstrs.front()->getSourceRef();

            m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                    << errRef << "Orphaned PUSH_RET block encountered without a matching terminating RET";

            return { .m_modifiedMir = modifiedMir, .m_executed = true, .m_succeeded = false };
        }

        if (!processReturnBlock(cc,
                                retBlock.m_targetBlock,
                                func,
                                func->getReturnType(),
                                retBlock.m_retIt,
                                retBlock.m_pushRetInstrs))
        {
            return { .m_modifiedMir = modifiedMir, .m_executed = true, .m_succeeded = false };
        }

        // Standardize the RET instruction to be a terminal zero-operand instruction
        MirInstruction *retInstr = *retBlock.m_retIt;
        retInstr->getOperands().clear();
        modifiedMir = true;
    }

    return { .m_modifiedMir = modifiedMir, .m_executed = true, .m_succeeded = true };
}

void ReturnAbiLowerer::printResult() const
{
    auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "ReturnAbiLowerer");
    diag << "Printing ReturnAbiLowererPass result:";

    for (auto &[tokenId, retBlock] : m_unloweredReturns)
    {
        diag.appendNote("Lowered return in block", (*retBlock.m_retIt)->getSourceRef());
        diag.appendNote(std::format("Block content: {}",
                                    MirPrinter::printToString(retBlock.m_targetBlock, MirPrinterDetail::Detailed))
                                .c_str(),
                        retBlock.m_targetBlock->getSourceRef());
    }
}

bool ReturnAbiLowerer::processReturnBlock(CallingConvDesc *cc,
                                          MirBlock *targetBlock,
                                          MirFunction *func,
                                          MirType *retType,
                                          std::pmr::list<MirInstruction *>::iterator it,
                                          std::pmr::vector<MirInstruction *> &retBlock)
{
    if (func->getReturnType()->getKind() == MirTypeKind::Void)
    {
        // Void methods do not need anything.
        return true;
    }

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
            const RegLoc &reg = loc.getReg();
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
                    oBuilder.buildPhysReg(srcVal->getMirType(), reg.m_regId, "ret", srcVal->getSourceRef());

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
                diag.appendNote(std::format("Target register ID: {}", indirect.m_pointerStorage).c_str(), nullptr);

                MirRegister *phys = oBuilder.buildPhysReg(sretPtrReg->getMirType(),
                                                          indirect.m_pointerStorage,
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

    return true;
}