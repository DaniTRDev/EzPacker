#include "TargetStackFrameLowerer/StackFrameLowerer.h"

StackFrameLowererPass::StackFrameLowererPass(StackFrameLowererContext *ctx) : m_ctx(ctx) {}

bool StackFrameLowererPass::run(TypedPoolLinkedList<struct MirFunction> *funcList,
                                TypedPoolLinkedList<struct MirFunction>::Iterator it,
                                struct MirPassManager *passManager)
{
    MirFunction *func = *it;
    MirEmitterContext *ctx = m_ctx->getEmitter()->getContext();

    // Calculate stack frame offsets for all StackFrameObjects in the function, and determine the total size of the
    // stack frame.
    if (!calculateStackFrameOffsets(func))
    {
        // TODO: Show error.
        return false;
    }

    // Insert the Prologue at the very beginning of the Entry Block
    MirBlock *entryBlock = func->getEntryPoint();
    if (entryBlock->getInstructions()->m_numElems > 0)
    {
        // Bind the emitter to insert BEFORE the first instruction
        ctx->setInsertPoint(entryBlock, entryBlock->getInstructions()->begin());
    }
    else
        ctx->setInsertPoint(entryBlock);

    insertPrologue(); // Emits SUB SP, FP, etc.

    // Trace every RET instruction to insert the Epilogue
    for (MirBlock *block : *func->getBlocks())
    {
        auto *instrList = block->getInstructions();

        // Skip empty blocks
        if (instrList->m_numElems == 0)
            continue;

        auto *tailNode = instrList->m_tail;
        MirInstruction *lastInstr = tailNode->m_object;

        // Check if the block terminates with a Return
        if (lastInstr->getOpCode() == MirInstructionOpCode::RET)
        {
            // Construct a forward iterator pointing directly at the RET node
            TypedPoolLinkedList<MirInstruction>::Iterator lastInstrIt{ tailNode };

            // Bind the emitter to insert BEFORE the RET instruction
            ctx->setInsertPoint(block, lastInstrIt);

            // Emits MOV SP, FP; LOAD FP, [SP]; ADD SP, fpSize
            insertEpilogue();
        }
    }

    return true;
}

bool StackFrameLowererPass::calculateStackFrameOffsets(MirFunction *func)
{
    ABIDesc *abiDesc = m_ctx->getTargetDesc()->getABI();
    const auto &stackLayout = abiDesc->getStackLayout();

    MirFunctionStackFrame *stackFrame = func->getStackFrame();
    auto stackObjList = stackFrame->getStackFrameObjects();

    // Track intervals of physical stack bytes that are locked down: [StartOffset, EndOffset]
    std::vector<std::pair<int64_t, int64_t>> reservedIntervals;

    // Process and lock fixed offsets
    for (auto stackObjIt = stackObjList->begin(); stackObjIt != stackObjList->end(); ++stackObjIt)
    {
        StackFrameObject *obj = *stackObjIt;
        if (obj->m_offset != 0)
        {
            int64_t start = obj->m_offset;
            int64_t end = start + static_cast<int64_t>(obj->m_sizeInBytes);

            // TODO: Report errors on collisions
            reservedIntervals.push_back({ start, end });
        }
    }

    // Sort intervals by start address to make scanning for free space easy
    std::sort(reservedIntervals.begin(), reservedIntervals.end());

    // Helper lambda to check if a proposed frame window collides with any fixed objects
    auto collidesWithReserved = [&](int64_t start, int64_t end)
    {
        for (const auto &interval : reservedIntervals)
        {
            // Overlap condition: start1 < end2 AND start2 < end1
            if (start < interval.second && interval.first < end)
            {
                return true;
            }
        }
        return false;
    };

    // Dynamically pack unallocated items
    int64_t currentOffset = stackLayout.alignAddress(0);

    for (auto stackObjIt = stackObjList->begin(); stackObjIt != stackObjList->end(); ++stackObjIt)
    {
        StackFrameObject *obj = *stackObjIt;

        // Skip objects that were already handled in Phase 1
        if (obj->m_offset != 0)
        {
            continue;
        }

        // Keep pushing the object upward until we find a gap that
        // doesn't collide with fixed layout constraints
        while (true)
        {
            int64_t proposedStart = stackLayout.alignAddress(currentOffset);
            int64_t proposedEnd = proposedStart + static_cast<int64_t>(obj->m_sizeInBytes);

            if (!collidesWithReserved(proposedStart, proposedEnd))
            {
                // We found a safe gap! Assign the finalized physical offset to the MIR object
                obj->m_offset = proposedStart;
                currentOffset = proposedEnd;
                break;
            }

            // If it collides, step forward by the target's minimal stack alignment unit
            currentOffset = stackLayout.alignAddress(currentOffset + 1);
        }
    }

    m_stackFrameEndOffset = abiDesc->getStackLayout().alignAddress(currentOffset);
    return true;
}

bool StackFrameLowererPass::insertPrologue()
{
    ABIDesc *abiDesc = m_ctx->getTargetDesc()->getABI();
    MirEmitter *emitter = m_ctx->getEmitter();

    MirType *regType = emitter->getContext()->getIntegerTypeBySize(abiDesc->getRegSizeInBits() / 8);

    MirRegister *sp = emitter->createPhysicalRegister(regType, abiDesc->getStackReg());
    MirRegister *fp = emitter->createPhysicalRegister(regType, abiDesc->getStackFrameReg());

    int64_t fpSize = abiDesc->getRegSizeInBits() / 8;

    // Allocate space on the stack for the old frame pointer
    MirInteger *fpSizeImm = emitter->createImmediateInteger(regType, fpSize);
    emitter->emitSUB(sp, fpSizeImm); // sp = sp - fpSize

    // Store the old frame pointer at [SP]
    MirInteger *zeroOffset = emitter->createImmediateInteger(regType, 0);
    MirMemory *mem = emitter->createMemoryOperand(regType, sp, zeroOffset);
    emitter->emitSTORE(mem, fp);

    // 3. Establish the new frame pointer (FP = SP)
    emitter->emitMOV(fp, sp);

    // Allocate the rest of the stack frame for local variables/spills
    if (m_stackFrameEndOffset > 0)
    {
        MirInteger *frameSizeImm = emitter->createImmediateInteger(regType, m_stackFrameEndOffset);
        emitter->emitSUB(sp, frameSizeImm); // sp = sp - localVarsSize
    }

    return true;
}

bool StackFrameLowererPass::insertEpilogue()
{
    ABIDesc *abiDesc = m_ctx->getTargetDesc()->getABI();
    MirEmitter *emitter = m_ctx->getEmitter();

    MirType *regType = emitter->getContext()->getIntegerTypeBySize(abiDesc->getRegSizeInBits() / 8);

    MirRegister *sp = emitter->createPhysicalRegister(regType, abiDesc->getStackReg());
    MirRegister *fp = emitter->createPhysicalRegister(regType, abiDesc->getStackFrameReg());

    int64_t fpSize = abiDesc->getRegSizeInBits() / 8;

    // Discard local variables by restoring SP to where the old FP is saved
    emitter->emitMOV(sp, fp); // sp = fp

    // Load the caller's frame pointer back from [SP]
    MirInteger *zeroOffset = emitter->createImmediateInteger(regType, 0);
    MirMemory *mem = emitter->createMemoryOperand(regType, sp, zeroOffset);
    emitter->emitLOAD(fp, mem); // fp = [sp]

    // Reclaim the space used by the saved frame pointer
    MirInteger *fpSizeImm = emitter->createImmediateInteger(regType, fpSize);
    emitter->emitADD(sp, fpSizeImm); // sp = sp + fpSize

    return true;
}

MirPassIterationPlace StackFrameLowererPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

const char *StackFrameLowererPass::getName() const { return "StackFrameLowererPass"; }
