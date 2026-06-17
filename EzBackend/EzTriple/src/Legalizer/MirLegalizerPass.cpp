#include "Legalizer/MirLegalizerPass.h"

MirLegalizerPass::MirLegalizerPass(MirBuilderContext *ctx, MirLegalizer *legalizer) : m_ctx(ctx), m_legalizer(legalizer)
{
}

const char *MirLegalizerPass::getName() const { return "MirLegalizerPass"; }

MirPassResult MirLegalizerPass::run(std::pmr::list<MirBlock *> &blockList,
                                    std::pmr::list<struct MirBlock *>::iterator it,
                                    struct MirPassManager *passManager)
{
    bool modified = false, succeeded = true;
    MirBlock *currentBlock = *it;
    auto &instructions = currentBlock->getInstructions();

    auto instrIt = instructions.begin();
    for (; instrIt != instructions.end(); instrIt++)
    {
        MirInstruction *instr = *instrIt;
        LegalizeAction *action = m_legalizer->getAction(instr->getOpCode(), instr->getOperands());

        if (action == MIRLEGALIZE_NO_ACTION)
        {
            auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirLegalizerPass");
            log << "LEGAL";
            log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(), instr->getSourceRef());
        }
        else if (action == nullptr)
        {
            auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirLegalizerPass");
            log << "Could not get action for instr";
            log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(), instr->getSourceRef());
        }
        else
        {
            auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirLegalizerPass");
            log << std::format("Illegal Instruction, applying {}", action->getName()).c_str();
            log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(), instr->getSourceRef());

            LegalizeActionResult actionRes = action->run(instructions, instrIt);

            if (!actionRes.m_succeeded)
            {
                succeeded = false;
                break;
            }

            if (actionRes.m_mirChanged)
            {
                modified = true;
            }
        }
    }

    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = succeeded };
}

MirPassIterationPlace MirLegalizerPass::getIterationPlace() const { return MirPassIterationPlace::Block; }
