#include "Legalizer/MirBlockLegalizerPass.h"

MirBlockLegalizerPass::MirBlockLegalizerPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_legalizer(targetDesc->getLegalizer()), m_modifiedBlockSet(ctx->getGlobalAllocator()),
    m_modifiedBlocks(ctx->getGlobalAllocator()),
    m_legalizeCtx(LegalizeCtx{
            .m_ctx = ctx,
            .m_targetDesc = targetDesc,
            .m_instrList = {},
            .m_it = {},
            .m_promotionMap = std::pmr::map<size_t, MirRegister *>(ctx->getGlobalAllocator()),
            .m_expandMap = std::pmr::map<size_t, std::pair<MirRegister *, MirRegister *>>(ctx->getGlobalAllocator()) })
{
}

const char *MirBlockLegalizerPass::getName() const { return "MirBlockLegalizerPass"; }

MirPassResult MirBlockLegalizerPass::run(std::pmr::list<MirBlock *> &blockList,
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
        m_legalizeCtx.m_instrList = currentBlock->getInstructionsPtr();

        for (; instrIt != instructions.end(); instrIt++)
        {
            m_legalizeCtx.m_it = instrIt;
            MirInstruction *instr = *instrIt;
            LegalizationResult res = m_legalizer->legalize(m_legalizeCtx);

            switch (res)
            {
                case LegalizationResult::AlreadyLegal:
                {
                    continue;
                }
                case LegalizationResult::NoRule:
                {
                    auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirBlockLegalizerPass");
                    log << "No legalization rule defined for instruction";
                    log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                                   instr->getSourceRef());

                    return { .m_modifiedMir = modifiedRes, .m_executed = true, .m_succeeded = false };
                };
                case LegalizationResult::Legalized:
                {
                    modifiedThisIt = true;
                    if (!m_modifiedBlockSet.contains(currentBlock->getId()))
                    {
                        m_modifiedBlocks.push_back(currentBlock);
                        m_modifiedBlockSet.insert(currentBlock->getId());
                    }
                    break;
                }
                case LegalizationResult::LegalizationError:
                {
                    auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirBlockLegalizerPass");
                    log << "Error during instruction legalization";
                    log.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                                   instr->getSourceRef());

                    return { .m_modifiedMir = modifiedRes, .m_executed = true, .m_succeeded = false };
                };
            }
        }

        if (modifiedThisIt)
        {
            modifiedRes = true;
        }

    } while (modifiedThisIt);

    return { .m_modifiedMir = modifiedRes, .m_executed = true, .m_succeeded = succeeded };
}

MirPassIterationPlace MirBlockLegalizerPass::getIterationPlace() const { return MirPassIterationPlace::Block; }

void MirBlockLegalizerPass::printResult() const
{
    auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirBlockLegalizerPass");
    log << std::format("Printing block legalization result").c_str();

    for (auto &block : m_modifiedBlocks)
    {
        std::string str = MirPrinter::printToString(block, MirPrinterDetail::Detailed);
        log.appendNote(str.c_str(), block->getSourceRef());
    }
}
