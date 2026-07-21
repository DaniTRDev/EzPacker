#include "CallAbiLowererVerifier.h"

CallAbiLowererVerifier::CallAbiLowererVerifier(MirBuilderContext *ctx, FunctionAbiLowererPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

CallAbiLowererVerifier &CallAbiLowererVerifier::verifyLoweredCall(MirBlock *targetBlock,
                                                                  const std::vector<MirOperand *> &origPushArgs)
{
    MirFunction *func = targetBlock->getOwner();
    CallingConvDesc *cc = func->getCallingConv();

    auto &instructions = targetBlock->getInstructions();
    EXPECT_FALSE(instructions.empty()) << "Target block must not be empty.";

    // 1. Ensure no PUSH_ARG instructions remain anywhere in the block
    for (const MirInstruction *instr : instructions)
    {
        EXPECT_NE(instr->getOpCode(), MirInstructionOpCode::PUSH_ARG)
                << "PUSH_ARG instruction was not erased during CallAbiLowerer execution.";
    }

    // 2. The target block or sequence must contain a CALL / CALL_INDIRECT instruction at the target site
    // Find the CALL instruction in the block
    auto callIt = instructions.end();
    for (auto it = instructions.begin(); it != instructions.end(); ++it)
    {
        MirInstructionOpCode op = (*it)->getOpCode();
        if (op == MirInstructionOpCode::CALL)
        {
            callIt = it;
            break;
        }
    }

    EXPECT_NE(callIt, instructions.end()) << "Could not find CALL instruction in target block.";
    MirInstruction *callInstr = *callIt;

    // Verify CALL instruction was standardized (token binding operands cleared)
    MirInstructionVerifier(callInstr).operandCount(0);

    CallLoweringState callState(cc->getCallerSavedGPRegs(), cc->getCallerSavedFPRegs());

    // Compute expected instruction count inserted right before CALL
    size_t expectedPrepInstrs = 0;
    for (MirOperand *argVal : origPushArgs)
    {
        ArgumentLocationDesc loc = cc->getArgLoc(argVal->getMirType(), &callState);
        switch (loc.getType())
        {
            case ArgLocationType::Register:
                expectedPrepInstrs += 1; // MOV physReg, argVal
                break;
            case ArgLocationType::Split:
                expectedPrepInstrs += loc.getSplit().m_parts.size(); // LOAD physReg, [argReg + offset]
                break;
            case ArgLocationType::Indirect:
                expectedPrepInstrs +=
                        (loc.getIndirect().m_isByVal ? 2
                                                     : 1); // STORE stack, val + MOV physReg, stack OR MOV physReg, ptr
                break;
            case ArgLocationType::Stack:
                expectedPrepInstrs += 1; // STORE stackSlot, argVal
                break;
            default:
                break;
        }
    }

    // Move iterator back to the first lowered argument instruction
    auto prepIt = callIt;
    for (size_t i = 0; i < expectedPrepInstrs; ++i)
    {
        EXPECT_NE(prepIt, instructions.begin()) << "Fewer prep instructions found before CALL than expected.";
        --prepIt;
    }

    // Reset CallLoweringState to walk the parameters symmetrically
    CallLoweringState verifyState(cc->getCallerSavedGPRegs(), cc->getCallerSavedFPRegs());

    // 3. Verify parameter setup instructions in order
    for (size_t argIdx = 0; argIdx < origPushArgs.size(); ++argIdx)
    {
        MirOperand *argVal = origPushArgs[argIdx];
        MirType *argType = argVal->getMirType();
        ArgumentLocationDesc loc = cc->getArgLoc(argType, &verifyState);

        switch (loc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = loc.getReg();
                MirInstruction *movInstr = *prepIt++;

                MirInstructionVerifier(movInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

                // Operand 0: Physical register created by oBuilder.buildPhysReg
                EXPECT_TRUE(movInstr->getOperands()[0]->isOfType<MirRegister>())
                        << "MOV destination must be a register for argument " << argIdx;
                MirRegister *destReg = movInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_FALSE(destReg->isVirtual()) << "Destination register must be physical for argument " << argIdx;
                EXPECT_EQ(destReg->getRegId(), reg.m_regId)
                        << "Physical register ID mismatch for register argument " << argIdx;

                // Operand 1: Original argument value payload
                EXPECT_EQ(movInstr->getOperands()[1], argVal) << "MOV source operand mismatch for argument " << argIdx;
                break;
            }

            case ArgLocationType::Split:
            {
                const SplitLoc &split = loc.getSplit();
                MirRegister *argReg = argVal->get<MirRegister>();
                EXPECT_NE(argReg, nullptr) << "Split argument " << argIdx << " must be a register operand.";

                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const SplitPiece &piece = split.m_parts[p];
                    MirInstruction *loadInstr = *prepIt++;

                    MirInstructionVerifier(loadInstr).opcode(MirInstructionOpCode::LOAD).operandCount(2);

                    // Operand 0: Destination physical register
                    EXPECT_TRUE(loadInstr->getOperands()[0]->isOfType<MirRegister>())
                            << "LOAD destination must be a register for split part " << p;
                    MirRegister *destReg = loadInstr->getOperands()[0]->get<MirRegister>();
                    EXPECT_FALSE(destReg->isVirtual());
                    EXPECT_EQ(destReg->getRegId(), piece.m_regId)
                            << "Split physical register ID mismatch at part " << p;

                    // Operand 1: Memory operand reading at offset
                    EXPECT_TRUE(loadInstr->getOperands()[1]->isOfType<MirMemory>())
                            << "LOAD source must be a memory operand for split part " << p;
                    auto *memOp = loadInstr->getOperands()[1]->get<MirMemory>();
                    EXPECT_EQ(memOp->getBase(), argReg) << "Base register mismatch for split load at part " << p;
                    EXPECT_EQ(memOp->getDisplacement()->getValue(), FlexInt(piece.m_offsetInParam))
                            << "Byte offset mismatch for split load at part " << p;
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = loc.getIndirect();

                if (indirect.m_isByVal)
                {
                    // Instruction A: STORE [stackFrameCopy], argVal
                    MirInstruction *storeInstr = *prepIt++;
                    MirInstructionVerifier(storeInstr).opcode(MirInstructionOpCode::STORE).operandCount(2);

                    EXPECT_TRUE(storeInstr->getOperands()[0]->isOfType<MirReference>())
                            << "ByVal STORE destination must be a stack frame reference.";
                    EXPECT_EQ(storeInstr->getOperands()[1], argVal) << "ByVal STORE source operand mismatch.";

                    // Instruction B: MOV physReg, stackFrameCopyAddr
                    MirInstruction *movInstr = *prepIt++;
                    MirInstructionVerifier(movInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

                    MirRegister *destReg = movInstr->getOperands()[0]->get<MirRegister>();
                    EXPECT_FALSE(destReg->isVirtual());
                    EXPECT_EQ(destReg->getRegId(), indirect.m_pointerStorage)
                            << "ByVal pointer storage physical register mismatch.";
                }
                else
                {
                    // Direct pointer pass MOV physReg, argVal
                    MirInstruction *movInstr = *prepIt++;
                    MirInstructionVerifier(movInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

                    MirRegister *destReg = movInstr->getOperands()[0]->get<MirRegister>();
                    EXPECT_FALSE(destReg->isVirtual());
                    EXPECT_EQ(destReg->getRegId(), indirect.m_pointerStorage);
                    EXPECT_EQ(movInstr->getOperands()[1], argVal);
                }
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = loc.getStack();
                MirInstruction *storeInstr = *prepIt++;

                MirInstructionVerifier(storeInstr).opcode(MirInstructionOpCode::STORE).operandCount(2);

                // Operand 0: Stack parameter reference object
                EXPECT_TRUE(storeInstr->getOperands()[0]->isOfType<MirReference>())
                        << "Stack parameter STORE destination must be a stack reference.";

                // Operand 1: Original argument value
                EXPECT_EQ(storeInstr->getOperands()[1], argVal) << "Stack parameter STORE source operand mismatch.";
                break;
            }

            default:
                EXPECT_TRUE(false) << "Unhandled ArgLocationType during Call verification.";
        }
    }

    // Ensure we consumed prep instructions exactly up to the CALL instruction
    EXPECT_EQ(prepIt, callIt) << "Instruction count mismatch between prep instructions and CALL location.";

    return *this;
}