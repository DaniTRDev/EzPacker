#include "EzCodeEmitterTestSuite.h"
#include "X86_64/X86_64Encoding.h"
#include "X86_64/X86_64Registers.h"

using namespace EzCodeEmitter::X86_64;

TEST_F(EzCodeEmitterTestSuite, TestRexPrefixEncoding)
{
    RexPrefix rexDefault;
    EXPECT_FALSE(rexDefault.isNeeded());

    RexPrefix rexW{ .w = true };
    EXPECT_TRUE(rexW.isNeeded());
    EXPECT_EQ(rexW.encode(), 0x48);

    RexPrefix rexR{ .r = true };
    EXPECT_TRUE(rexR.isNeeded());
    EXPECT_EQ(rexR.encode(), 0x44);

    RexPrefix rexX{ .x = true };
    EXPECT_TRUE(rexX.isNeeded());
    EXPECT_EQ(rexX.encode(), 0x42);

    RexPrefix rexB{ .b = true };
    EXPECT_TRUE(rexB.isNeeded());
    EXPECT_EQ(rexB.encode(), 0x41);

    RexPrefix rexAll{ .w = true, .r = true, .x = true, .b = true };
    EXPECT_TRUE(rexAll.isNeeded());
    EXPECT_EQ(rexAll.encode(), 0x4F);
}

TEST_F(EzCodeEmitterTestSuite, TestModRMSIBEncoding)
{
    // Reg-to-reg: mod=3 (11b), reg=0 (RAX), rm=3 (RBX) -> 11 000 011 = 0xC3
    EXPECT_EQ(InstructionEncoder::encodeModRM(3, 0, 3), 0xC3);

    // SIB: scale=2 (4x), index=1 (RCX), base=3 (RBX) -> 10 001 011 = 0x8B
    EXPECT_EQ(InstructionEncoder::encodeSIB(2, 1, 3), 0x8B);

    // Scale 1x, index none (4), base RSP (4) -> 00 100 100 = 0x24
    EXPECT_EQ(InstructionEncoder::encodeSIB(0, 4, 4), 0x24);
}

TEST_F(EzCodeEmitterTestSuite, TestMovInstructionEncoding)
{
    std::vector<uint8_t> out;

    // 1. MOV RAX, RBX -> 48 89 D8
    out.clear();
    InstructionEncoder::emitMovRR(out, Reg::RAX, Reg::RBX, 8);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x89);
    EXPECT_EQ(out[2], 0xD8);

    // 2. MOV EAX, EBX (32-bit) -> 89 D8
    out.clear();
    InstructionEncoder::emitMovRR(out, Reg::RAX, Reg::RBX, 4);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 0x89);
    EXPECT_EQ(out[1], 0xD8);

    // 3. MOV R8, R9 -> 4D 89 C8
    out.clear();
    InstructionEncoder::emitMovRR(out, Reg::R8, Reg::R9, 8);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 0x4D);
    EXPECT_EQ(out[1], 0x89);
    EXPECT_EQ(out[2], 0xC8);

    // 4. MOV RAX, 42 (imm32) -> 48 C7 C0 2A 00 00 00
    out.clear();
    InstructionEncoder::emitMovRI(out, Reg::RAX, 42, 8);
    ASSERT_EQ(out.size(), 7u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0xC7);
    EXPECT_EQ(out[2], 0xC0);
    EXPECT_EQ(out[3], 42);
    EXPECT_EQ(out[4], 0);
    EXPECT_EQ(out[5], 0);
    EXPECT_EQ(out[6], 0);

    // 5. MOV RAX, 0x1122334455667788 (imm64 / MOVABS) -> 48 B8 88 77 66 55 44 33 22 11
    out.clear();
    InstructionEncoder::emitMovRI(out, Reg::RAX, 0x1122334455667788LL, 8);
    ASSERT_EQ(out.size(), 10u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0xB8);
    EXPECT_EQ(out[2], 0x88);
    EXPECT_EQ(out[3], 0x77);
    EXPECT_EQ(out[4], 0x66);
    EXPECT_EQ(out[5], 0x55);
    EXPECT_EQ(out[6], 0x44);
    EXPECT_EQ(out[7], 0x33);
    EXPECT_EQ(out[8], 0x22);
    EXPECT_EQ(out[9], 0x11);

    // 6. MOV [RBP - 8], RAX -> 48 89 45 F8
    out.clear();
    InstructionEncoder::emitMovMR(out, MemoryOperand::BaseDisp(Reg::RBP, -8), Reg::RAX, 8);
    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x89);
    EXPECT_EQ(out[2], 0x45);
    EXPECT_EQ(out[3], static_cast<uint8_t>(-8));

    // 7. MOV RAX, [RBP - 8] -> 48 8B 45 F8
    out.clear();
    InstructionEncoder::emitMovRM(out, Reg::RAX, MemoryOperand::BaseDisp(Reg::RBP, -8), 8);
    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x8B);
    EXPECT_EQ(out[2], 0x45);
    EXPECT_EQ(out[3], static_cast<uint8_t>(-8));

    // 8. MOV RAX, [RSP + 16] (RSP requires SIB 0x24) -> 48 8B 44 24 10
    out.clear();
    InstructionEncoder::emitMovRM(out, Reg::RAX, MemoryOperand::BaseDisp(Reg::RSP, 16), 8);
    ASSERT_EQ(out.size(), 5u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x8B);
    EXPECT_EQ(out[2], 0x44);
    EXPECT_EQ(out[3], 0x24);
    EXPECT_EQ(out[4], 16);

    // 9. MOV RAX, [RBX + RCX * 4 + 32] (Complex SIB) -> 48 8B 44 8B 20
    out.clear();
    InstructionEncoder::emitMovRM(out, Reg::RAX, MemoryOperand::BaseIndex(Reg::RBX, Reg::RCX, 4, 32), 8);
    ASSERT_EQ(out.size(), 5u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x8B);
    EXPECT_EQ(out[2], 0x44);
    EXPECT_EQ(out[3], 0x8B);
    EXPECT_EQ(out[4], 32);
}

TEST_F(EzCodeEmitterTestSuite, TestAluInstructionEncoding)
{
    std::vector<uint8_t> out;

    // 1. ADD RAX, RBX -> 48 01 D8
    out.clear();
    InstructionEncoder::emitAluRR(out, AluOp::ADD, Reg::RAX, Reg::RBX, 8);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x01);
    EXPECT_EQ(out[2], 0xD8);

    // 2. ADD RAX, 1 (imm8 sign extended) -> 48 83 C0 01
    out.clear();
    InstructionEncoder::emitAluRI(out, AluOp::ADD, Reg::RAX, 1, 8);
    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x83);
    EXPECT_EQ(out[2], 0xC0);
    EXPECT_EQ(out[3], 1);

    // 3. ADD RAX, 1000 (imm32) -> 48 81 C0 E8 03 00 00
    out.clear();
    InstructionEncoder::emitAluRI(out, AluOp::ADD, Reg::RAX, 1000, 8);
    ASSERT_EQ(out.size(), 7u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x81);
    EXPECT_EQ(out[2], 0xC0);
    EXPECT_EQ(out[3], 0xE8);
    EXPECT_EQ(out[4], 0x03);
    EXPECT_EQ(out[5], 0x00);
    EXPECT_EQ(out[6], 0x00);

    // 4. SUB RAX, RBX -> 48 29 D8
    out.clear();
    InstructionEncoder::emitAluRR(out, AluOp::SUB, Reg::RAX, Reg::RBX, 8);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x29);
    EXPECT_EQ(out[2], 0xD8);

    // 5. SUB RSP, 32 -> 48 83 EC 20
    out.clear();
    InstructionEncoder::emitAluRI(out, AluOp::SUB, Reg::RSP, 32, 8);
    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x83);
    EXPECT_EQ(out[2], 0xEC);
    EXPECT_EQ(out[3], 32);

    // 6. XOR RAX, RAX -> 48 31 C0
    out.clear();
    InstructionEncoder::emitAluRR(out, AluOp::XOR, Reg::RAX, Reg::RAX, 8);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x31);
    EXPECT_EQ(out[2], 0xC0);

    // 7. CMP RAX, 0 -> 48 83 F8 00
    out.clear();
    InstructionEncoder::emitAluRI(out, AluOp::CMP, Reg::RAX, 0, 8);
    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x83);
    EXPECT_EQ(out[2], 0xF8);
    EXPECT_EQ(out[3], 0);
}

TEST_F(EzCodeEmitterTestSuite, TestStackAndControlFlowEncoding)
{
    std::vector<uint8_t> out;

    // PUSH RBP -> 55
    out.clear();
    InstructionEncoder::emitPushR(out, Reg::RBP);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 0x55);

    // PUSH R12 -> 41 54
    out.clear();
    InstructionEncoder::emitPushR(out, Reg::R12);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 0x41);
    EXPECT_EQ(out[1], 0x54);

    // POP RBP -> 5D
    out.clear();
    InstructionEncoder::emitPopR(out, Reg::RBP);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 0x5D);

    // POP R12 -> 41 5C
    out.clear();
    InstructionEncoder::emitPopR(out, Reg::R12);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 0x41);
    EXPECT_EQ(out[1], 0x5C);

    // RET -> C3
    out.clear();
    InstructionEncoder::emitRet(out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 0xC3);

    // NOP -> 90
    out.clear();
    InstructionEncoder::emitNop(out, 1);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0], 0x90);

    // LEA RAX, [RBP - 16] -> 48 8D 45 F0
    out.clear();
    InstructionEncoder::emitLea(out, Reg::RAX, MemoryOperand::BaseDisp(Reg::RBP, -16), 8);
    ASSERT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x8D);
    EXPECT_EQ(out[2], 0x45);
    EXPECT_EQ(out[3], static_cast<uint8_t>(-16));

    // TEST RAX, RAX -> 48 85 C0
    out.clear();
    InstructionEncoder::emitTestRR(out, Reg::RAX, Reg::RAX, 8);
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 0x48);
    EXPECT_EQ(out[1], 0x85);
    EXPECT_EQ(out[2], 0xC0);

    // JMP short 10 -> EB 0A
    out.clear();
    InstructionEncoder::emitJmpShort(out, 10);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 0xEB);
    EXPECT_EQ(out[1], 10);

    // JMP near 0x12345678 -> E9 78 56 34 12
    out.clear();
    InstructionEncoder::emitJmpNear(out, 0x12345678);
    ASSERT_EQ(out.size(), 5u);
    EXPECT_EQ(out[0], 0xE9);
    EXPECT_EQ(out[1], 0x78);
    EXPECT_EQ(out[2], 0x56);
    EXPECT_EQ(out[3], 0x34);
    EXPECT_EQ(out[4], 0x12);

    // JE short 5 -> 74 05
    out.clear();
    InstructionEncoder::emitJccShort(out, ConditionCode::E, 5);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0], 0x74);
    EXPECT_EQ(out[1], 5);

    // JNE near 0x1000 -> 0F 85 00 10 00 00
    out.clear();
    InstructionEncoder::emitJccNear(out, ConditionCode::NE, 0x1000);
    ASSERT_EQ(out.size(), 6u);
    EXPECT_EQ(out[0], 0x0F);
    EXPECT_EQ(out[1], 0x85);
    EXPECT_EQ(out[2], 0x00);
    EXPECT_EQ(out[3], 0x10);
    EXPECT_EQ(out[4], 0x00);
    EXPECT_EQ(out[5], 0x00);

    // CALL near 0x20 -> E8 20 00 00 00
    out.clear();
    InstructionEncoder::emitCallNear(out, 0x20);
    ASSERT_EQ(out.size(), 5u);
    EXPECT_EQ(out[0], 0xE8);
    EXPECT_EQ(out[1], 0x20);
    EXPECT_EQ(out[2], 0x00);
    EXPECT_EQ(out[3], 0x00);
    EXPECT_EQ(out[4], 0x00);
}
