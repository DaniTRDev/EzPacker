#include "EzCodeEmitterTestSuite.h"
#include "Helpers.h"
#include "X86_64/X86_64CodeEmitter.h"
#include "TableGen/EncodingDesc.h"
#include "TableGen/InstructionEncoder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirRegisterClass.h"
#include "x86_64EncodingTable.h"

#include <span>
#include <vector>

using namespace EzCodeEmitter;
using namespace EzCodeEmitter::X86_64;

namespace
{

std::vector<uint8_t> emitInstruction(bool tableDriven,
                                     MirTargetInstructionDesc *desc,
                                     std::span<MirOperand *> operands,
                                     MirBuilderContext *builderCtx,
                                     DiagnosticCollector *diag,
                                     std::pmr::memory_resource *alloc)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(alloc);
    Helpers::ObjectFormat::CreateElfSections(sections, alloc);

    CodeEmitterContext context(diag, sections, alloc);
    X86_64CodeEmitter emitter;

    if (tableDriven)
    {
        emitter.setEncodingResolver([](MirTargetInstructionDesc *d) -> const TableGen::EncodingDesc *
                                    { return TableGen::x86_64::findEncodingDesc(d->getName()); });
    }

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
 * Differential verification: the generated table-driven encoder must produce exactly the
 * same bytes as the legacy hand-written encoder for every instruction shape.
 */
TEST_F(EzCodeEmitterTestSuite, TestTableDrivenMatchesLegacyEncoder)
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
    MirRegister *y2 = s64(2);

    MirInteger *imm8 = opBuilder.buildInt(i8, FlexInt(3, 8));
    MirInteger *imm16 = opBuilder.buildInt(i16, FlexInt(7, 16));
    MirInteger *imm32 = opBuilder.buildInt(i32, FlexInt(42, 32));
    MirInteger *imm32Big = opBuilder.buildInt(i32, FlexInt(1000, 32));
    MirInteger *imm64 = opBuilder.buildInt(i64, FlexInt(42, 64));
    MirInteger *imm64Big = opBuilder.buildInt(i64, FlexInt(static_cast<int64_t>(0x1122334455667788LL), 64));
    MirInteger *disp = opBuilder.buildInt(i64, FlexInt(16, 64));

    auto check = [&](const char *name, std::vector<MirOperand *> ops)
    {
        MirTargetInstructionDesc desc(name, 0);
        std::vector<uint8_t> legacy =
                emitInstruction(false, &desc, ops, getBuilderCtx(), getDiagCollector(), getAllocator());
        std::vector<uint8_t> table =
                emitInstruction(true, &desc, ops, getBuilderCtx(), getDiagCollector(), getAllocator());
        EXPECT_EQ(legacy, table) << "Mismatch for instruction: " << name;
    };

    // Data movement.
    check("MOV8rr", { b0, b1 });
    check("MOV16rr", { w0, w1 });
    check("MOV32rr", { d0, d1 });
    check("MOV64rr", { r0, r1 });
    check("MOV8ri", { b0, imm8 });
    check("MOV16ri", { w0, imm16 });
    check("MOV32ri", { d0, imm32 });
    check("MOV64ri", { r0, imm64 });
    check("MOV64ri", { r0, imm64Big });
    check("MOVSX64_8", { r0, b1 });
    check("MOVSX64_32", { r0, d1 });
    check("MOVZX64_16", { r0, w1 });

    // Integer ALU (register-register, destructive two-address forms).
    check("ADD32rr", { d0, d1, d2 });
    check("ADD64rr", { r0, r1, r2 });
    check("ADD64rr", { r0, r0, r2 });
    check("SUB64rr", { r0, r1, r2 });
    check("AND64rr", { r0, r1, r2 });
    check("OR64rr", { r0, r1, r2 });
    check("XOR64rr", { r0, r1, r2 });
    check("IMUL64rr", { r0, r1, r2 });

    // Integer ALU (immediate forms exercising 0x83/0x81 selection).
    check("ADD64ri", { r0, r1, imm8 });
    check("ADD64ri", { r0, r1, imm32Big });
    check("ADD32ri", { d0, d1, imm32 });
    check("SUB64ri", { r0, r1, imm8 });
    check("AND64ri", { r0, r1, imm8 });
    check("OR64ri", { r0, r1, imm8 });
    check("XOR64ri", { r0, r1, imm8 });
    check("CMP64ri", { r1, imm8 });
    check("IMUL64ri", { r0, r1, imm8 });
    check("IMUL64ri", { r0, r1, imm32Big });

    // Comparisons, tests, unary and shifts.
    check("CMP64rr", { r1, r2 });
    check("TEST64rr", { r1, r2 });
    check("NEG64r", { r0, r1 });
    check("NOT64r", { r0, r1 });
    check("IDIV64r", { r1 });
    check("DIV64r", { r1 });
    check("SHL64ri", { r0, r1, imm8 });
    check("SHR64ri", { r0, r1, imm8 });
    check("SAR64ri", { r0, r1, imm8 });
    check("SHL64rCL", { r0, r1 });
    check("SHR64rCL", { r0, r1 });
    check("SAR64rCL", { r0, r1 });

    // Conditional set and control flow with explicit displacements.
    check("SETE", { b0 });
    check("SETNE", { b0 });
    check("JMP", { disp });
    check("JE", { disp });
    check("JNE", { disp });
    check("PUSH64r", { r1 });
    check("POP64r", { r0 });
    check("RET", {});
    check("NOP", {});
    check("SYSCALL", {});

    // Floating point / SSE and conversions.
    check("ADDSS", { x0, x1, x2 });
    check("ADDSD", { y0, y1, y2 });
    check("SUBSS", { x0, x1, x2 });
    check("MULSD", { y0, y1, y2 });
    check("DIVSS", { x0, x1, x2 });
    check("MOVSSrr", { x0, x1 });
    check("MOVSDrr", { y0, y1 });
    check("UCOMISS", { x0, x1 });
    check("UCOMISD", { y0, y1 });
    check("CVTSI2SS", { x0, d1 });
    check("CVTSI2SS", { x0, r1 });
    check("CVTSI2SD", { y0, r1 });
    check("CVTTSS2SI", { d0, x1 });
    check("CVTTSD2SI", { r0, y1 });
}

/**
 * Direct unit coverage of the target-agnostic TableGen::InstructionEncoder runtime.
 */
TEST_F(EzCodeEmitterTestSuite, TestRuntimeInstructionEncoderPrimitives)
{
    using namespace TableGen;

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
        ASSERT_TRUE(TableGen::InstructionEncoder::encode(desc, ops, out, result));
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
        ASSERT_TRUE(TableGen::InstructionEncoder::encode(desc, ops, out, result));
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
        EXPECT_FALSE(TableGen::InstructionEncoder::encode(desc, ops, out, result));
    }
}
