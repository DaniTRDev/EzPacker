#include "FrameLowerer/MirFrameLowererPass.h"

#include <ranges>

MirFrameLowererPass::MirFrameLowererPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_result(ctx->getGlobalAllocator()), m_targetDesc(targetDesc)
{
}

const char *MirFrameLowererPass::getName() const { return "FrameLowererPass"; }

const MirFrameLowererPassResult &MirFrameLowererPass::getResult() { return m_result; }

MirPassIterationPlace MirFrameLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult MirFrameLowererPass::run(std::pmr::list<MirFunction *> &funcList,
                                       std::pmr::list<MirFunction *>::iterator it,
                                       MirPassManager *passManager)
{
    MirFunction *func = *it;
    if (!func || !m_targetDesc)
    {
        return { .m_modifiedMir = false, .m_executed = false, .m_succeeded = false };
    }

    // Prepare frame lowerer context using temporary pass memory resource
    FrameLowererCtx ctx(m_ctx, func, m_targetDesc, m_ctx->getGlobalAllocator());
    MirFrameLowerer *lowerer = m_targetDesc->getFrameLowerer();

    // Scan for ALLOC/DEALLOC instructions and lower them.
    for (MirBlock *block : func->getBlocks())
    {
        auto &instrList = block->getInstructions();
        auto it = instrList.begin();
        while (it != instrList.end())
        {
            MirInstruction *instr = *it;
            ctx.m_allocIt = it;

            if (instr->getOpCode() == MirInstructionOpCode::ALLOC)
                lowerer->lowerAlloc(ctx);
            else if (instr->getOpCode() == MirInstructionOpCode::DALLOC)
                lowerer->lowerDAlloc(ctx);

            it++;
        }
    }

    // Compute frame dimensions and local object offsets
    // Insert function entry prologue (PUSH FP, callee-saved pushes, SUB SP)
    // Insert function exit epilogue (ADD SP, callee-saved pops, POP FP) before return instructions
    // Lower abstract %stack[N] references into concrete memory operands [baseReg + offset]

    lowerer->calculateFrameLayout(ctx);
    lowerer->insertPrologue(ctx);
    lowerer->insertEpilogue(ctx);
    lowerer->lowerStackObjectReferences(ctx);

    m_result.m_layouts[func] = ctx.m_layout;
    return { .m_modifiedMir = true, .m_executed = true, .m_succeeded = true };
}

void MirFrameLowererPass::printResult() const
{
    auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "MirFrameLowererPass");
    diag << "Printing MirFrameLowererPass result:";

    const auto &layouts = m_result.m_layouts;
    for (auto &func : layouts | std::ranges::views::keys)
    {
        diag.appendNote(MirPrinter::printToString(func, MirPrinterDetail::Detailed).c_str(), func->getSourceRef());
    }
}

void MirFrameLowererPass::reset() { m_result.m_layouts.clear(); }

std::vector<std::type_index> MirFrameLowererPass::getDependencies() const
{
    // Frame lowering MUST execute after register allocation!
    return { std::type_index(typeid(MirRegisterAllocatorPass)) };
}