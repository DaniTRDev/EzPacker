#include "InstructionSelector/MirInstructionSelectorPass.h"

MirInstructionSelectorPass::MirInstructionSelectorPass(MirBuilderContext *ctx, MirInstructionSelector *selector) :
    m_ctx(ctx), m_selector(selector)
{
}

const char *MirInstructionSelectorPass::getName() const { return "MirInstructionSelectorPass"; }

MirPassIterationPlace MirInstructionSelectorPass::getIterationPlace() const { return MirPassIterationPlace::Block; }

MirPassResult MirInstructionSelectorPass::run(std::pmr::list<MirBlock *> &blockList,
                                              std::pmr::list<MirBlock *>::iterator it,
                                              MirPassManager *passManager)
{
    bool modifiedRes = false, modifiedThisIt = false;
    MirBlock *currentBlock = *it;
    auto &instructions = currentBlock->getInstructions();

    do
    {
        modifiedThisIt = false;
        SelectionContext selectCtx{ .m_ctx = m_ctx, .m_instrList = instructions, .m_it = instructions.begin() };

        for (; selectCtx.m_it != instructions.end(); selectCtx.m_it++)
        {
            MirInstruction *instr = *selectCtx.m_it;
            SelectionResult selectionResult = m_selector->select(selectCtx);
            switch (selectionResult)
            {
                case SelectionResult::NoRule:
                {
                    auto diag = m_ctx->getDiagCollector()->builder(Diag_Error, "MirInstructionSelectorPass");
                    diag << "No rule specified for instruction selection" << instr->getSourceRef();
                    diag.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                                    instr->getSourceRef());

                    return { .m_modifiedMir = modifiedRes, .m_executed = true, .m_succeeded = false };
                }

                case SelectionResult::AlreadySelected:
                    continue;

                case SelectionResult::SelectionError:
                {
                    auto diag = m_ctx->getDiagCollector()->builder(Diag_Error, "MirInstructionSelectorPass");
                    diag << "Error during instruction selection" << instr->getSourceRef();
                    diag.appendNote(MirPrinter::printToString(instr, MirPrinterDetail::Detailed).c_str(),
                                    instr->getSourceRef());

                    return { .m_modifiedMir = modifiedRes, .m_executed = true, .m_succeeded = false };
                }

                case SelectionResult::Selected:
                {
                    modifiedThisIt = true;
                    continue;
                }
            }
        }

        if (modifiedThisIt)
        {
            modifiedRes = true;
        }

    } while (modifiedThisIt);

    return { .m_modifiedMir = modifiedRes, .m_executed = true, .m_succeeded = true };
}

std::vector<std::type_index> MirInstructionSelectorPass::getDependencies() const
{
    return { std::type_index(typeid(MirBlockLegalizerPass)),
             std::type_index(typeid(MirFunctionSignatureLegalizerPass)),
             std::type_index(typeid(FunctionAbiLowererPass)) };
}
