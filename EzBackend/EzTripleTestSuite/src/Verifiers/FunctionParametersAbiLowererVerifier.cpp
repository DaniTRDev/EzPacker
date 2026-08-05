#include "Verifiers/FunctionParametersAbiLowererVerifier.h"

FunctionArgAbiLowererVerifier::FunctionArgAbiLowererVerifier(MirBuilderContext *ctx, FunctionAbiLowererPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

FunctionArgAbiLowererVerifier &
FunctionArgAbiLowererVerifier::verifyLoweredFunctionArguments(MirBlock *entryBlock,
                                                              const std::vector<MirOperand *> &origPopArgs)
{
    MirFunction *func = entryBlock->getOwner();
    CallingConvDesc *cc = func->getCallingConv();

    auto &instructions = entryBlock->getInstructions();

    // 1. Ensure no POP_ARG instructions remain anywhere in the entry block
    for (const MirInstruction *instr : instructions)
    {
        EXPECT_NE(instr->getOpCode(), MirInstructionOpCode::POP_ARG)
                << "POP_ARG instruction was not erased during processFunctionArguments execution.";
    }

    CallLoweringState verifyState(cc, m_ctx);

    // Compute expected instruction count inserted at the top of the entry block
    size_t expectedPrepInstrs = 0;
    for (MirOperand *argVal : origPopArgs)
    {
        ArgumentLocationDesc loc = cc->getArgLoc(argVal->getMirType(), &verifyState);
        switch (loc.getType())
        {
            case ArgLocationType::Register:
                expectedPrepInstrs += 1; // MOV vreg, physReg
                break;
            case ArgLocationType::Split:
                expectedPrepInstrs += loc.getSplit().m_parts.size(); // STORE [vreg + offset], physReg
                break;
            case ArgLocationType::Indirect:
                expectedPrepInstrs += 1; // MOV vreg, physReg
                break;
            case ArgLocationType::Stack:
                expectedPrepInstrs += 1; // LOAD vreg, [stackSlot]
                break;
            default:
                break;
        }
    }

    // 2. Start checking from the very first instruction in the entry block
    auto prepIt = instructions.begin();
    CallLoweringState walkState(cc, m_ctx);

    // 3. Verify incoming parameter setup instructions in order
    for (size_t argIdx = 0; argIdx < origPopArgs.size(); ++argIdx)
    {
        MirOperand *destVal = origPopArgs[argIdx];
        MirType *argType = destVal->getMirType();
        ArgumentLocationDesc loc = cc->getArgLoc(argType, &walkState);

        switch (loc.getType())
        {
            case ArgLocationType::Register:
            {
                const RegLoc &reg = loc.getReg();
                EXPECT_NE(prepIt, instructions.end()) << "Missing instruction for register parameter " << argIdx;
                MirInstruction *movInstr = *prepIt++;

                MirInstructionVerifier(movInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

                // Operand 0: Virtual register receiving the parameter
                EXPECT_EQ(movInstr->getOperands()[0], destVal) << "MOV destination mismatch for parameter " << argIdx;

                // Operand 1: Incoming physical parameter register
                EXPECT_TRUE(movInstr->getOperands()[1]->isOfType<MirRegister>())
                        << "MOV source must be a physical register for parameter " << argIdx;
                MirRegister *srcReg = movInstr->getOperands()[1]->get<MirRegister>();
                EXPECT_FALSE(srcReg->isVirtual()) << "Source register must be physical for parameter " << argIdx;
                EXPECT_EQ(srcReg->getRef(), reg.m_ref) << "Physical register ID mismatch for parameter " << argIdx;
                break;
            }

            case ArgLocationType::Split:
            {
                const SplitLoc &split = loc.getSplit();
                MirRegister *destReg = destVal->get<MirRegister>();
                EXPECT_NE(destReg, nullptr) << "Split parameter destination " << argIdx << " must be a register.";

                for (size_t p = 0; p < split.m_parts.size(); ++p)
                {
                    const SplitPiece &piece = split.m_parts[p];
                    EXPECT_NE(prepIt, instructions.end()) << "Missing instruction for split part " << p;
                    MirInstruction *storeInstr = *prepIt++;

                    MirInstructionVerifier(storeInstr).opcode(MirInstructionOpCode::STORE).operandCount(2);

                    // Operand 0: Memory destination [destReg + offset]
                    EXPECT_TRUE(storeInstr->getOperands()[0]->isOfType<MirMemory>())
                            << "STORE destination must be a memory operand for split part " << p;
                    auto *memOp = storeInstr->getOperands()[0]->get<MirMemory>();
                    EXPECT_EQ(memOp->getBase(), destReg) << "Base register mismatch for split store at part " << p;
                    EXPECT_EQ(memOp->getDisplacement()->getValue(), FlexInt(piece.m_offsetInParam))
                            << "Byte offset mismatch for split store at part " << p;

                    // Operand 1: Physical register providing the chunk
                    EXPECT_TRUE(storeInstr->getOperands()[1]->isOfType<MirRegister>())
                            << "STORE source must be a physical register for split part " << p;
                    MirRegister *srcReg = storeInstr->getOperands()[1]->get<MirRegister>();
                    EXPECT_FALSE(srcReg->isVirtual());
                    EXPECT_EQ(srcReg->getRef(), piece.m_reg) << "Split physical register ID mismatch at part " << p;
                }
                break;
            }

            case ArgLocationType::Indirect:
            {
                const IndirectLoc &indirect = loc.getIndirect();
                EXPECT_NE(prepIt, instructions.end()) << "Missing instruction for indirect parameter " << argIdx;
                MirInstruction *movInstr = *prepIt++;

                MirInstructionVerifier(movInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

                // Operand 0: Destination virtual register receiving pointer
                EXPECT_EQ(movInstr->getOperands()[0], destVal)
                        << "Indirect MOV destination mismatch for parameter " << argIdx;

                // Operand 1: Physical register holding incoming pointer
                EXPECT_TRUE(movInstr->getOperands()[1]->isOfType<MirRegister>())
                        << "Indirect MOV source must be a physical register.";
                MirRegister *srcReg = movInstr->getOperands()[1]->get<MirRegister>();
                EXPECT_FALSE(srcReg->isVirtual());
                EXPECT_EQ(srcReg->getRef(), indirect.m_pointerStorage)
                        << "Indirect pointer storage physical register mismatch for parameter " << argIdx;
                break;
            }

            case ArgLocationType::Stack:
            {
                const StackLoc &stack = loc.getStack();
                EXPECT_NE(prepIt, instructions.end()) << "Missing instruction for stack parameter " << argIdx;
                MirInstruction *loadInstr = *prepIt++;

                MirInstructionVerifier(loadInstr).opcode(MirInstructionOpCode::LOAD).operandCount(2);

                // Operand 0: Destination virtual register
                EXPECT_EQ(loadInstr->getOperands()[0], destVal)
                        << "Stack parameter LOAD destination mismatch for parameter " << argIdx;

                // Operand 1: Stack parameter reference object
                EXPECT_TRUE(loadInstr->getOperands()[1]->isOfType<MirReference>())
                        << "Stack parameter LOAD source must be a stack frame reference for parameter " << argIdx;
                break;
            }

            default:
                EXPECT_TRUE(false) << "Unhandled ArgLocationType during FunctionArg verification.";
        }
    }

    return *this;
}