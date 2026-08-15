#include "Verifiers/FrameLowererPassVerifier.h"

using namespace EzTestTriple;
namespace
{
bool isPushInst(const MirInstruction *inst)
{
    if (inst->getOpCode() == MirInstructionOpCode::PUSH)
        return true;
    if (inst->getOpCode() == MirInstructionOpCode::TARGET_INST)
        return inst->getTargetDesc() == TargetInst::PUSH64r;
    return false;
}

bool isPopInst(const MirInstruction *inst)
{
    if (inst->getOpCode() == MirInstructionOpCode::POP)
        return true;
    if (inst->getOpCode() == MirInstructionOpCode::TARGET_INST)
        return inst->getTargetDesc() == TargetInst::POP64r;
    return false;
}

bool isMovInst(const MirInstruction *inst)
{
    if (inst->getOpCode() == MirInstructionOpCode::MOV)
        return true;
    if (inst->getOpCode() == MirInstructionOpCode::TARGET_INST)
    {
        auto tDesc = inst->getTargetDesc();
        return tDesc == TargetInst::MOV64rr || tDesc == TargetInst::MOV32rr || tDesc == TargetInst::MOV16rr ||
                tDesc == TargetInst::MOV8rr || tDesc == TargetInst::MOV64rm;
    }
    return false;
}

bool isSubInst(const MirInstruction *inst)
{
    if (inst->getOpCode() == MirInstructionOpCode::SUB)
        return true;
    if (inst->getOpCode() == MirInstructionOpCode::TARGET_INST)
    {
        auto tDesc = inst->getTargetDesc();
        return tDesc == TargetInst::SUB64rr || tDesc == TargetInst::SUB32rr || tDesc == TargetInst::SUB16rr ||
                tDesc == TargetInst::SUB8rr;
    }
    return false;
}

bool isAddInst(const MirInstruction *inst)
{
    if (inst->getOpCode() == MirInstructionOpCode::ADD)
        return true;
    if (inst->getOpCode() == MirInstructionOpCode::TARGET_INST)
    {
        auto tDesc = inst->getTargetDesc();
        return tDesc == TargetInst::ADD64rr || tDesc == TargetInst::ADD32rr || tDesc == TargetInst::ADD16rr ||
                tDesc == TargetInst::ADD8rr;
    }
    return false;
}

bool isRetInst(const MirInstruction *inst)
{
    if (inst->getOpCode() == MirInstructionOpCode::RET || (inst->getFlags() & MirInstructionFlags::IsReturn))
        return true;
    if (inst->getOpCode() == MirInstructionOpCode::TARGET_INST)
    {
        return inst->getTargetDesc() == TargetInst::RET;
    }
    return false;
}

bool isLeaInst(const MirInstruction *inst) { return inst->getOpCode() == MirInstructionOpCode::LEA; }
} // anonymous namespace

FrameLowererPassVerifier::FrameLowererPassVerifier(MirBuilderContext *ctx, MirFrameLowererPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyPrologue(MirFunction *func)
{
    CallingConvDesc *cc = func->getCallingConv();
    MirFunctionAnalysisData *analysisData = func->getAnalysisData();
    EXPECT_NE(cc, nullptr) << "Function must have a valid CallingConvDesc.";

    MirBlock *entryBlock = func->getEntryPoint();
    EXPECT_NE(entryBlock, nullptr) << "Function entry point block cannot be nullptr.";

    const auto &instructions = entryBlock->getInstructions();
    EXPECT_FALSE(instructions.empty()) << "Entry block instructions cannot be empty after frame lowering.";

    auto instIt = instructions.begin();
    RegisterRef fpRegRef = cc->getFramePointerReg();
    RegisterRef spRegRef = cc->getStackPointerReg();

    const bool useFramePointer = cc->hasFramePointer(func) || analysisData->m_hasDynamicAllocs;

    // 1. Verify Frame Pointer Setup: PUSH RFP -> MOV RFP, RSP
    if (useFramePointer)
    {
        EXPECT_NE(instIt, instructions.end()) << "Expected PUSH FP instruction in prologue.";
        MirInstruction *pushFpInstr = *instIt++;
        EXPECT_TRUE(isPushInst(pushFpInstr)) << "Expected PUSH instruction for FP setup in prologue.";

        MirOperand *op0 = pushFpInstr->getOperands()[0];
        EXPECT_TRUE(op0->isOfType<MirRegister>()) << "PUSH operand must be a physical register.";
        MirRegister *regOp = op0->get<MirRegister>();
        EXPECT_FALSE(regOp->isVirtual()) << "Frame pointer register must be physical.";
        EXPECT_EQ(regOp->getRef(), fpRegRef) << "Pushed frame pointer register mismatch.";

        EXPECT_NE(instIt, instructions.end()) << "Expected MOV FP, SP instruction in prologue.";
        MirInstruction *movFpInstr = *instIt++;
        EXPECT_TRUE(isMovInst(movFpInstr)) << "Expected MOV instruction for FP setup in prologue.";

        MirRegister *destReg = movFpInstr->getOperands()[0]->get<MirRegister>();
        MirRegister *srcReg = movFpInstr->getOperands()[1]->get<MirRegister>();
        EXPECT_EQ(destReg->getRef(), fpRegRef) << "MOV destination must be frame pointer.";
        EXPECT_EQ(srcReg->getRef(), spRegRef) << "MOV source must be stack pointer.";
    }

    // 2. Verify Callee-Saved Physical Register Pushes
    const auto &usedCalleeSaved = func->getUsedCalleeSavedRegs();
    for (const auto &regRef : usedCalleeSaved)
    {
        if (useFramePointer && regRef == fpRegRef)
            continue;

        EXPECT_NE(instIt, instructions.end())
                << "Expected PUSH instruction for callee-saved register %p" << regRef.getId();
        MirInstruction *pushInstr = *instIt++;
        EXPECT_TRUE(isPushInst(pushInstr)) << "Expected PUSH opcode for callee-saved register %p" << regRef.getId();

        MirRegister *pushedReg = pushInstr->getOperands()[0]->get<MirRegister>();
        EXPECT_FALSE(pushedReg->isVirtual()) << "Callee-saved register must be physical.";
        EXPECT_EQ(pushedReg->getRef(), regRef) << "Callee-saved register push mismatch.";
    }

    // 3. Verify Stack Payload Allocation: SUB SP, stackAllocSize
    int64_t stackAllocSize = (analysisData->m_totalFrameSize > analysisData->m_calleeSavedAreaSize)
            ? static_cast<int64_t>(analysisData->m_totalFrameSize - analysisData->m_calleeSavedAreaSize)
            : 0;

    if (stackAllocSize > 0)
    {
        EXPECT_NE(instIt, instructions.end()) << "Expected SUB SP instruction for stack payload allocation.";
        MirInstruction *subInstr = *instIt++;
        EXPECT_TRUE(isSubInst(subInstr)) << "Expected SUB opcode for stack payload allocation.";

        MirRegister *spReg = subInstr->getOperands()[0]->get<MirRegister>();
        EXPECT_EQ(spReg->getRef(), spRegRef) << "SUB destination register must be stack pointer.";

        MirOperand *immOrRegOp = subInstr->getOperands()[1];
        if (immOrRegOp->isOfType<MirInteger>())
        {
            auto *immVal = immOrRegOp->get<MirInteger>();
            EXPECT_EQ(immVal->getValue(), FlexInt(stackAllocSize, 64))
                    << "Stack allocation immediate size mismatch in prologue.";
        }
        else
        {
            EXPECT_TRUE(immOrRegOp->isOfType<MirRegister>())
                    << "Large frame SUB operand must be a materialized scratch register.";
        }
    }

    return *this;
}

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyEpilogue(MirFunction *func)
{
    CallingConvDesc *cc = func->getCallingConv();
    MirFunctionAnalysisData *analysisData = func->getAnalysisData();
    EXPECT_NE(cc, nullptr) << "Function must have a valid CallingConvDesc.";

    RegisterRef fpRegRef = cc->getFramePointerReg();
    RegisterRef spRegRef = cc->getStackPointerReg();
    const auto &usedCalleeSaved = func->getUsedCalleeSavedRegs();

    int64_t stackAllocSize = (analysisData->m_totalFrameSize > analysisData->m_calleeSavedAreaSize)
            ? static_cast<int64_t>(analysisData->m_totalFrameSize - analysisData->m_calleeSavedAreaSize)
            : 0;

    const bool useFramePointer = cc->hasFramePointer(func) || analysisData->m_hasDynamicAllocs;
    bool returnFound = false;

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto it = instructions.begin(); it != instructions.end(); ++it)
        {
            MirInstruction *inst = *it;

            if (!isRetInst(inst))
                continue;

            returnFound = true;
            auto prepIt = it;

            size_t expectedEpilogueInstrs = 0;

            if (analysisData->m_hasDynamicAllocs || useFramePointer)
            {
                expectedEpilogueInstrs += 1;
            }
            else if (stackAllocSize > 0)
            {
                expectedEpilogueInstrs += 1;
            }

            for (const auto &regRef : usedCalleeSaved)
            {
                if (useFramePointer && regRef == fpRegRef)
                    continue;
                expectedEpilogueInstrs += 1;
            }

            if (useFramePointer)
            {
                expectedEpilogueInstrs += 1;
            }

            for (size_t i = 0; i < expectedEpilogueInstrs; ++i)
            {
                EXPECT_NE(prepIt, instructions.begin()) << "Fewer epilogue instructions before return than expected.";
                --prepIt;
            }

            // 1. Stack Pointer Restoration
            if (analysisData->m_hasDynamicAllocs || useFramePointer || stackAllocSize > 0)
            {
                MirInstruction *spRestInstr = *prepIt++;
                EXPECT_TRUE(isMovInst(spRestInstr) || isLeaInst(spRestInstr) || isAddInst(spRestInstr))
                        << "Epilogue must restore SP via MOV, LEA, or ADD.";

                MirRegister *spReg = spRestInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_EQ(spReg->getRef(), spRegRef) << "SP restoration destination register must be stack pointer.";
            }

            // 2. Callee-Saved Registers Restoration in Reverse Order
            for (auto regIt = usedCalleeSaved.rbegin(); regIt != usedCalleeSaved.rend(); ++regIt)
            {
                if (useFramePointer && *regIt == fpRegRef)
                    continue;

                MirInstruction *popInstr = *prepIt++;
                EXPECT_TRUE(isPopInst(popInstr)) << "Expected POP opcode for callee-saved register restoration.";

                MirRegister *poppedReg = popInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_FALSE(poppedReg->isVirtual());
                EXPECT_EQ(poppedReg->getRef(), *regIt) << "Callee-saved register pop mismatch in epilogue.";
            }

            // 3. Frame Pointer Restoration
            if (useFramePointer)
            {
                MirInstruction *popFpInstr = *prepIt++;
                EXPECT_TRUE(isPopInst(popFpInstr)) << "Expected POP opcode for FP restoration in epilogue.";

                MirRegister *poppedFp = popFpInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_EQ(poppedFp->getRef(), fpRegRef) << "POP frame pointer register mismatch in epilogue.";
            }

            EXPECT_EQ(prepIt, it) << "Epilogue instruction count mismatch before return instruction.";
        }
    }

    EXPECT_TRUE(returnFound) << "At least one return instruction must exist in function to verify epilogue.";

    return *this;
}

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyStackReferencesLowered(MirFunction *func)
{
    CallingConvDesc *cc = func->getCallingConv();
    EXPECT_NE(cc, nullptr);

    const bool useFramePointer = cc->hasFramePointer(func) || func->getAnalysisData()->m_hasDynamicAllocs;
    RegisterRef expectedBaseReg = useFramePointer ? cc->getFramePointerReg() : cc->getStackPointerReg();

    for (MirBlock *block : func->getBlocks())
    {
        for (const MirInstruction *instr : block->getInstructions())
        {
            for (size_t i = 0; i < instr->getOperands().size(); ++i)
            {
                const MirOperand *op = instr->getOperands()[i];

                if (op->isOfType<MirReference>())
                {
                    const auto *ref = op->get<MirReference>();
                    EXPECT_NE(ref->getRefType(), MirReferenceType::StackFrameObject)
                            << "Instruction still contains abstract StackObject reference at operand " << i;
                }

                if (op->isOfType<MirMemory>())
                {
                    const auto *memOp = op->get<MirMemory>();
                    EXPECT_EQ(memOp->getBase()->getRef(), expectedBaseReg)
                            << "Memory operand base register mismatch in instruction: " << instr->toString();
                }
            }
        }
    }

    return *this;
}

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyDAllocLowered(MirFunction *func)
{
    CallingConvDesc *cc = func->getCallingConv();
    MirFunctionAnalysisData *analysisData = func->getAnalysisData();
    EXPECT_NE(cc, nullptr) << "Function must have a valid CallingConvDesc.";

    RegisterRef spRegRef = cc->getStackPointerReg();
    RegisterRef fpRegRef = cc->getFramePointerReg();

    for (MirBlock *block : func->getBlocks())
    {
        for (const MirInstruction *instr : block->getInstructions())
        {
            EXPECT_NE(instr->getOpCode(), MirInstructionOpCode::DALLOC)
                    << "Instruction still contains unlowered DALLOC opcode!";

            if (isSubInst(instr))
            {
                const auto &operands = instr->getOperands();
                if (!operands.empty() && operands[0]->isOfType<MirRegister>())
                {
                    const auto *destReg = operands[0]->get<MirRegister>();
                    if (destReg->getRef() == spRegRef)
                    {
                        EXPECT_TRUE(analysisData->m_hasDynamicAllocs)
                                << "Dynamic SUB SP detected but func->hasDynamicAllocs is false!";
                    }
                }
            }
        }
    }

    return *this;
}