#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "FrameLowerer/MirFrameLowerer.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Printer/MirPrinter.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"

MirFrameLowererPass::MirFrameLowererPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc)
{
}

const char *MirFrameLowererPass::getName() const { return "FrameLowererPass"; }

MirPassIterationPlace MirFrameLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult MirFrameLowererPass::run(IntrusiveLinkedList<MirFunction> &funcList,
                                        IntrusiveLinkedList<MirFunction>::iterator it,
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

            if (!lowerer->lowerAlloc(ctx))
            {
                lowerer->lowerDAlloc(ctx);
            }

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
    m_loweredFunctions.push_back(func);

    return { .m_modifiedMir = true, .m_executed = true, .m_succeeded = true };
}

void MirFrameLowererPass::printResult()
{
    auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirFrameLowererPass");
    log << std::format("Printing frame lowerer result").c_str();

    for (auto &func : m_loweredFunctions)
    {
        std::string str = MirPrinter::printToString(func, MirPrinterDetail::Detailed);
        log.appendNote(func->getSourceRef(), "{}", str);
    }
}

void MirFrameLowererPass::reset() { m_loweredFunctions.clear(); }

std::vector<std::type_index> MirFrameLowererPass::getDependencies() const
{
    // Frame lowering MUST execute after register allocation!
    return { std::type_index(typeid(class MirRegisterAllocatorPass)) };
}