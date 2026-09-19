#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "FrameLowerer/MirFrameLowerer.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Printer/MirPrinter.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"

/**
 * Stores the builder context and target descriptor used to build the per-function frame context.
 */
MirFrameLowererPass::MirFrameLowererPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc)
{
}

/// Returns the diagnostic name of this pass.
const char *MirFrameLowererPass::getName() const { return "FrameLowererPass"; }

/// Runs once per function rather than once per module.
MirPassIterationPlace MirFrameLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

/**
 * Lowers allocations, computes the frame layout, emits the prologue/epilogue and replaces stack
 * object references for the given function.
 */
MirPassResult MirFrameLowererPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it, MirPassManager *passManager)
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
            auto currIt = it++;
            ctx.m_allocIt = currIt;

            if (instr && instr->getOpCode() == MirInstructionOpCode::ALLOC)
            {
                lowerer->lowerAlloc(ctx);
            }
            else if (instr && instr->getOpCode() == MirInstructionOpCode::DALLOC)
            {
                lowerer->lowerDAlloc(ctx);
            }
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

/**
 * Emits a debug dump of each lowered function to the diagnostic collector.
 */
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

/// Drops the list of lowered functions so the pass can run again on a fresh module.
void MirFrameLowererPass::reset() { m_loweredFunctions.clear(); }

/**
 * Declares that this pass must run after register allocation so spill slots already exist.
 */
std::vector<std::type_index> MirFrameLowererPass::getDependencies() const
{
    // Frame lowering MUST execute after register allocation!
    return { std::type_index(typeid(class MirRegisterAllocatorPass)) };
}