#include "EzCodeEmitterTestSuite.h"
#include "Helpers.h"
#include "X86_64CodeEmitter.h"
#include "Encoding/X86_64EncodingDesc.h"
#include "Encoding/X86_64InstructionEncoder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirRegisterClass.h"
#include "x86_64EncodingTable.h"

#include <span>
#include <vector>

using namespace EzCodeEmitter;
using namespace EzTargets::X86_64;

namespace
{

/**
 * Emits a single instruction through the table-driven emitter, resolving the encoding
 * from the generated x86-64 table by instruction name.
 */
std::vector<uint8_t> emitInstruction(MirTargetInstructionDesc *desc,
                                     std::span<MirOperand *> operands,
                                     DiagnosticCollector *diag,
                                     std::pmr::memory_resource *alloc)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(alloc);
    Helpers::ObjectFormat::CreateElfSections(sections, alloc);

    CodeEmitterContext context(diag, sections, alloc);
    X86_64CodeEmitter emitter;
    emitter.setEncodingResolver([](const MirTargetInstructionDesc *d) -> const EncodingDesc *
                                { return findEncodingDesc(d->getName()); });

    emitter.beginFunction(&context, "test_fn");
    emitter.emitInst(desc, operands);
    emitter.endFunction(&context);

    CodeSection *text = sections[SectionType::Text];
    text->finalize();
    std::span<const uint8_t> data = text->getData();
    return std::vector<uint8_t>(data.begin(), data.end());
}

} // namespace

/**
 * Verifies that the generated encoding table drives the x86-64 emitter to produce the
 * exact expected machine bytes, including two-address coalescing and REX selection.
 */
TEST_F(EzCodeEmitterTestSuite, TestTableDrivenEmitterProducesExpectedBytes)
{
    MirOperandBuilder opBuilder(getBuilderCtx());

    MirType *i64 = getTypeTable()->i64();
    MirType *i32 = getTypeTable()->i32();
    MirType *i16 = getTypeTable()->i16();
    MirType *i8 = getTypeTable()->i8();
    MirType *f32 = getTypeTable()->f32();
    MirType *f64 = getTypeTable()->f64();

    MirRegisterClass gpr64Class("GPR64", nullptr, getAllocator());
    MirRegisterClass gpr32Class("GPR32", nullptr, getAllocator());
    MirRegisterClass gpr16Class("GPR16", nullptr, getAllocator());
    MirRegisterClass gpr8Class("GPR8", nullptr, getAllocator());
    MirRegisterClass fpr32Class("FPR32", nullptr, getAllocator());
    MirRegisterClass fpr64Class("FPR64", nullptr, getAllocator());
    MirRegisterClass vr128Class("VR128", nullptr, getAllocator());

    auto makeReg = [&](MirType *type, size_t id, MirRegisterClass *cls) -> MirRegister *
    {
        MirRegister *reg = opBuilder.buildVReg(type, "", nullptr, cls);
        reg->setRef(MirRegisterRef(id, /*isVirtual=*/false, cls));
        return reg;
    };

    auto g64 = [&](size_t id) { return makeReg(i64, id, &gpr64Class); };
    auto g32 = [&](size_t id) { return makeReg(i32, id, &gpr32Class); };
    auto g16 = [&](size_t id) { return makeReg(i16, id, &gpr16Class); };
    auto g8 = [&](size_t id) { return makeReg(i8, id, &gpr8Class); };
    auto s32 = [&](size_t id) { return makeReg(f32, id, &fpr32Class); };
    auto s64 = [&](size_t id) { return makeReg(f64, id, &fpr64Class); };
    auto v128 = [&](size_t id) { return makeReg(getTypeTable()->v4f32(), id, &vr128Class); };

    MirRegister *r0 = g64(0);
    MirRegister *r1 = g64(1);
    MirRegister *r2 = g64(2);
    MirRegister *d0 = g32(0);
    MirRegister *d1 = g32(1);
    MirRegister *d2 = g32(2);
    MirRegister *w0 = g16(0);
    MirRegister *w1 = g16(1);
    MirRegister *b0 = g8(0);
    MirRegister *b1 = g8(1);
    MirRegister *x0 = s32(0);
    MirRegister *x1 = s32(1);
    MirRegister *x2 = s32(2);
    MirRegister *y0 = s64(0);
    MirRegister *y1 = s64(1);
    MirRegister *v0 = v128(0);
    MirRegister *v1 = v128(1);
    MirRegister *v2 = v128(2);
    MirRegister *v8 = v128(8);

    MirInteger *imm8 = opBuilder.buildInt(i8, FlexInt(3, 8));
    MirInteger *imm16 = opBuilder.buildInt(i16, FlexInt(7, 16));
    MirInteger *imm32 = opBuilder.buildInt(i32, FlexInt(42, 32));
    MirInteger *imm32Big = opBuilder.buildInt(i32, FlexInt(1000, 32));
    MirInteger *imm64 = opBuilder.buildInt(i64, FlexInt(42, 64));
    MirInteger *imm64Big = opBuilder.buildInt(i64, FlexInt(static_cast<int64_t>(0x1122334455667788LL), 64));
    MirInteger *disp = opBuilder.buildInt(i64, FlexInt(16, 64));

    auto expect = [&](const char *name, std::vector<MirOperand *> ops, std::vector<uint8_t> expected)
    {
        MirTargetInstructionDesc desc(name, 0);
        std::vector<uint8_t> actual = emitInstruction(&desc, ops, getDiagCollector(), getAllocator());
        EXPECT_EQ(actual, expected) << "Unexpected encoding for instruction: " << name;
    };

    // Data movement.
    expect("MOV8rr", { b0, b1 }, { 0x88, 0xC8 });
    expect("MOV16rr", { w0, w1 }, { 0x66, 0x89, 0xC8 });
    expect("MOV32rr", { d0, d1 }, { 0x89, 0xC8 });
    expect("MOV64rr", { r0, r1 }, { 0x48, 0x89, 0xC8 });
    expect("MOV64ri", { r0, imm64 }, { 0x48, 0xC7, 0xC0, 0x2A, 0x00, 0x00, 0x00 });
    expect("MOV64ri", { r0, imm64Big }, { 0x48, 0xB8, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 });

    // Two-address ALU forms: a copy is materialized when dst and the first source differ.
    expect("ADD64rr", { r0, r1, r2 }, { 0x48, 0x89, 0xC8, 0x48, 0x01, 0xD0 });
    expect("ADD64rr", { r0, r0, r2 }, { 0x48, 0x01, 0xD0 });
    expect("SUB64rr", { r0, r1, r2 }, { 0x48, 0x89, 0xC8, 0x48, 0x29, 0xD0 });
    expect("IMUL64rr", { r0, r1, r2 }, { 0x48, 0x89, 0xC8, 0x48, 0x0F, 0xAF, 0xC2 });

    // Immediate-width selection (0x83 vs 0x81).
    expect("ADD64ri", { r0, r1, imm8 }, { 0x48, 0x89, 0xC8, 0x48, 0x83, 0xC0, 0x03 });
    expect("ADD64ri", { r0, r1, imm32Big }, { 0x48, 0x89, 0xC8, 0x48, 0x81, 0xC0, 0xE8, 0x03, 0x00, 0x00 });

    // Comparison, test, unary and shift forms.
    expect("CMP64rr", { r1, r2 }, { 0x48, 0x39, 0xD1 });
    expect("TEST64rr", { r1, r2 }, { 0x48, 0x85, 0xD1 });
    expect("NEG64r", { r0, r1 }, { 0x48, 0x89, 0xC8, 0x48, 0xF7, 0xD8 });
    expect("SHL64ri", { r0, r1, imm8 }, { 0x48, 0x89, 0xC8, 0x48, 0xC1, 0xE0, 0x03 });
    expect("SETE", { b0 }, { 0x0F, 0x94, 0xC0 });

    // Control flow and system instructions.
    expect("JMP", { disp }, { 0xE9, 0x10, 0x00, 0x00, 0x00 });
    expect("JE", { disp }, { 0x0F, 0x84, 0x10, 0x00, 0x00, 0x00 });
    expect("PUSH64r", { r1 }, { 0x51 });
    expect("POP64r", { r0 }, { 0x58 });
    expect("RET", {}, { 0xC3 });
    expect("NOP", {}, { 0x90 });
    expect("SYSCALL", {}, { 0x0F, 0x05 });

    // SSE and scalar conversions.
    expect("ADDSS", { x0, x1, x2 }, { 0xF3, 0x0F, 0x10, 0xC1, 0xF3, 0x0F, 0x58, 0xC2 });
    expect("ADDSD", { y0, y0, y1 }, { 0xF2, 0x0F, 0x58, 0xC1 });
    expect("CVTSI2SS", { x0, d1 }, { 0xF3, 0x0F, 0x2A, 0xC1 });
    expect("CVTSI2SS", { x0, r1 }, { 0xF3, 0x48, 0x0F, 0x2A, 0xC1 });

    // SSE vector operations across versions (SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2).
    expect("ADDPSrr", { v0, v0, v1 }, { 0x0F, 0x58, 0xC1 });
    expect("ADDPSrr", { v0, v1, v2 }, { 0x0F, 0x28, 0xC1, 0x0F, 0x58, 0xC2 });
    expect("ADDPSrr", { v0, v0, v8 }, { 0x41, 0x0F, 0x58, 0xC0 });
    expect("MOVAPSrr", { v0, v1 }, { 0x0F, 0x28, 0xC1 });
    expect("ADDPDrr", { v0, v0, v1 }, { 0x66, 0x0F, 0x58, 0xC1 });
    expect("PADDBrr", { v0, v0, v1 }, { 0x66, 0x0F, 0xFC, 0xC1 });
    expect("PADDDrr", { v0, v0, v1 }, { 0x66, 0x0F, 0xFE, 0xC1 });
    expect("MOVDQUrr", { v0, v1 }, { 0xF3, 0x0F, 0x6F, 0xC1 });
    expect("HADDPSrr", { v0, v0, v1 }, { 0xF2, 0x0F, 0x7C, 0xC1 });
    expect("HADDPDrr", { v0, v0, v1 }, { 0x66, 0x0F, 0x7C, 0xC1 });
    expect("PHADDDrr", { v0, v0, v1 }, { 0x66, 0x0F, 0x38, 0x02, 0xC1 });
    expect("PMULLDrr", { v0, v0, v1 }, { 0x66, 0x0F, 0x38, 0x40, 0xC1 });
    expect("PCMPGTQrr", { v0, v0, v1 }, { 0x66, 0x0F, 0x38, 0x37, 0xC1 });
}

/**
 * Direct unit coverage of the target-agnostic InstructionEncoder runtime.
 */
TEST_F(EzCodeEmitterTestSuite, TestRuntimeInstructionEncoderPrimitives)
{
    {
        // MOV r/m64, r64 -> 48 89 /r with reg=src, rm=dst.
        EncodingDesc desc{};
        desc.m_form = EncForm::Rr;
        desc.m_rexW = 2;
        desc.m_opcode[0] = 0x89;
        desc.m_opcodeLen = 1;
        desc.m_operandCount = 2;
        desc.m_operands[0] = EncOperandBinding{ EncSlotKind::Reg, 1, EncRegClass::GPR };
        desc.m_operands[1] = EncOperandBinding{ EncSlotKind::RmReg, 0, EncRegClass::GPR };
        desc.m_sizeOperand = 0;

        std::vector<ResolvedOperand> ops(2);
        ops[0].m_kind = ResolvedOperand::Kind::Register;
        ops[0].m_reg = 0;
        ops[0].m_sizeBytes = 8;
        ops[1].m_kind = ResolvedOperand::Kind::Register;
        ops[1].m_reg = 1;
        ops[1].m_sizeBytes = 8;

        std::vector<uint8_t> out;
        EncodeResult result;
        ASSERT_TRUE(InstructionEncoder::encode(desc, ops, out, result));
        ASSERT_EQ(out.size(), 3u);
        EXPECT_EQ(out[0], 0x48);
        EXPECT_EQ(out[1], 0x89);
        EXPECT_EQ(out[2], 0xC8);
        EXPECT_FALSE(result.m_hasReloc);
    }

    {
        // JNE rel32 -> 0F 85 00000000 with a reported branch relocation.
        EncodingDesc desc{};
        desc.m_form = EncForm::Jcc;
        desc.m_opcode[0] = 0x0F;
        desc.m_opcode[1] = 0x80;
        desc.m_opcodeLen = 2;
        desc.m_condCode = 5;
        desc.m_operandCount = 1;
        desc.m_operands[0] = EncOperandBinding{ EncSlotKind::Rel32, 0, EncRegClass::Any };

        std::vector<ResolvedOperand> ops(1);
        ops[0].m_kind = ResolvedOperand::Kind::Immediate;
        ops[0].m_needsReloc = true;

        std::vector<uint8_t> out;
        EncodeResult result;
        ASSERT_TRUE(InstructionEncoder::encode(desc, ops, out, result));
        ASSERT_EQ(out.size(), 6u);
        EXPECT_EQ(out[0], 0x0F);
        EXPECT_EQ(out[1], 0x85);
        EXPECT_EQ(out[2], 0x00);
        EXPECT_TRUE(result.m_hasReloc);
        EXPECT_TRUE(result.m_isBranch);
        EXPECT_EQ(result.m_relocOffset, 2u);
    }

    {
        // Encoders reject malformed descriptors rather than emitting garbage.
        EncodingDesc desc{};
        desc.m_form = EncForm::Rr;
        std::vector<ResolvedOperand> ops;
        std::vector<uint8_t> out;
        EncodeResult result;
        EXPECT_FALSE(InstructionEncoder::encode(desc, ops, out, result));
    }
}
