#include "Legalizer/MirFunctionSignatureLegalizerPass.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

MirFunctionSignatureLegalizerPass::MirFunctionSignatureLegalizerPass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_legalizer(targetDesc ? targetDesc->getLegalizer() : nullptr)
{
}

const char *MirFunctionSignatureLegalizerPass::getName() const { return "MirFunctionSignatureLegalizerPass"; }

MirPassIterationPlace MirFunctionSignatureLegalizerPass::getIterationPlace() const
{
    return MirPassIterationPlace::Function;
}

MirPassResult MirFunctionSignatureLegalizerPass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
                                                     MirPassManager *passManager)
{
    bool modified = false;
    bool succeeded = true;
    MirFunction *func = *it;
    if (!func)
    {
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    MirBlock *entryPoint = func->getEntryPoint();
    if (!entryPoint)
    {
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
    }

    // Check if signature is already legalized (e.g. entry begins with POP_ARG or END_ARG)
    if (!entryPoint->getInstructions().empty())
    {
        auto firstOp = entryPoint->getInstructions().front()->getOpCode();
        if (firstOp == MirInstructionOpCode::POP_ARG || firstOp == MirInstructionOpCode::END_ARG)
        {
            return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
        }
    }

    MirFunctionBuilder fBuilder(m_ctx);
    MirInstructionBuilder builder = entryPoint->getInstructions().empty()
            ? MirInstructionBuilder(m_ctx, entryPoint, InsertionType::Append)
            : MirInstructionBuilder(m_ctx,
                                    entryPoint,
                                    InsertionType::InsertBefore,
                                    entryPoint->getInstructions().begin());
    MirOperandBuilder opBuilder(m_ctx);

    CallingConvDesc *cc = func->getCallingConv();
    if (cc && func->getReturnType() && !cc->canReturnInRegs(func->getReturnType()))
    {
        // Check if SRET pointer is already present
        bool hasSretParam = !func->getParameters().empty() && func->getParameters().front()->getMirType() &&
                func->getParameters().front()->getMirType()->getKind() == MirTypeKind::Pointer;
        if (!hasSretParam)
        {
            // Allocate a hidden implicit SRET pointer register
            MirType *ptrType = m_ctx->getTypeTable()->getPtr(func->getReturnType());
            MirRegister *sretPtrParam = opBuilder.buildVReg(ptrType, "sret_ptr", func->getSourceRef());

            // Prepend this implicit argument directly to the front of the parameters
            fBuilder.addParamFront(func, sretPtrParam);
            modified = true;
        }
    }

    MirRegister *token = opBuilder.buildVReg(m_ctx->getTypeTable()->__bindToken());
    bool firstInserted = false;

    for (MirRegister *param : func->getParameters())
    {
        builder.build(MirInstructionOpCode::POP_ARG, param->getSourceRef(), { token, param });
        if (!firstInserted && !entryPoint->getInstructions().empty())
        {
            builder.changeInsertionType(InsertionType::InsertAfter);
            firstInserted = true;
        }
        modified = true;
    }

    builder.build(MirInstructionOpCode::END_ARG, func->getSourceRef(), { token });
    modified = true;

    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = succeeded };
}
