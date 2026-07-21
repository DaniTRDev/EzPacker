#include "AbiLowerer/FunctionAbiLowererPass.h"

FunctionAbiLowererPass::FunctionAbiLowererPass(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *FunctionAbiLowererPass::getName() const { return "FunctionAbiLowererPass"; }

MirPassIterationPlace FunctionAbiLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; };

MirPassResult FunctionAbiLowererPass::run(std::pmr::list<MirFunction *> &funcList,
                                          std::pmr::list<MirFunction *>::iterator it,
                                          MirPassManager *passManager)
{
    AbiLowerer abiLowerer(m_ctx);
    bool modifiedMir = false;
    MirFunction *func = *it;
    CallingConvDesc *cc = func->getCallingConv();

    std::pmr::map<MirId, UnloweredBlock> pendingBlocks{ m_ctx->getGlobalAllocator() };

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto instrIt = instructions.begin(); instrIt != instructions.end();)
        {
            MirInstruction *instr = *instrIt;
            MirInstructionOpCode op = instr->getOpCode();

            if (op == MirInstructionOpCode::PUSH_ARG)
            {
                MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                auto [mapIt, _] =
                        pendingBlocks.try_emplace(tokenId, UnloweredBlockType::Call, m_ctx->getGlobalAllocator());
                mapIt->second.m_pushList.push_back(instr);

                instrIt = instructions.erase(instrIt);
                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::PUSH_RET)
            {
                MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                auto [mapIt, _] =
                        pendingBlocks.try_emplace(tokenId, UnloweredBlockType::Return, m_ctx->getGlobalAllocator());
                mapIt->second.m_pushList.push_back(instr);

                instrIt = instructions.erase(instrIt);
                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::CALL)
            {
                MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                auto [mapIt, _] =
                        pendingBlocks.try_emplace(tokenId, UnloweredBlockType::Call, m_ctx->getGlobalAllocator());

                mapIt->second.m_targetBlock = block;
                mapIt->second.m_termIt = instrIt;
                mapIt->second.m_terminated = true;

                ++instrIt;
                continue;
            }
            else if (op == MirInstructionOpCode::RET)
            {
                MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                auto [mapIt, _] =
                        pendingBlocks.try_emplace(tokenId, UnloweredBlockType::Return, m_ctx->getGlobalAllocator());

                mapIt->second.m_targetBlock = block;
                mapIt->second.m_termIt = instrIt;
                mapIt->second.m_terminated = true;

                ++instrIt;
                continue;
            }

            ++instrIt;
        }
    }

    for (auto &[tokenId, block] : pendingBlocks)
    {
        SourceReference *errRef =
                block.m_pushList.empty() ? func->getSourceRef() : block.m_pushList.front()->getSourceRef();

        if (!block.m_terminated)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                    << errRef << "Block to lower (CALL/RET) did not have proper terminator";
            return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
        }

        if (block.m_type == UnloweredBlockType::Return)
        {
            if (!abiLowerer.processReturnBlock(cc,
                                               block.m_targetBlock,
                                               func,
                                               func->getReturnType(),
                                               block.m_termIt,
                                               block.m_pushList))
            {
                return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
            }

            m_loweredBlocks.push_back(std::move(block));
            modifiedMir = true;
        }
        else if (block.m_type == UnloweredBlockType::Call)
        {
            if (!abiLowerer.processCallBlock(cc,
                                             block.m_targetBlock,
                                             func,
                                             func->getReturnType(),
                                             block.m_termIt,
                                             block.m_pushList))
            {
                return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
            }

            modifiedMir = true;
            m_loweredBlocks.push_back(std::move(block));
        }
    }

    return { .m_modifiedMir = modifiedMir, .m_executed = true, .m_succeeded = true };
}

void FunctionAbiLowererPass::printResult() const
{
    auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "ReturnAbiLowerer");
    diag << "Printing FunctionAbiLowererPass result:";

    for (auto &block : m_loweredBlocks)
    {
        diag.appendNote("Lowered block in function", (*block.m_termIt)->getSourceRef());
        diag.appendNote(std::format("Block content: {}",
                                    MirPrinter::printToString(block.m_targetBlock, MirPrinterDetail::Detailed))
                                .c_str(),
                        block.m_targetBlock->getSourceRef());
    }
}