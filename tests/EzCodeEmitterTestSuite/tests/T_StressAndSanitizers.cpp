#include "EzCodeEmitterTestSuite.h"
#include "Helpers.h"
#include "X86_64/X86_64Encoding.h"
#include "BranchRelaxation/BranchRelaxer.h"
#include <chrono>

using namespace EzCodeEmitter;
using namespace EzCodeEmitter::X86_64;

TEST_F(EzCodeEmitterTestSuite, TestMassiveBasicBlockEmission)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeSection *textSec = sections[SectionType::Text];

    auto startTime = std::chrono::high_resolution_clock::now();

    // Emit 5,000 machine instructions
    constexpr size_t NUM_INSTRUCTIONS = 5000;
    std::vector<uint8_t> buf;
    buf.reserve(16);

    for (size_t i = 0; i < NUM_INSTRUCTIONS; ++i)
    {
        buf.clear();
        Reg r1 = static_cast<Reg>(i % 16);
        Reg r2 = static_cast<Reg>((i + 1) % 16);

        switch (i % 4)
        {
            case 0:
                InstructionEncoder::emitMovRR(buf, r1, r2, 8);
                break;
            case 1:
                InstructionEncoder::emitAluRR(buf, AluOp::ADD, r1, r2, 8);
                break;
            case 2:
                InstructionEncoder::emitAluRI(buf, AluOp::SUB, r1, static_cast<int32_t>(i & 0xFF), 8);
                break;
            case 3:
                InstructionEncoder::emitAluRR(buf, AluOp::XOR, r1, r1, 8);
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

TEST_F(EzCodeEmitterTestSuite, TestMassiveBranchRelaxationStress)
{
    BranchRelaxer relaxer;
    constexpr size_t NUM_BRANCHES = 100;
    constexpr size_t PAD_SIZE = 150; // Each gap is > 127 bytes, forcing near jumps

    std::vector<uint8_t> padding(PAD_SIZE, 0x90);

    for (size_t i = 0; i < NUM_BRANCHES; ++i)
    {
        relaxer.emitJcc(X86_64::ConditionCode::NE, static_cast<MirId>(i + 1));
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
