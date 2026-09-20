#include "AbiLowerer/MirAbiLowerer.h"
#include "AbiLowerer/MirAbiLowererPass.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"

/**
 * Extracts the binding-token register id from a token-bound ABI instruction. Returns MIRID_INVALID
 * when the instruction has no first operand or that operand is not a register.
 */
static MirId getTokenId(const MirInstruction *instr)
{
    const auto &operands = instr->getOperands();
    if (operands.empty() || !operands[0])
    {
        return MIRID_INVALID;
    }

    const MirRegister *reg = operands[0]->get<MirRegister>();
    return reg ? reg->getRegId() : MIRID_INVALID;
}

/// Stores the builder context used to inspect and rewrite instructions.
MirAbiLowererPass::MirAbiLowererPass(MirBuilderContext *ctx) : m_ctx(ctx) {}

/// Returns the diagnostic name of this pass.
const char *MirAbiLowererPass::getName() const { return "MirAbiLowererPass"; }

/// Runs once per function rather than once per module.
MirPassIterationPlace MirAbiLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; };

/**
 * Groups token-bound ABI instruction sequences by token id, removes them from the MIR, and
 * delegates each completed group to the matching MirAbiLowerer routine.
 */
MirPassResult MirAbiLowererPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it, MirPassManager *passManager)
{
    MirFunction *func = *it;
    if (!func)
    {
        m_ctx->getDiagCollector()->error(getName(), "Cannot lower ABI for an invalid function");
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    bool modifiedMir = false;
    MirAbiLowerer abiLowerer(m_ctx);
    CallingConvDesc *cc = func->getCallingConv();

    // Token id -> the still-unlowered PUSH/POP group accumulated for that call/return/args token.
    std::pmr::map<MirId, UnloweredBlock> pendingBlocks{ m_ctx->getGlobalAllocator() };

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        MirInstructionBuilder iBuilder(m_ctx, block, InsertionType::Append);

        for (auto instrIt = instructions.begin(); instrIt != instructions.end();)
        {
            MirInstruction *instr = *instrIt;
            MirInstructionOpCode op = instr->getOpCode();

            // Every handled opcode carries its binding-token register as the first operand.
            const bool needsToken = op == MirInstructionOpCode::PUSH_ARG || op == MirInstructionOpCode::PUSH_RET ||
                    op == MirInstructionOpCode::CALL || op == MirInstructionOpCode::RET ||
                    op == MirInstructionOpCode::POP_ARG || op == MirInstructionOpCode::POP_RET ||
                    op == MirInstructionOpCode::END_ARG;

            MirId tokenId = MIRID_INVALID;
            if (needsToken)
            {
                tokenId = getTokenId(instr);
                if (tokenId == MIRID_INVALID)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, getName())
                            << instr->getSourceRef() << "Malformed ABI instruction: missing binding token register";
                    return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
                }
            }

            if (op == MirInstructionOpCode::PUSH_ARG)
            {
                auto [mapIt, _] =
                        pendingBlocks.try_emplace(tokenId, UnloweredBlockType::Call, m_ctx->getGlobalAllocator());
                mapIt->second.m_pushList.push_back(instr);

                instrIt++;
                iBuilder.erase(instr);

                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::PUSH_RET)
            {
                auto [mapIt, _] =
                        pendingBlocks.try_emplace(tokenId, UnloweredBlockType::Return, m_ctx->getGlobalAllocator());
                mapIt->second.m_pushList.push_back(instr);

                instrIt++;
                iBuilder.erase(instr);

                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::CALL)
            {
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
                auto [mapIt, _] = pendingBlocks.try_emplace(tokenId,
                                                            UnloweredBlockType::FunctionArgs,
                                                            m_ctx->getGlobalAllocator());
                mapIt->second.m_popList.push_back(instr);

                instrIt++;
                iBuilder.erase(instr);

                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::POP_RET)
            {
                auto [mapIt, _] =
                        pendingBlocks.try_emplace(tokenId, UnloweredBlockType::Call, m_ctx->getGlobalAllocator());
                mapIt->second.m_popList.push_back(instr);

                instrIt++;
                iBuilder.erase(instr);

                modifiedMir = true;
                continue;
            }
            else if (op == MirInstructionOpCode::END_ARG)
            {
                auto [mapIt, _] = pendingBlocks.try_emplace(tokenId,
                                                            UnloweredBlockType::FunctionArgs,
                                                            m_ctx->getGlobalAllocator());

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

/**
 * Emits a trace dump of every block that was ABI-lowered.
 */
void MirAbiLowererPass::printResult()
{
    auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "ReturnAbiLowerer");
    diag << "Printing MirAbiLowererPass result:";

    for (auto &block : m_loweredBlocks)
    {
        if (block.m_type != UnloweredBlockType::FunctionArgs)
            diag.appendNote((*block.m_termIt)->getSourceRef(), "Lowered block in function");

        diag.appendNote(block.m_targetBlock->getSourceRef(),
                        "Block content: {}",
                        MirPrinter::printToString(block.m_targetBlock, MirPrinterDetail::Detailed));
    }
}