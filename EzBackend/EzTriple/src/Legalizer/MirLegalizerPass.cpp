#include "Legalizer/MirLegalizerPass.h"

MirLegalizerPass::MirLegalizerPass(MirBuilderContext *ctx, MirLegalizer *legalizer) : m_ctx(ctx), m_legalizer(legalizer)
{
}

const char *MirLegalizerPass::getName() const { return "MirLegalizerPass"; }

MirPassResult MirLegalizerPass::run(std::pmr::list<MirBlock *> &blockList,
                                    std::pmr::list<struct MirBlock *>::iterator it,
                                    struct MirPassManager *passManager)
{
    bool modifiedRes = false, modifiedThisIt = false, succeeded = true;
    MirBlock *currentBlock = *it;
    auto &instructions = currentBlock->getInstructions();

    do
    {
        modifiedThisIt = false;
        auto instrIt = instructions.begin();
        for (; instrIt != instructions.end(); instrIt++)
        {
            MirInstruction *instr = *instrIt;
            LegalizeAction *action = m_legalizer->getAction(instr->getOpCode(), instr->getOperands());

            if (action == MIRLEGALIZE_NO_ACTION)
            {
                auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirLegalizerPass");
                log << "LEGAL";
                log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                               instr->getSourceRef());
            }
            else if (action == nullptr)
            {
                auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirLegalizerPass");
                log << "Could not get action for instr";
                log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                               instr->getSourceRef());
            }
            else
            {
                auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirLegalizerPass");
                log << std::format("Illegal Instruction, applying {}", action->getName()).c_str();
                log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                               instr->getSourceRef());
                log.flush();

                LegalizeActionResult actionRes = action->run(instructions, instrIt);

                if (!actionRes.m_succeeded)
                {
                    succeeded = false;
                    break;
                }

                if (actionRes.m_mirChanged)
                {
                    modifiedThisIt = true;
                    if (!m_modifiedBlockSet.contains(currentBlock->getId()))
                    {
                        m_modifiedBlocks.push_back(currentBlock);
                        m_modifiedBlockSet.insert(currentBlock->getId());
                    }
                }
            }
        }

        if (modifiedThisIt)
        {
            modifiedRes = true;
        }

    } while (modifiedThisIt);

    return { .m_modifiedMir = modifiedRes, .m_executed = true, .m_succeeded = succeeded };
}

MirPassIterationPlace MirLegalizerPass::getIterationPlace() const { return MirPassIterationPlace::Block; }

void MirLegalizerPass::printResult() const
{
    auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirLegalizerPass");
    log << std::format("Printing legalization result").c_str();

    for (auto &block : m_modifiedBlocks)
    {
        std::string str = MirPrinter::printToString(block, MirPrinterDetail::Detailed);
        log.appendNote(str.c_str(), block->getSourceRef());
    }
}
