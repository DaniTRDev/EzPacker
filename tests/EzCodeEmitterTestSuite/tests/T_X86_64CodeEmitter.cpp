#include "EzCodeEmitterTestSuite.h"
#include "Helpers.h"
#include "X86_64/X86_64CodeEmitter.h"
#include "X86_64/Encoding/X86_64EncodingDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "x86_64EncodingTable.h"

#include <stdexcept>

using namespace EzCodeEmitter;
using namespace EzCodeEmitter::X86_64;

// Emits MOV/ADD/RET through the x86-64 emitter and verifies the exact sequence of machine bytes.
TEST_F(EzCodeEmitterTestSuite, TestFullEmitterIntegration)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeEmitterContext context(getDiagCollector(), sections, getAllocator());

    X86_64CodeEmitter emitter;
    emitter.setEncodingResolver([](const MirTargetInstructionDesc *desc) -> const EncodingDesc *
                                { return findEncodingDesc(desc->getName()); });
    emitter.beginFunction(&context, "main");

    MirOperandBuilder opBuilder(getBuilderCtx());
    auto *i64 = getTypeTable()->i64();

    // vreg0 (RAX)
    MirRegister *vreg0 = opBuilder.buildVReg(i64);
    // Force vreg0 to physical ID 0 (RAX)
    vreg0->setRef(MirRegisterRef(0, /*isVirtual=*/false));

    // 1. MOV64ri: RAX = 42
    MirTargetInstructionDesc descMOV64ri("MOV64ri", 100);
    MirInteger *imm42 = opBuilder.buildInt(i64, FlexInt(42, 64));
    MirOperand *opsMov[] = { vreg0, imm42 };
    emitter.emitInst(&descMOV64ri, opsMov);

    // 2. ADD64ri: RAX = RAX + 10
    MirTargetInstructionDesc descADD64ri("ADD64ri", 101);
    MirInteger *imm10 = opBuilder.buildInt(i64, FlexInt(10, 64));
    MirOperand *opsAdd[] = { vreg0, vreg0, imm10 };
    emitter.emitInst(&descADD64ri, opsAdd);

    // 3. RET
    MirTargetInstructionDesc descRET("RET", 102);
    emitter.emitInst(&descRET, {});

    emitter.endFunction(&context);

    CodeSection *textSec = sections[SectionType::Text];
    textSec->finalize();

    std::span<const uint8_t> code = textSec->getData();
    // MOV RAX, 42: 48 C7 C0 2A 00 00 00 (7 bytes)
    // ADD RAX, 10: 48 83 C0 0A (4 bytes)
    // RET: C3 (1 byte)
    // Total = 12 bytes
    ASSERT_EQ(code.size(), 12u);

    // Check MOV
    EXPECT_EQ(code[0], 0x48);
    EXPECT_EQ(code[1], 0xC7);
    EXPECT_EQ(code[2], 0xC0);
    EXPECT_EQ(code[3], 42);

    // Check ADD
    EXPECT_EQ(code[7], 0x48);
    EXPECT_EQ(code[8], 0x83);
    EXPECT_EQ(code[9], 0xC0);
    EXPECT_EQ(code[10], 10);

    // Check RET
    EXPECT_EQ(code[11], 0xC3);
}

// WEI-03: an instruction the encoding table cannot resolve must throw instead of vanishing.
TEST_F(EzCodeEmitterTestSuite, TestEmitterRejectsUnknownEncoding)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeEmitterContext context(getDiagCollector(), sections, getAllocator());

    X86_64CodeEmitter emitter;
    emitter.setEncodingResolver([](const MirTargetInstructionDesc *desc) -> const EncodingDesc *
                                { return findEncodingDesc(desc->getName()); });
    emitter.beginFunction(&context, "bad");

    MirTargetInstructionDesc unknown("NOT_A_REAL_INSTRUCTION", 999);
    EXPECT_THROW(emitter.emitInst(&unknown, {}), std::runtime_error);
}
