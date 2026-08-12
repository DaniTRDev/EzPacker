#include "Verifiers/FrameLowererPassVerifier.h"

FrameLowererPassVerifier::FrameLowererPassVerifier(MirBuilderContext *ctx, MirFrameLowererPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyPrologue(MirFunction *func, const FrameLayout &layout)
{
    CallingConvDesc *cc = func->getCallingConv();
    EXPECT_NE(cc, nullptr) << "Function must have a valid CallingConvDesc.";

    MirBlock *entryBlock = func->getEntryPoint();
    EXPECT_NE(entryBlock, nullptr) << "Function entry point block cannot be nullptr.";

    const auto &instructions = entryBlock->getInstructions();
    EXPECT_FALSE(instructions.empty()) << "Entry block instructions cannot be empty after frame lowering.";

    auto instIt = instructions.begin();
    RegisterRef fpRegRef = cc->getFramePointerReg();
    RegisterRef spRegRef = cc->getStackPointerReg();

    const bool useFramePointer = cc->hasFramePointer(func) || layout.m_hasDynamicAllocs;

    // 1. Verify Frame Pointer Setup: PUSH FP -> MOV FP, SP
    if (useFramePointer)
    {
        EXPECT_NE(instIt, instructions.end()) << "Expected PUSH FP instruction in prologue.";
        MirInstruction *pushFpInstr = *instIt++;
        MirInstructionVerifier(pushFpInstr).opcode(MirInstructionOpCode::PUSH).operandCount(1);

        MirOperand *op0 = pushFpInstr->getOperands()[0];
        EXPECT_TRUE(op0->isOfType<MirRegister>()) << "PUSH operand must be a physical register.";
        MirRegister *regOp = op0->get<MirRegister>();
        EXPECT_FALSE(regOp->isVirtual()) << "Frame pointer register must be physical.";
        EXPECT_EQ(regOp->getRef(), fpRegRef) << "Pushed frame pointer register mismatch.";

        EXPECT_NE(instIt, instructions.end()) << "Expected MOV FP, SP instruction in prologue.";
        MirInstruction *movFpInstr = *instIt++;
        MirInstructionVerifier(movFpInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

        MirRegister *destReg = movFpInstr->getOperands()[0]->get<MirRegister>();
        MirRegister *srcReg = movFpInstr->getOperands()[1]->get<MirRegister>();
        EXPECT_EQ(destReg->getRef(), fpRegRef) << "MOV destination must be frame pointer.";
        EXPECT_EQ(srcReg->getRef(), spRegRef) << "MOV source must be stack pointer.";
    }

    // 2. Verify Callee-Saved Physical Register Pushes
    const auto &usedCalleeSaved = func->getUsedCalleeSavedRegs();
    for (const auto &regRef : usedCalleeSaved)
    {
        // Skip FP as it was explicitly verified in Step 1
        if (useFramePointer && regRef == fpRegRef)
            continue;

        EXPECT_NE(instIt, instructions.end())
                << "Expected PUSH instruction for callee-saved register %p" << regRef.getId();
        MirInstruction *pushInstr = *instIt++;
        MirInstructionVerifier(pushInstr).opcode(MirInstructionOpCode::PUSH).operandCount(1);

        MirRegister *pushedReg = pushInstr->getOperands()[0]->get<MirRegister>();
        EXPECT_FALSE(pushedReg->isVirtual()) << "Callee-saved register must be physical.";
        EXPECT_EQ(pushedReg->getRef(), regRef) << "Callee-saved register push mismatch.";
    }

    // 3. Verify Stack Payload Allocation: SUB SP, stackAllocSize
    size_t stackAllocSize = layout.totalFrameSize - layout.calleeSavedAreaSize;
    if (stackAllocSize > 0)
    {
        EXPECT_NE(instIt, instructions.end()) << "Expected SUB SP instruction for stack payload allocation.";
        MirInstruction *subInstr = *instIt++;
        MirInstructionVerifier(subInstr).opcode(MirInstructionOpCode::SUB).operandCount(2);

        MirRegister *spReg = subInstr->getOperands()[0]->get<MirRegister>();
        EXPECT_EQ(spReg->getRef(), spRegRef) << "SUB destination register must be stack pointer.";

        // Check offset operand (direct immediate or loaded via scratch register)
        MirOperand *immOrRegOp = subInstr->getOperands()[1];
        if (immOrRegOp->isOfType<MirInteger>())
        {
            auto *immVal = immOrRegOp->get<MirInteger>();
            EXPECT_EQ(immVal->getValue(), FlexInt(static_cast<int64_t>(stackAllocSize)))
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

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyEpilogue(MirFunction *func, const FrameLayout &layout)
{
    CallingConvDesc *cc = func->getCallingConv();
    EXPECT_NE(cc, nullptr) << "Function must have a valid CallingConvDesc.";

    RegisterRef fpRegRef = cc->getFramePointerReg();
    RegisterRef spRegRef = cc->getStackPointerReg();
    const auto &usedCalleeSaved = func->getUsedCalleeSavedRegs();
    size_t stackAllocSize = layout.totalFrameSize - layout.calleeSavedAreaSize;

    const bool useFramePointer = cc->hasFramePointer(func) || layout.m_hasDynamicAllocs;
    bool returnFound = false;

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto it = instructions.begin(); it != instructions.end(); ++it)
        {
            MirInstruction *inst = *it;
            if (!(inst->getFlags() & MirInstructionFlags::IsReturn))
                continue;

            returnFound = true;
            auto prepIt = it;

            // Compute expected instruction count before return
            size_t expectedEpilogueInstrs = 0;

            if (layout.m_hasDynamicAllocs)
            {
                // Dynamic Allocs: MOV SP, FP or LEA SP, [FP + offset]
                expectedEpilogueInstrs += 1;
            }
            else if (stackAllocSize > 0)
            {
                // Fixed Frame: ADD SP, stackAllocSize
                expectedEpilogueInstrs += 1;
            }

            // Callee-saved registers (excluding FP if already pushed/popped separately)
            for (const auto &regRef : usedCalleeSaved)
            {
                if (useFramePointer && regRef == fpRegRef)
                    continue;
                expectedEpilogueInstrs += 1;
            }

            if (useFramePointer)
            {
                expectedEpilogueInstrs += 1; // POP FP
            }

            // Move iterator back to the start of the epilogue sequence
            for (size_t i = 0; i < expectedEpilogueInstrs; ++i)
            {
                EXPECT_NE(prepIt, instructions.begin()) << "Fewer epilogue instructions before return than expected.";
                --prepIt;
            }

            // 1. Verify Stack Pointer Restoration (ADD SP, imm OR MOV/LEA SP, FP)
            if (layout.m_hasDynamicAllocs || stackAllocSize > 0)
            {
                MirInstruction *spRestInstr = *prepIt++;
                MirInstructionOpCode op = spRestInstr->getOpCode();
                EXPECT_TRUE(op == MirInstructionOpCode::MOV || op == MirInstructionOpCode::LEA)
                        << "Dynamic alloca epilogue must restore SP via MOV or LEA from FP.";

                MirRegister *spReg = spRestInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_EQ(spReg->getRef(), spRegRef) << "SP restoration destination register must be stack pointer.";
            }

            // 2. Verify Callee-Saved Registers Popped in REVERSE Order
            for (auto regIt = usedCalleeSaved.rbegin(); regIt != usedCalleeSaved.rend(); ++regIt)
            {
                if (useFramePointer && *regIt == fpRegRef)
                    continue;

                MirInstruction *popInstr = *prepIt++;
                MirInstructionVerifier(popInstr).opcode(MirInstructionOpCode::POP).operandCount(1);

                MirRegister *poppedReg = popInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_FALSE(poppedReg->isVirtual());
                EXPECT_EQ(poppedReg->getRef(), *regIt) << "Callee-saved register pop mismatch in epilogue.";
            }

            // 3. Verify Frame Pointer Restoration: POP FP
            if (useFramePointer)
            {
                MirInstruction *popFpInstr = *prepIt++;
                MirInstructionVerifier(popFpInstr).opcode(MirInstructionOpCode::POP).operandCount(1);

                MirRegister *poppedFp = popFpInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_EQ(poppedFp->getRef(), fpRegRef) << "POP frame pointer register mismatch in epilogue.";
            }

            EXPECT_EQ(prepIt, it) << "Epilogue instruction count mismatch before return instruction.";
        }
    }

    EXPECT_TRUE(returnFound) << "At least one return instruction must exist in function to verify epilogue.";

    return *this;
}

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyStackReferencesLowered(MirFunction *func,
                                                                                 const FrameLayout &layout)
{
    CallingConvDesc *cc = func->getCallingConv();
    EXPECT_NE(cc, nullptr);

    const bool useFramePointer = cc->hasFramePointer(func) || layout.m_hasDynamicAllocs;
    RegisterRef expectedBaseReg = useFramePointer ? cc->getFramePointerReg() : cc->getStackPointerReg();

    for (MirBlock *block : func->getBlocks())
    {
        for (const MirInstruction *instr : block->getInstructions())
        {
            for (size_t i = 0; i < instr->getOperands().size(); ++i)
            {
                const MirOperand *op = instr->getOperands()[i];

                // Ensure NO abstract StackObject references remain in any instruction operand
                if (op->isOfType<MirReference>())
                {
                    const auto *ref = op->get<MirReference>();
                    EXPECT_NE(ref->getRefType(), MirReferenceType::StackFrameObject)
                            << "Instruction '" << instr->toString() << "' in block #" << block->getId()
                            << " still contains abstract StackObject reference at operand index " << i;
                }

                // Verify that lowered memory operands reference the expected base pointer (FP/SP)
                if (op->isOfType<MirMemory>())
                {
                    const auto *memOp = op->get<MirMemory>();
                    EXPECT_EQ(memOp->getBase()->getRef(), expectedBaseReg)
                            << "Memory operand base register mismatch in instruction '" << instr->toString() << "'";
                }
            }
        }
    }

    return *this;
}

FrameLowererPassVerifier &FrameLowererPassVerifier::verifyDAllocLowered(MirFunction *func, const FrameLayout &layout)
{
    CallingConvDesc *cc = func->getCallingConv();
    EXPECT_NE(cc, nullptr) << "Function must have a valid CallingConvDesc.";

    RegisterRef spRegRef = cc->getStackPointerReg();
    RegisterRef fpRegRef = cc->getFramePointerReg();

    for (MirBlock *block : func->getBlocks())
    {
        for (const MirInstruction *instr : block->getInstructions())
        {
            // 1. Assert NO DALLOC instructions remain in any basic block
            EXPECT_NE(instr->getOpCode(), MirInstructionOpCode::DALLOC)
                    << "Instruction in block #" << block->getId()
                    << " still contains unlowered DALLOC opcode after FrameLowererPass!";

            // 2. Validate SUB SP sequence (emitted for lowered dynamic allocations)
            if (instr->getOpCode() == MirInstructionOpCode::SUB)
            {
                const auto &operands = instr->getOperands();
                if (!operands.empty() && operands[0]->isOfType<MirRegister>())
                {
                    const auto *destReg = operands[0]->get<MirRegister>();
                    if (destReg->getRef() == spRegRef)
                    {
                        // If SP is subtracted dynamically, function MUST have hasDynamicAlloca() flagged
                        EXPECT_TRUE(layout.m_hasDynamicAllocs)
                                << "Dynamic SUB SP instruction detected, but func->hasDynamicAlloca() is false!";
                    }
                }
            }

            // 3. Verify memory base registers use FP when hasDynamicAlloca() is true
            if (layout.m_hasDynamicAllocs)
            {
                for (const MirOperand *op : instr->getOperands())
                {
                    if (op->isOfType<MirMemory>())
                    {
                        const auto *memOp = op->get<MirMemory>();
                        EXPECT_EQ(memOp->getBase()->getRef(), fpRegRef)
                                << "Memory operand in block #" << block->getId()
                                << " uses SP instead of FP in a function with dynamic allocations!";
                    }
                }
            }
        }
    }

    return *this;
}