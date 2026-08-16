#include "Verifiers/ReturnAbiLowererVerifier.h"

ReturnAbiLowererVerifier::ReturnAbiLowererVerifier(MirBuilderContext *ctx, FunctionAbiLowererPass *pass) :
    m_ctx(ctx), MirPassVerifier(pass)
{
}

ReturnAbiLowererVerifier &ReturnAbiLowererVerifier::verifyLoweredReturn(MirBlock *targetBlock,
                                                                        const std::vector<MirOperand *> &origValues)
{
    MirFunction *func = targetBlock->getOwner();
    CallingConvDesc *cc = func->getCallingConv();
    MirType *retType = func->getReturnType();

    auto &instructions = targetBlock->getInstructions();
    EXPECT_FALSE(instructions.empty()) << "Block must not be empty.";

    // 1. Ensure no PUSH_RET instructions remain anywhere in the block
    for (const MirInstruction *instr : instructions)
    {
        EXPECT_NE(instr->getOpCode(), MirInstructionOpCode::PUSH_RET)
                << "PUSH_RET instruction was not erased during ReturnAbiLowererPass.";
    }

    // 2. The block must end with a standardized RET instruction
    MirInstruction *retInstr = instructions.back();
    auto retVerifier = MirInstructionVerifier(retInstr).opcode(MirInstructionOpCode::RET).operandCount(0);

    // If returning void, zero physical assignments are required
    if (retType->getKind() == MirTypeKind::Void || origValues.empty())
    {
        return *this;
    }

    CallLoweringState st(cc, m_ctx, func);
    ArgumentLocationDesc loc = cc->getReturnLoc(retType, &st);

    switch (loc.getType())
    {
        case ArgLocationType::Register:
        {
            // Verify single physical register MOV right before RET
            // [MOV physReg, origValue] -> [RET]
            auto it = std::prev(instructions.end(), 2);
            MirInstruction *movInstr = *it;

            MirInstructionVerifier(movInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

            // Operand 0: Physical register created by oBuilder.buildPhysReg
            EXPECT_TRUE(movInstr->getOperands()[0]->isOfType<MirRegister>())
                    << "MOV destination must be a physical register.";
            MirRegister *destReg = movInstr->getOperands()[0]->get<MirRegister>();
            EXPECT_FALSE(destReg->isVirtual()) << "MOV destination register must be physical.";
            EXPECT_EQ(destReg->getRef(), loc.getReg().m_ref)
                    << "Physical register ID does not match calling convention return register.";

            // Operand 1: Original return payload value
            EXPECT_EQ(movInstr->getOperands()[1], origValues[0])
                    << "MOV source operand does not match original return value.";
            break;
        }

        case ArgLocationType::Split:
        {
            const SplitLoc &split = loc.getSplit();
            size_t partCount = split.m_parts.size();

            // Verify sequential MOVs for each split part right before RET
            auto it = std::prev(instructions.end(), static_cast<int64_t>(partCount + 1));

            for (size_t p = 0; p < partCount; ++p, ++it)
            {
                MirInstruction *movInstr = *it;
                MirInstructionVerifier(movInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

                MirRegister *destReg = movInstr->getOperands()[0]->get<MirRegister>();
                EXPECT_FALSE(destReg->isVirtual()) << "Split MOV destination must be a physical register.";
                EXPECT_EQ(destReg->getRef(), split.m_parts[p].m_reg)
                        << "Split physical register ID mismatch at part " << p;

                EXPECT_EQ(movInstr->getOperands()[1], origValues[p])
                        << "Split MOV source operand mismatch at part " << p;
            }
            break;
        }

        case ArgLocationType::Indirect:
        {
            const IndirectLoc &indirect = loc.getIndirect();
            size_t storeCount = origValues.size();
            size_t expectedInstrsBeforeRet = storeCount + (indirect.m_copyOnReg ? 1 : 0);

            auto it = std::prev(instructions.end(), static_cast<int64_t>(expectedInstrsBeforeRet + 1));

            // Verify STORE instructions writing payload into sretPtr
            EXPECT_FALSE(func->getParameters().empty()) << "Indirect return expects implicit sret parameter.";
            MirRegister *sretPtrReg = func->getParameters().front();

            for (size_t i = 0; i < storeCount; ++i, ++it)
            {
                MirInstruction *storeInstr = *it;
                MirInstructionVerifier(storeInstr).opcode(MirInstructionOpCode::STORE).operandCount(2);

                // STORE destination must be memory operand relative to sretPtrReg
                EXPECT_TRUE(storeInstr->getOperands()[0]->isOfType<MirMemory>())
                        << "STORE destination must be a memory operand.";
                auto *memOp = storeInstr->getOperands()[0]->get<MirMemory>();
                EXPECT_EQ(memOp->getBase()->getRegId(), sretPtrReg->getRegId());

                // STORE value payload
                EXPECT_EQ(storeInstr->getOperands()[1], origValues[i]);
            }

            // Verify optional copyOnReg MOV (e.g. copying sretPtrReg into %rax)
            if (indirect.m_copyOnReg)
            {
                MirInstruction *copyMovInstr = *it;
                MirInstructionVerifier(copyMovInstr).opcode(MirInstructionOpCode::MOV).operandCount(2);

                MirRegister *destReg = copyMovInstr->getOperands()[0]->get<MirRegister>();
                RegisterRef targetPhysReg = indirect.m_pointerStorage;

                EXPECT_FALSE(destReg->isVirtual());
                EXPECT_EQ(destReg->getRef(), targetPhysReg);
                EXPECT_EQ(copyMovInstr->getOperands()[1], sretPtrReg);
            }
            break;
        }

        case ArgLocationType::Stack:
        {
            size_t stackStores = origValues.size();
            auto it = std::prev(instructions.end(), static_cast<int64_t>(stackStores + 1));

            for (size_t i = 0; i < stackStores; ++i, ++it)
            {
                MirInstruction *storeInstr = *it;
                MirInstructionVerifier(storeInstr).opcode(MirInstructionOpCode::STORE).operandCount(2);

                // Verify target is an abstract stack reference frame object
                EXPECT_TRUE(storeInstr->getOperands()[0]->isOfType<MirReference>())
                        << "Stack return STORE destination must be a stack frame reference.";
                EXPECT_EQ(storeInstr->getOperands()[1], origValues[i]);
            }
            break;
        }

        default:
            EXPECT_TRUE(false) << "Unhandled ArgLocationType during verification.";
    }

    return *this;
}