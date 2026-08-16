#include "FrameLowerer/MirFrameLowerer.h"

void MirFrameLowerer::calculateFrameLayout(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    CallingConvDesc *cc = func->getCallingConv();
    MirFunctionAnalysisData *analysisData = func->getAnalysisData();
    const size_t slotSize = targetDesc->getStackSlotSize();
    const size_t stackAlign = cc->getStackAlignment();

    // Calculate Callee-saved register push area
    const auto &usedCalleeSavedRegs = func->getUsedCalleeSavedRegs();
    analysisData->m_calleeSavedAreaSize = usedCalleeSavedRegs.size() * slotSize;

    // Use unsigned arithmetic during offset computation to avoid sign-extension masking issues
    size_t currentOffset = analysisData->m_calleeSavedAreaSize;
    const bool growsDown = cc->doesStackGrowsDownwards();

    for (StackFrameObject *obj : func->getStackFrame()->getObjects())
    {
        size_t objSize = obj->m_type->getTotalSizeInBytes();
        size_t objAlign = std::max(obj->m_type->getMaxAlignmentInBytes(), slotSize);

        // Align current byte offset upwards to object's alignment requirement
        currentOffset = (currentOffset + objAlign - 1) & ~(objAlign - 1);

        // Assign object offset relative to Frame Pointer (FP)
        if (growsDown)
        {
            obj->m_offset = -static_cast<int64_t>(currentOffset); // Negative displacement from FP
        }
        else
        {
            obj->m_offset = static_cast<int64_t>(currentOffset); // Positive displacement
        }

        auto log = ctx.m_ctx->getDiagCollector()->builder(Diag_Trace, "MirFrameLowerer");
        log << "Lowered stack frame object" << func->getSourceRef();
        log.appendNote(MirPrinter::printToString(obj).c_str(), func->getSourceRef());

        currentOffset += objSize;
    }

    // Include ABI shadow space (e.g., 32 bytes on Win64 ABI)
    currentOffset += cc->getShadowSpaceSize();

    // Round total frame payload up to required ABI stack boundary (e.g., 16 bytes)
    analysisData->m_totalFrameSize = (currentOffset + stackAlign - 1) & ~(stackAlign - 1);

    auto log = ctx.m_ctx->getDiagCollector()->builder(Diag_Trace, "MirFrameLowerer");
    log << "Calculated stack frame layout" << func->getSourceRef();
    log.appendNote(std::format("Callee save size: {:X}", analysisData->m_calleeSavedAreaSize).c_str(),
                   func->getSourceRef());
    log.appendNote(std::format("Total size: {:X}", analysisData->m_totalFrameSize).c_str(), func->getSourceRef());
}

void MirFrameLowerer::lowerStackObjectReferences(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    CallingConvDesc *cc = func->getCallingConv();
    MirOperandBuilder opBuilder(ctx.m_ctx);

    const size_t slotSize = targetDesc->getStackSlotSize();
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getIntegerTypeBySize(slotSize);

    // Determine the base register designated by the ABI (FP if enabled, otherwise SP)
    RegisterRef baseRegRef = cc->hasFramePointer(func) ? cc->getFramePointerReg() : cc->getStackPointerReg();
    MirRegister *baseReg = opBuilder.buildPhysReg(ptrType, baseRegRef.getId(), "", baseRegRef.getClass());

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructionList = block->getInstructions();
        for (MirInstruction *inst : instructionList)
        {
            bool instructionModified = false;
            auto &operands = inst->getOperands();

            for (size_t i = 0; i < operands.size(); ++i)
            {
                MirOperand *op = operands[i];
                if (!op || !op->isOfType<MirReference>())
                {
                    continue;
                }

                MirReference *ref = op->get<MirReference>();

                // Only process references that represent abstract Stack Frame Objects
                if (ref->getRefType() == MirReferenceType::StackFrameObject)
                {
                    size_t objId = ref->getRefId();

                    // Retrieve the corresponding StackFrameObject from the function's stack frame
                    StackFrameObject *stackObj = func->getStackFrame()->getObjectFromId(objId);
                    if (!stackObj)
                    {
                        auto log = ctx.m_ctx->getDiagCollector()->builder(Diag_Error, "MirFrameLowerer");
                        log << "Could not retrieve stack frame object out of given reference" << ref->getSourceRef();
                        log.appendNote(MirPrinter::printToString(ref).c_str(), ref->getSourceRef());
                        continue;
                    }

                    // Get the concrete offset computed during calculateFrameLayout
                    MirInteger *imm = opBuilder.buildInt(targetDesc->getMemOperandDisplacementType(),
                                                         FlexInt(stackObj->m_offset));
                    MirMemory *memOp = opBuilder.buildMem(ref->getMirType(), baseReg, imm);

                    // Replace abstract stack reference with concrete memory operand
                    operands[i] = memOp;
                    instructionModified = true;
                }
            }

            // Invalidate use-def caches if operands were rewritten
            if (instructionModified)
            {
                inst->invalidateCachedUsedAndDefs();
            }
        }
    }
}