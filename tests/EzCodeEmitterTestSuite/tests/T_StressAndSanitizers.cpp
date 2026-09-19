#include "EzCodeEmitterTestSuite.h"
#include "Helpers.h"
#include "TableGen/EncodingDesc.h"
#include "TableGen/InstructionEncoder.h"
#include "BranchRelaxation/BranchRelaxer.h"
#include <chrono>

using namespace EzCodeEmitter;

namespace
{

using TableGen::EncForm;
using TableGen::EncodingDesc;
using TableGen::EncOperandBinding;
using TableGen::EncRegClass;
using TableGen::EncSlotKind;

/**
 * Builds a two-operand, two-address register instruction descriptor with the given
 * MR-form opcode (0x88/0x89 style), REX.W policy and register bindings.
 */
EncodingDesc makeMrForm(uint8_t opcode, uint8_t sizeOperand, uint8_t regOperand, uint8_t rmOperand)
{
    EncodingDesc desc{};
    desc.m_form = EncForm::Rr;
    desc.m_rexW = 2;
    desc.m_opcode[0] = opcode;
    desc.m_opcodeLen = 1;
    desc.m_operandCount = 2;
    desc.m_operands[0] = EncOperandBinding{ EncSlotKind::Reg, regOperand, EncRegClass::GPR };
    desc.m_operands[1] = EncOperandBinding{ EncSlotKind::RmReg, rmOperand, EncRegClass::GPR };
    desc.m_sizeOperand = sizeOperand;
    return desc;
}

} // namespace

// Encodes 5,000 register-register instructions through the table-driven runtime and checks it completes within the time
// bound.
TEST_F(EzCodeEmitterTestSuite, TestMassiveBasicBlockEmission)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeSection *textSec = sections[SectionType::Text];

    auto startTime = std::chrono::high_resolution_clock::now();

    // Encode 5,000 machine instructions through the table-driven runtime.
    constexpr size_t NUM_INSTRUCTIONS = 5000;
    std::vector<uint8_t> buf;
    buf.reserve(16);

    const EncodingDesc movDesc = makeMrForm(0x89, /*sizeOperand=*/0, /*regOperand=*/1, /*rmOperand=*/0);
    const EncodingDesc addDesc = makeMrForm(0x01, /*sizeOperand=*/0, /*regOperand=*/1, /*rmOperand=*/0);
    const EncodingDesc xorDesc = makeMrForm(0x31, /*sizeOperand=*/0, /*regOperand=*/1, /*rmOperand=*/0);

    std::vector<TableGen::ResolvedOperand> operands(2);
    operands[0].m_kind = TableGen::ResolvedOperand::Kind::Register;
    operands[0].m_sizeBytes = 8;
    operands[1].m_kind = TableGen::ResolvedOperand::Kind::Register;
    operands[1].m_sizeBytes = 8;

    for (size_t i = 0; i < NUM_INSTRUCTIONS; ++i)
    {
        buf.clear();
        operands[0].m_reg = static_cast<uint8_t>(i % 16);
        operands[1].m_reg = static_cast<uint8_t>((i + 1) % 16);

        TableGen::EncodeResult result;
        switch (i % 3)
        {
            case 0:
                TableGen::InstructionEncoder::encode(movDesc, operands, buf, result);
                break;
            case 1:
                TableGen::InstructionEncoder::encode(addDesc, operands, buf, result);
                break;
            default:
                TableGen::InstructionEncoder::encode(xorDesc, operands, buf, result);
                break;
        }

        textSec->emitBytes(buf.data(), buf.size());
    }

    textSec->finalize();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // 5,000 instructions must encode and serialize within 500ms (linear performance)
    EXPECT_LT(elapsedMs, 500);

    std::span<const uint8_t> code = textSec->getData();
    EXPECT_GT(code.size(), NUM_INSTRUCTIONS * 2);
}

// Relaxes 100 conditional branches separated by >127-byte gaps and verifies all are widened within the time bound.
TEST_F(EzCodeEmitterTestSuite, TestMassiveBranchRelaxationStress)
{
    BranchRelaxer relaxer;
    constexpr size_t NUM_BRANCHES = 100;
    constexpr size_t PAD_SIZE = 150; // Each gap is > 127 bytes, forcing near jumps

    std::vector<uint8_t> padding(PAD_SIZE, 0x90);

    for (size_t i = 0; i < NUM_BRANCHES; ++i)
    {
        relaxer.emitJcc(TableGen::ConditionCode::NE, static_cast<MirId>(i + 1));
        relaxer.emitBytes(padding);
        relaxer.defineLabel(static_cast<MirId>(i + 1));
    }

    std::vector<uint8_t> outCode;
    std::unordered_map<MirId, uint64_t> resolvedLabels;

    auto startTime = std::chrono::high_resolution_clock::now();
    size_t relaxedCount = relaxer.relaxAndResolve(outCode, resolvedLabels);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // All 100 branches should relax because distance is 150 > 127
    EXPECT_EQ(relaxedCount, NUM_BRANCHES);
    EXPECT_EQ(resolvedLabels.size(), NUM_BRANCHES);
    EXPECT_LT(elapsedMs, 200);
}
