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
