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

// Verifies encoding of scalar floating-point instructions (FADD_S, FADD_D, FEQ_S, etc.).
TEST_F(EzCodeEmitterTestSuite, TestRiscV64EncodeFloat)
{
    // FADD_S fa0 (f10), fa1 (f11), fa2 (f12)
    const EncodingDesc *faddSDesc = findEncodingDesc("FADD_S");
    ASSERT_NE(faddSDesc, nullptr);
    EXPECT_EQ(faddSDesc->m_opcode, 0x53u);
    EXPECT_EQ(faddSDesc->m_funct7, 0x00u);

    ResolvedOperand opFa0{};
    opFa0.m_kind = ResolvedOperand::Kind::Register;
    opFa0.m_isFpr = true;
    opFa0.m_sizeBytes = 4;
    opFa0.m_reg = 10;

    ResolvedOperand opFa1{};
    opFa1.m_kind = ResolvedOperand::Kind::Register;
    opFa1.m_isFpr = true;
    opFa1.m_sizeBytes = 4;
    opFa1.m_reg = 11;

    ResolvedOperand opFa2{};
    opFa2.m_kind = ResolvedOperand::Kind::Register;
    opFa2.m_isFpr = true;
    opFa2.m_sizeBytes = 4;
    opFa2.m_reg = 12;

    std::vector<ResolvedOperand> opsS = { opFa0, opFa1, opFa2 };
    std::vector<uint8_t> outS;
    EzCodeEmitter::EncodeResult resS;
    ASSERT_TRUE(InstructionEncoder::encode(*faddSDesc, opsS, outS, resS));
    ASSERT_EQ(outS.size(), 4u);

    uint32_t wordS = readWord(outS, 0);
    EXPECT_EQ(wordS & 0x7F, 0x53u);
    EXPECT_EQ((wordS >> 7) & 0x1F, 10u);
    EXPECT_EQ((wordS >> 12) & 0x7, 0x7u); // rounding mode
    EXPECT_EQ((wordS >> 15) & 0x1F, 11u);
    EXPECT_EQ((wordS >> 20) & 0x1F, 12u);
    EXPECT_EQ((wordS >> 25) & 0x7F, 0x00u);

    // FADD_D fa0 (f10), fa1 (f11), fa2 (f12)
    const EncodingDesc *faddDDesc = findEncodingDesc("FADD_D");
    ASSERT_NE(faddDDesc, nullptr);
    EXPECT_EQ(faddDDesc->m_opcode, 0x53u);
    EXPECT_EQ(faddDDesc->m_funct7, 0x01u);

    std::vector<uint8_t> outD;
    EzCodeEmitter::EncodeResult resD;
    ASSERT_TRUE(InstructionEncoder::encode(*faddDDesc, opsS, outD, resD));
    ASSERT_EQ(outD.size(), 4u);

    uint32_t wordD = readWord(outD, 0);
    EXPECT_EQ(wordD & 0x7F, 0x53u);
    EXPECT_EQ((wordD >> 7) & 0x1F, 10u);
    EXPECT_EQ((wordD >> 25) & 0x7F, 0x01u);

    // FEQ_S a0 (x10), fa1 (f11), fa2 (f12)
    const EncodingDesc *feqDesc = findEncodingDesc("FEQ_S");
    ASSERT_NE(feqDesc, nullptr);
    EXPECT_EQ(feqDesc->m_opcode, 0x53u);
    EXPECT_EQ(feqDesc->m_funct7, 0x50u);
    EXPECT_EQ(feqDesc->m_funct3, 0x2u);

    ResolvedOperand opA0{};
    opA0.m_kind = ResolvedOperand::Kind::Register;
    opA0.m_reg = 10;
    std::vector<ResolvedOperand> opsFeq = { opA0, opFa1, opFa2 };
    std::vector<uint8_t> outFeq;
    EzCodeEmitter::EncodeResult resFeq;
    ASSERT_TRUE(InstructionEncoder::encode(*feqDesc, opsFeq, outFeq, resFeq));
    ASSERT_EQ(outFeq.size(), 4u);
    uint32_t wordFeq = readWord(outFeq, 0);
    EXPECT_EQ(wordFeq & 0x7F, 0x53u);
    EXPECT_EQ((wordFeq >> 7) & 0x1F, 10u);
    EXPECT_EQ((wordFeq >> 12) & 0x7, 0x2u);
    EXPECT_EQ((wordFeq >> 25) & 0x7F, 0x50u);
}

// Verifies encoding of vector instructions (VADD_VV, VFADD_VV, VMV_VV, VLE32_V, VSE32_V).
TEST_F(EzCodeEmitterTestSuite, TestRiscV64EncodeVector)
{
    // VADD_VV vd=v1, vs1=v2, vs2=v3
    const EncodingDesc *vaddDesc = findEncodingDesc("VADD_VV");
    ASSERT_NE(vaddDesc, nullptr);
    EXPECT_EQ(vaddDesc->m_opcode, 0x57u);
    EXPECT_EQ(vaddDesc->m_funct3, 0x0u);
    EXPECT_EQ(vaddDesc->m_funct7, 0x01u);

    ResolvedOperand opV1{};
    opV1.m_kind = ResolvedOperand::Kind::Register;
    opV1.m_isVr = true;
    opV1.m_reg = 1;

    ResolvedOperand opV2{};
    opV2.m_kind = ResolvedOperand::Kind::Register;
    opV2.m_isVr = true;
    opV2.m_reg = 2;

    ResolvedOperand opV3{};
    opV3.m_kind = ResolvedOperand::Kind::Register;
    opV3.m_isVr = true;
    opV3.m_reg = 3;

    std::vector<ResolvedOperand> opsVadd = { opV1, opV2, opV3 };
    std::vector<uint8_t> outVadd;
    EzCodeEmitter::EncodeResult resVadd;
    ASSERT_TRUE(InstructionEncoder::encode(*vaddDesc, opsVadd, outVadd, resVadd));
    ASSERT_EQ(outVadd.size(), 4u);

    uint32_t wordVadd = readWord(outVadd, 0);
    EXPECT_EQ(wordVadd & 0x7F, 0x57u);
    EXPECT_EQ((wordVadd >> 7) & 0x1F, 1u);   // vd = v1
    EXPECT_EQ((wordVadd >> 12) & 0x7, 0x0u); // funct3 = OPIVV
    EXPECT_EQ((wordVadd >> 15) & 0x1F, 2u);  // vs1 = v2
    EXPECT_EQ((wordVadd >> 20) & 0x1F, 3u);  // vs2 = v3
    EXPECT_EQ((wordVadd >> 25) & 0x7F, 0x01u);

    // VFADD_VV vd=v1, vs1=v2, vs2=v3
    const EncodingDesc *vfaddDesc = findEncodingDesc("VFADD_VV");
    ASSERT_NE(vfaddDesc, nullptr);
    EXPECT_EQ(vfaddDesc->m_opcode, 0x57u);
    EXPECT_EQ(vfaddDesc->m_funct3, 0x1u); // OPFVV

    std::vector<uint8_t> outVfadd;
    EzCodeEmitter::EncodeResult resVfadd;
    ASSERT_TRUE(InstructionEncoder::encode(*vfaddDesc, opsVadd, outVfadd, resVfadd));
    uint32_t wordVfadd = readWord(outVfadd, 0);
    EXPECT_EQ(wordVfadd & 0x7F, 0x57u);
    EXPECT_EQ((wordVfadd >> 12) & 0x7, 0x1u);

    // VMV_VV vd=v1, vs=v2
    const EncodingDesc *vmvDesc = findEncodingDesc("VMV_VV");
    ASSERT_NE(vmvDesc, nullptr);
    EXPECT_EQ(vmvDesc->m_opcode, 0x57u);
    EXPECT_EQ(vmvDesc->m_funct7, 0x2Fu);

    std::vector<ResolvedOperand> opsVmv = { opV1, opV2 };
    std::vector<uint8_t> outVmv;
    EzCodeEmitter::EncodeResult resVmv;
    ASSERT_TRUE(InstructionEncoder::encode(*vmvDesc, opsVmv, outVmv, resVmv));
    uint32_t wordVmv = readWord(outVmv, 0);
    EXPECT_EQ(wordVmv & 0x7F, 0x57u);
    EXPECT_EQ((wordVmv >> 7) & 0x1F, 1u);
    EXPECT_EQ((wordVmv >> 15) & 0x1F, 2u);
    EXPECT_EQ((wordVmv >> 25) & 0x7F, 0x2Fu);

    // VLE32_V vd=v1, base=x10 (a0)
    const EncodingDesc *vleDesc = findEncodingDesc("VLE32_V");
    ASSERT_NE(vleDesc, nullptr);
    EXPECT_EQ(vleDesc->m_opcode, 0x07u);
    EXPECT_EQ(vleDesc->m_funct3, 0x6u); // e32
    EXPECT_EQ(vleDesc->m_funct7, 0x01u); // unmasked

    ResolvedOperand opA0{};
    opA0.m_kind = ResolvedOperand::Kind::Register;
    opA0.m_reg = 10;
    std::vector<ResolvedOperand> opsVle = { opV1, opA0 };
    std::vector<uint8_t> outVle;
    EzCodeEmitter::EncodeResult resVle;
    ASSERT_TRUE(InstructionEncoder::encode(*vleDesc, opsVle, outVle, resVle));
    uint32_t wordVle = readWord(outVle, 0);
    EXPECT_EQ(wordVle & 0x7F, 0x07u);
    EXPECT_EQ((wordVle >> 7) & 0x1F, 1u);
    EXPECT_EQ((wordVle >> 12) & 0x7, 0x6u);
    EXPECT_EQ((wordVle >> 15) & 0x1F, 10u);
    EXPECT_EQ((wordVle >> 25) & 0x7F, 0x01u);

    // VSE32_V vs=v1, base=x10 (a0)
    const EncodingDesc *vseDesc = findEncodingDesc("VSE32_V");
    ASSERT_NE(vseDesc, nullptr);
    EXPECT_EQ(vseDesc->m_opcode, 0x27u);
    EXPECT_EQ(vseDesc->m_funct3, 0x6u); // e32
    EXPECT_EQ(vseDesc->m_funct7, 0x01u); // unmasked

    std::vector<ResolvedOperand> opsVse = { opV1, opA0 };
    std::vector<uint8_t> outVse;
    EzCodeEmitter::EncodeResult resVse;
    ASSERT_TRUE(InstructionEncoder::encode(*vseDesc, opsVse, outVse, resVse));
    uint32_t wordVse = readWord(outVse, 0);
    EXPECT_EQ(wordVse & 0x7F, 0x27u);
    EXPECT_EQ((wordVse >> 7) & 0x1F, 1u);
    EXPECT_EQ((wordVse >> 12) & 0x7, 0x6u);
    EXPECT_EQ((wordVse >> 15) & 0x1F, 10u);
    EXPECT_EQ((wordVse >> 25) & 0x7F, 0x01u);
}

// Verifies register move helpers for vector and floating-point registers.
TEST_F(EzCodeEmitterTestSuite, TestRiscV64RegisterMoveFloatAndVector)
{
    // VR Move -> vmv.v.v vd, vs1 (opcode 0x57, funct7 0x2F)
    ResolvedOperand dstVr{};
    dstVr.m_kind = ResolvedOperand::Kind::Register;
    dstVr.m_isVr = true;
    dstVr.m_reg = 5;

    ResolvedOperand srcVr{};
    srcVr.m_kind = ResolvedOperand::Kind::Register;
    srcVr.m_isVr = true;
    srcVr.m_reg = 6;

    std::vector<uint8_t> outVr;
    InstructionEncoder::encodeRegisterMove(dstVr, srcVr, outVr);
    ASSERT_EQ(outVr.size(), 4u);
    uint32_t wordVr = readWord(outVr, 0);
    EXPECT_EQ(wordVr & 0x7F, 0x57u);
    EXPECT_EQ((wordVr >> 7) & 0x1F, 5u);
    EXPECT_EQ((wordVr >> 15) & 0x1F, 6u);
    EXPECT_EQ((wordVr >> 25) & 0x7F, 0x2Fu);

    // FPR32 Move -> fsgnj.s (opcode 0x53, funct7 0x10)
    ResolvedOperand dstF32{};
    dstF32.m_kind = ResolvedOperand::Kind::Register;
    dstF32.m_isFpr = true;
    dstF32.m_sizeBytes = 4;
    dstF32.m_reg = 10;

    ResolvedOperand srcF32{};
    srcF32.m_kind = ResolvedOperand::Kind::Register;
    srcF32.m_isFpr = true;
    srcF32.m_sizeBytes = 4;
    srcF32.m_reg = 11;

    std::vector<uint8_t> outF32;
    InstructionEncoder::encodeRegisterMove(dstF32, srcF32, outF32);
    ASSERT_EQ(outF32.size(), 4u);
    uint32_t wordF32 = readWord(outF32, 0);
    EXPECT_EQ(wordF32 & 0x7F, 0x53u);
    EXPECT_EQ((wordF32 >> 7) & 0x1F, 10u);
    EXPECT_EQ((wordF32 >> 15) & 0x1F, 11u);
    EXPECT_EQ((wordF32 >> 25) & 0x7F, 0x10u);

    // FPR64 Move -> fsgnj.d (opcode 0x53, funct7 0x11)
    ResolvedOperand dstF64{};
    dstF64.m_kind = ResolvedOperand::Kind::Register;
    dstF64.m_isFpr = true;
    dstF64.m_sizeBytes = 8;
    dstF64.m_reg = 10;

    ResolvedOperand srcF64{};
    srcF64.m_kind = ResolvedOperand::Kind::Register;
    srcF64.m_isFpr = true;
    srcF64.m_sizeBytes = 8;
    srcF64.m_reg = 11;

    std::vector<uint8_t> outF64;
    InstructionEncoder::encodeRegisterMove(dstF64, srcF64, outF64);
    ASSERT_EQ(outF64.size(), 4u);
    uint32_t wordF64 = readWord(outF64, 0);
    EXPECT_EQ(wordF64 & 0x7F, 0x53u);
    EXPECT_EQ((wordF64 >> 7) & 0x1F, 10u);
    EXPECT_EQ((wordF64 >> 15) & 0x1F, 11u);
    EXPECT_EQ((wordF64 >> 25) & 0x7F, 0x11u);
}
