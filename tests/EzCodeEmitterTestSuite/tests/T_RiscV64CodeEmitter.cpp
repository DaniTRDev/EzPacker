#include "EzCodeEmitterTestSuite.h"
#include "Helpers.h"
#include "CodeEmitter/RiscV64CodeEmitter.h"
#include "Encoding/RiscV64EncodingDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "riscv64EncodingTable.h"

using namespace EzCodeEmitter;
using namespace EzTargets::RiscV64;

// Emits ADDI / ADD / RET through RiscV64CodeEmitter and verifies the emitted 32-bit instruction words.
TEST_F(EzCodeEmitterTestSuite, TestRiscV64CodeEmitterIntegration)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeEmitterContext context(getDiagCollector(), sections, getAllocator());

    RiscV64CodeEmitter emitter;
    emitter.setEncodingResolver([](const MirTargetInstructionDesc *desc) -> const EncodingDesc *
                                { return findEncodingDesc(desc->getName()); });
    emitter.beginFunction(&context, "main");

    MirOperandBuilder opBuilder(getBuilderCtx());
    auto *i64 = getTypeTable()->i64();

    // Registers: a0 (x10), a1 (x11), zero (x0)
    MirRegister *vregA0 = opBuilder.buildVReg(i64);
    vregA0->setRef(MirRegisterRef(10, /*isVirtual=*/false));

    MirRegister *vregA1 = opBuilder.buildVReg(i64);
    vregA1->setRef(MirRegisterRef(11, /*isVirtual=*/false));

    MirRegister *vregZero = opBuilder.buildVReg(i64);
    vregZero->setRef(MirRegisterRef(0, /*isVirtual=*/false));

    // 1. ADDI64ri: a0 = zero + 42
    MirTargetInstructionDesc descADDI("ADDI64ri", 100);
    MirInteger *imm42 = opBuilder.buildInt(i64, FlexInt(42, 64));
    MirOperand *opsAddi1[] = { vregA0, vregZero, imm42 };
    emitter.emitInst(&descADDI, opsAddi1);

    // 2. ADDI64ri: a1 = zero + 10
    MirInteger *imm10 = opBuilder.buildInt(i64, FlexInt(10, 64));
    MirOperand *opsAddi2[] = { vregA1, vregZero, imm10 };
    emitter.emitInst(&descADDI, opsAddi2);

    // 3. ADD64rr: a0 = a0 + a1
    MirTargetInstructionDesc descADD("ADD64rr", 101);
    MirOperand *opsAdd[] = { vregA0, vregA0, vregA1 };
    emitter.emitInst(&descADD, opsAdd);

    // 4. RET
    MirTargetInstructionDesc descRET("RET", 102);
    emitter.emitInst(&descRET, {});

    emitter.endFunction(&context);

    CodeSection *textSec = sections[SectionType::Text];
    textSec->finalize();

    std::span<const uint8_t> code = textSec->getData();
    // 4 instructions * 4 bytes each = 16 bytes
    ASSERT_EQ(code.size(), 16u);

    // Word 0: ADDI a0, zero, 42 -> 0x02A00513
    uint32_t w0 = static_cast<uint32_t>(code[0]) | (static_cast<uint32_t>(code[1]) << 8) |
                  (static_cast<uint32_t>(code[2]) << 16) | (static_cast<uint32_t>(code[3]) << 24);
    EXPECT_EQ(w0 & 0x7F, 0x13u);              // OP-IMM
    EXPECT_EQ((w0 >> 7) & 0x1F, 10u);         // rd = a0
    EXPECT_EQ((w0 >> 15) & 0x1F, 0u);         // rs1 = zero
    EXPECT_EQ((w0 >> 20) & 0xFFF, 42u);       // imm = 42

    // Word 1: ADDI a1, zero, 10
    uint32_t w1 = static_cast<uint32_t>(code[4]) | (static_cast<uint32_t>(code[5]) << 8) |
                  (static_cast<uint32_t>(code[6]) << 16) | (static_cast<uint32_t>(code[7]) << 24);
    EXPECT_EQ(w1 & 0x7F, 0x13u);
    EXPECT_EQ((w1 >> 7) & 0x1F, 11u);         // rd = a1
    EXPECT_EQ((w1 >> 20) & 0xFFF, 10u);       // imm = 10

    // Word 2: ADD a0, a0, a1 -> 0x00B50533
    uint32_t w2 = static_cast<uint32_t>(code[8]) | (static_cast<uint32_t>(code[9]) << 8) |
                  (static_cast<uint32_t>(code[10]) << 16) | (static_cast<uint32_t>(code[11]) << 24);
    EXPECT_EQ(w2 & 0x7F, 0x33u);              // OP
    EXPECT_EQ((w2 >> 7) & 0x1F, 10u);         // rd = a0
    EXPECT_EQ((w2 >> 15) & 0x1F, 10u);        // rs1 = a0
    EXPECT_EQ((w2 >> 20) & 0x1F, 11u);        // rs2 = a1

    // Word 3: RET -> JALR x0, x1, 0 -> 0x00008067
    uint32_t w3 = static_cast<uint32_t>(code[12]) | (static_cast<uint32_t>(code[13]) << 8) |
                  (static_cast<uint32_t>(code[14]) << 16) | (static_cast<uint32_t>(code[15]) << 24);
    EXPECT_EQ(w3 & 0x7F, 0x67u);              // JALR
    EXPECT_EQ((w3 >> 7) & 0x1F, 0u);          // rd = zero
    EXPECT_EQ((w3 >> 15) & 0x1F, 1u);         // rs1 = ra (x1)
}

// Emits float and vector instructions through RiscV64CodeEmitter and verifies binary code output.
TEST_F(EzCodeEmitterTestSuite, TestRiscV64CodeEmitterFloatAndVector)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeEmitterContext context(getDiagCollector(), sections, getAllocator());

    RiscV64CodeEmitter emitter;
    emitter.setEncodingResolver([](const MirTargetInstructionDesc *desc) -> const EncodingDesc *
                                { return findEncodingDesc(desc->getName()); });
    emitter.beginFunction(&context, "vec_fn");

    MirOperandBuilder opBuilder(getBuilderCtx());
    auto *f32 = getTypeTable()->f32();
    auto *i64 = getTypeTable()->i64();
    auto *v4i32 = getTypeTable()->v4i32();

    // Floating point registers: fa0 (f10), fa1 (f11), fa2 (f12)
    MirRegister *vregFa0 = opBuilder.buildVReg(f32);
    vregFa0->setRef(MirRegisterRef(10, /*isVirtual=*/false));

    MirRegister *vregFa1 = opBuilder.buildVReg(f32);
    vregFa1->setRef(MirRegisterRef(11, /*isVirtual=*/false));

    MirRegister *vregFa2 = opBuilder.buildVReg(f32);
    vregFa2->setRef(MirRegisterRef(12, /*isVirtual=*/false));

    // Vector registers: v1, v2, v3
    MirRegister *vregV1 = opBuilder.buildVReg(v4i32);
    vregV1->setRef(MirRegisterRef(1, /*isVirtual=*/false));

    MirRegister *vregV2 = opBuilder.buildVReg(v4i32);
    vregV2->setRef(MirRegisterRef(2, /*isVirtual=*/false));

    MirRegister *vregV3 = opBuilder.buildVReg(v4i32);
    vregV3->setRef(MirRegisterRef(3, /*isVirtual=*/false));

    // GPR base register: a0 (x10)
    MirRegister *vregA0 = opBuilder.buildVReg(i64);
    vregA0->setRef(MirRegisterRef(10, /*isVirtual=*/false));

    // 1. FADD_S fa0, fa1, fa2
    MirTargetInstructionDesc descFAdd("FADD_S", 200);
    MirOperand *opsFAdd[] = { vregFa0, vregFa1, vregFa2 };
    emitter.emitInst(&descFAdd, opsFAdd);

    // 2. VADD_VV v1, v2, v3
    MirTargetInstructionDesc descVAdd("VADD_VV", 201);
    MirOperand *opsVAdd[] = { vregV1, vregV2, vregV3 };
    emitter.emitInst(&descVAdd, opsVAdd);

    // 3. VLE32_V v1, a0
    MirTargetInstructionDesc descVLe("VLE32_V", 202);
    MirOperand *opsVLe[] = { vregV1, vregA0 };
    emitter.emitInst(&descVLe, opsVLe);

    // 4. VSE32_V v1, a0
    MirTargetInstructionDesc descVSe("VSE32_V", 203);
    MirOperand *opsVSe[] = { vregV1, vregA0 };
    emitter.emitInst(&descVSe, opsVSe);

    emitter.endFunction(&context);

    CodeSection *textSec = sections[SectionType::Text];
    textSec->finalize();

    std::span<const uint8_t> code = textSec->getData();
    ASSERT_EQ(code.size(), 16u);

    // Word 0: FADD_S fa0, fa1, fa2 -> opcode 0x53
    uint32_t w0 = static_cast<uint32_t>(code[0]) | (static_cast<uint32_t>(code[1]) << 8) |
                  (static_cast<uint32_t>(code[2]) << 16) | (static_cast<uint32_t>(code[3]) << 24);
    EXPECT_EQ(w0 & 0x7F, 0x53u);
    EXPECT_EQ((w0 >> 7) & 0x1F, 10u);   // fa0
    EXPECT_EQ((w0 >> 15) & 0x1F, 11u);  // fa1
    EXPECT_EQ((w0 >> 20) & 0x1F, 12u);  // fa2
    EXPECT_EQ((w0 >> 25) & 0x7F, 0x00u);

    // Word 1: VADD_VV v1, v2, v3 -> opcode 0x57
    uint32_t w1 = static_cast<uint32_t>(code[4]) | (static_cast<uint32_t>(code[5]) << 8) |
                  (static_cast<uint32_t>(code[6]) << 16) | (static_cast<uint32_t>(code[7]) << 24);
    EXPECT_EQ(w1 & 0x7F, 0x57u);
    EXPECT_EQ((w1 >> 7) & 0x1F, 1u);    // v1
    EXPECT_EQ((w1 >> 12) & 0x7, 0u);    // OPIVV
    EXPECT_EQ((w1 >> 15) & 0x1F, 2u);   // v2
    EXPECT_EQ((w1 >> 20) & 0x1F, 3u);   // v3

    // Word 2: VLE32_V v1, a0 -> opcode 0x07
    uint32_t w2 = static_cast<uint32_t>(code[8]) | (static_cast<uint32_t>(code[9]) << 8) |
                  (static_cast<uint32_t>(code[10]) << 16) | (static_cast<uint32_t>(code[11]) << 24);
    EXPECT_EQ(w2 & 0x7F, 0x07u);
    EXPECT_EQ((w2 >> 7) & 0x1F, 1u);
    EXPECT_EQ((w2 >> 12) & 0x7, 6u);    // e32
    EXPECT_EQ((w2 >> 15) & 0x1F, 10u);  // a0
    EXPECT_EQ((w2 >> 25) & 0x7F, 0x01u);

    // Word 3: VSE32_V v1, a0 -> opcode 0x27
    uint32_t w3 = static_cast<uint32_t>(code[12]) | (static_cast<uint32_t>(code[13]) << 8) |
                  (static_cast<uint32_t>(code[14]) << 16) | (static_cast<uint32_t>(code[15]) << 24);
    EXPECT_EQ(w3 & 0x7F, 0x27u);
    EXPECT_EQ((w3 >> 7) & 0x1F, 1u);
    EXPECT_EQ((w3 >> 12) & 0x7, 6u);    // e32
    EXPECT_EQ((w3 >> 15) & 0x1F, 10u);  // a0
    EXPECT_EQ((w3 >> 25) & 0x7F, 0x01u);
}
