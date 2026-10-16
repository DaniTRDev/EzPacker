#include "EzCodeEmitterTestSuite.h"
#include "Encoding/RiscV64InstructionEncoder.h"
#include "Encoding/RiscV64EncodingDesc.h"
#include "riscv64EncodingTable.h"

using namespace EzTargets::RiscV64;

namespace
{

uint32_t readWord(const std::vector<uint8_t> &bytes, size_t offset)
{
    return static_cast<uint32_t>(bytes[offset]) |
           (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

} // namespace

// Verifies encoding of R-type instruction (e.g. ADD64rr).
TEST_F(EzCodeEmitterTestSuite, TestRiscV64EncodeRType)
{
    const EncodingDesc *addDesc = findEncodingDesc("ADD64rr");
    ASSERT_NE(addDesc, nullptr);
    EXPECT_EQ(addDesc->m_form, EncForm::RType);
    EXPECT_EQ(addDesc->m_opcode, 0x33u);
    EXPECT_EQ(addDesc->m_funct3, 0u);
    EXPECT_EQ(addDesc->m_funct7, 0x00u);

    // Encode ADD64rr rd=x10 (a0), rs1=x11 (a1), rs2=x12 (a2)
    ResolvedOperand opRd{};
    opRd.m_kind = ResolvedOperand::Kind::Register;
    opRd.m_reg = 10;

    ResolvedOperand opRs1{};
    opRs1.m_kind = ResolvedOperand::Kind::Register;
    opRs1.m_reg = 11;

    ResolvedOperand opRs2{};
    opRs2.m_kind = ResolvedOperand::Kind::Register;
    opRs2.m_reg = 12;

    std::vector<ResolvedOperand> operands = { opRd, opRs1, opRs2 };

    std::vector<uint8_t> out;
    EzCodeEmitter::EncodeResult result;
    ASSERT_TRUE(InstructionEncoder::encode(*addDesc, operands, out, result));
    ASSERT_EQ(out.size(), 4u);

    uint32_t word = readWord(out, 0);
    EXPECT_EQ(word & 0x7F, 0x33u);               // Opcode
    EXPECT_EQ((word >> 7) & 0x1F, 10u);          // rd = a0 (x10)
    EXPECT_EQ((word >> 12) & 0x7, 0u);           // funct3 = 0
    EXPECT_EQ((word >> 15) & 0x1F, 11u);         // rs1 = a1 (x11)
    EXPECT_EQ((word >> 20) & 0x1F, 12u);         // rs2 = a2 (x12)
    EXPECT_EQ((word >> 25) & 0x7F, 0x00u);       // funct7 = 0x00
}

// Verifies encoding of I-type instruction (e.g. ADDI64ri).
TEST_F(EzCodeEmitterTestSuite, TestRiscV64EncodeIType)
{
    const EncodingDesc *addiDesc = findEncodingDesc("ADDI64ri");
    ASSERT_NE(addiDesc, nullptr);
    EXPECT_EQ(addiDesc->m_form, EncForm::IType);
    EXPECT_EQ(addiDesc->m_opcode, 0x13u);

    // Encode ADDI64ri rd=x10, rs1=x0, imm=42
    ResolvedOperand opRd{};
    opRd.m_kind = ResolvedOperand::Kind::Register;
    opRd.m_reg = 10;

    ResolvedOperand opRs1{};
    opRs1.m_kind = ResolvedOperand::Kind::Register;
    opRs1.m_reg = 0;

    ResolvedOperand opImm{};
    opImm.m_kind = ResolvedOperand::Kind::Immediate;
    opImm.m_imm = 42;

    std::vector<ResolvedOperand> operands = { opRd, opRs1, opImm };

    std::vector<uint8_t> out;
    EzCodeEmitter::EncodeResult result;
    ASSERT_TRUE(InstructionEncoder::encode(*addiDesc, operands, out, result));
    ASSERT_EQ(out.size(), 4u);

    uint32_t word = readWord(out, 0);
    EXPECT_EQ(word & 0x7F, 0x13u);
    EXPECT_EQ((word >> 7) & 0x1F, 10u);
    EXPECT_EQ((word >> 12) & 0x7, 0u);
    EXPECT_EQ((word >> 15) & 0x1F, 0u);
    EXPECT_EQ((word >> 20) & 0xFFF, 42u);
}

// Verifies encoding of S-type instruction (e.g. SD).
TEST_F(EzCodeEmitterTestSuite, TestRiscV64EncodeSType)
{
    const EncodingDesc *sdDesc = findEncodingDesc("SD");
    ASSERT_NE(sdDesc, nullptr);
    EXPECT_EQ(sdDesc->m_form, EncForm::SType);
    EXPECT_EQ(sdDesc->m_opcode, 0x23u);

    // Encode SD rs2=x1 (ra), base=x2 (sp), disp=24
    ResolvedOperand opRs2{};
    opRs2.m_kind = ResolvedOperand::Kind::Register;
    opRs2.m_reg = 1;

    ResolvedOperand opMem{};
    opMem.m_kind = ResolvedOperand::Kind::Memory;
    opMem.m_mem.m_base = 2; // sp
    opMem.m_mem.m_disp = 24;

    std::vector<ResolvedOperand> operands = { opRs2, opMem };

    std::vector<uint8_t> out;
    EzCodeEmitter::EncodeResult result;
    ASSERT_TRUE(InstructionEncoder::encode(*sdDesc, operands, out, result));
    ASSERT_EQ(out.size(), 4u);

    uint32_t word = readWord(out, 0);
    EXPECT_EQ(word & 0x7F, 0x23u);
    EXPECT_EQ((word >> 12) & 0x7, 3u);           // funct3 = 3 for 64-bit store
    EXPECT_EQ((word >> 15) & 0x1F, 2u);          // rs1 = sp
    EXPECT_EQ((word >> 20) & 0x1F, 1u);          // rs2 = ra
    EXPECT_EQ((word >> 7) & 0x1F, 24u);          // imm[4:0]
}

// Verifies emitBranch and patchBranchDisp methods.
TEST_F(EzCodeEmitterTestSuite, TestRiscV64EmitAndPatchBranch)
{
    std::vector<uint8_t> out;
    // Emit BEQ: funct3=0, rs1=10, rs2=11, offset=16
    InstructionEncoder::emitBranch(out, 0, 10, 11, 16);
    ASSERT_EQ(out.size(), 4u);

    bool isJal = false, isBranch = false;
    uint8_t funct3 = 0, rs1 = 0, rs2 = 0, rd = 0;
    EXPECT_TRUE(InstructionEncoder::classifyBranch(out, 0, isJal, isBranch, funct3, rs1, rs2, rd));
    EXPECT_TRUE(isBranch);
    EXPECT_FALSE(isJal);
    EXPECT_EQ(funct3, 0u);
    EXPECT_EQ(rs1, 10u);
    EXPECT_EQ(rs2, 11u);

    // Patch displacement to -32
    EXPECT_TRUE(InstructionEncoder::patchBranchDisp(out, 0, -32));
    uint32_t patched = readWord(out, 0);
    EXPECT_EQ(patched & 0x7F, 0x63u);
}

// Verifies emitJal and patchJalDisp methods.
TEST_F(EzCodeEmitterTestSuite, TestRiscV64EmitAndPatchJal)
{
    std::vector<uint8_t> out;
    // Emit JAL: rd=1 (ra), offset=0x100
    InstructionEncoder::emitJal(out, 1, 0x100);
    ASSERT_EQ(out.size(), 4u);

    bool isJal = false, isBranch = false;
    uint8_t funct3 = 0, rs1 = 0, rs2 = 0, rd = 0;
    EXPECT_TRUE(InstructionEncoder::classifyBranch(out, 0, isJal, isBranch, funct3, rs1, rs2, rd));
    EXPECT_TRUE(isJal);
    EXPECT_FALSE(isBranch);
    EXPECT_EQ(rd, 1u);

    // Patch displacement to 0x200
    EXPECT_TRUE(InstructionEncoder::patchJalDisp(out, 0, 0x200));
    uint32_t patched = readWord(out, 0);
    EXPECT_EQ(patched & 0x7F, 0x6Fu);
    EXPECT_EQ((patched >> 7) & 0x1F, 1u);
}

// Verifies register move synthetic helper.
TEST_F(EzCodeEmitterTestSuite, TestRiscV64RegisterMove)
{
    ResolvedOperand dst{};
    dst.m_kind = ResolvedOperand::Kind::Register;
    dst.m_reg = 10;

    ResolvedOperand src{};
    src.m_kind = ResolvedOperand::Kind::Register;
    src.m_reg = 11;

    std::vector<uint8_t> out;
    InstructionEncoder::encodeRegisterMove(dst, src, out);
    ASSERT_EQ(out.size(), 4u);

    uint32_t word = readWord(out, 0);
    // Move is ADDI rd, rs, 0 -> opcode 0x13
    EXPECT_EQ(word & 0x7F, 0x13u);
    EXPECT_EQ((word >> 7) & 0x1F, 10u);
    EXPECT_EQ((word >> 15) & 0x1F, 11u);
    EXPECT_EQ((word >> 20) & 0xFFF, 0u);
}
