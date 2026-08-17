#include "AbiLowerer/MirAbiLowerer.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"

MirAbiLowererPass::MirAbiLowererPass(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *MirAbiLowererPass::getName() const { return "MirAbiLowererPass"; }

MirPassIterationPlace MirAbiLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; };

MirPassResult MirAbiLowererPass::run(std::pmr::list<MirFunction *> &funcList,
                                     std::pmr::list<MirFunction *>::iterator it,
                                     MirPassManager *passManager)
{
    MirAbiLowerer abiLowerer(m_ctx);
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
            else if (op == MirInstructionOpCode::POP_ARG)
            {
                MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                auto [mapIt, _] = pendingBlocks.try_emplace(tokenId,
                                                            UnloweredBlockType::FunctionArgs,
                                                            m_ctx->getGlobalAllocator());
                mapIt->second.m_popList.push_back(instr);

                instrIt = instructions.erase(instrIt);
                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::POP_RET)
            {
                MirId tokenId = instr->getOperands()[0]->get<MirRegister>()->getRegId();
                auto [mapIt, _] = pendingBlocks.try_emplace(tokenId,
                                                            UnloweredBlockType::FunctionArgs,
                                                            m_ctx->getGlobalAllocator());
                mapIt->second.m_popList.push_back(instr);

                instrIt = instructions.erase(instrIt);
                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::END_ARG)
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

    // If a token matches the function's ID, it represents the arguments of the function. They need to be lowered like
    // parameters but the other way around.

    for (auto &[tokenId, block] : pendingBlocks)
    {
        SourceReference *errRef =
                block.m_pushList.empty() ? func->getSourceRef() : block.m_pushList.front()->getSourceRef();

        if (!block.m_terminated)
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "ReturnAbiLowerer")
                    << errRef << "Block to lower (CALL/RET/ARGS) did not have proper terminator";
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
            if (!abiLowerer.processCallBlock(cc, block.m_targetBlock, func, block.m_termIt, block.m_pushList))
            {
                return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
            }

            if (!abiLowerer.processCallReturnBlock(cc, block.m_targetBlock, func, block.m_termIt, block.m_popList))
            {
                return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
            }

            modifiedMir = true;
            m_loweredBlocks.push_back(std::move(block));
        }
        else if (block.m_type == UnloweredBlockType::FunctionArgs)
        {
            if (!abiLowerer.processFunctionArguments(cc, block.m_targetBlock, func, block.m_termIt, block.m_popList))
            {
                return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
            }

            modifiedMir = true;
            m_loweredBlocks.push_back(std::move(block));
        }
    }

    return { .m_modifiedMir = modifiedMir, .m_executed = true, .m_succeeded = true };
}

void MirAbiLowererPass::printResult()
{
    auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "ReturnAbiLowerer");
    diag << "Printing MirAbiLowererPass result:";

    for (auto &block : m_loweredBlocks)
    {
        if (block.m_type != UnloweredBlockType::FunctionArgs)
            diag.appendNote("Lowered block in function", (*block.m_termIt)->getSourceRef());

        diag.appendNote(std::format("Block content: {}",
                                    MirPrinter::printToString(block.m_targetBlock, MirPrinterDetail::Detailed))
                                .c_str(),
                        block.m_targetBlock->getSourceRef());
    }
}