#include "EzCodeEmitterTestSuite.h"
#include "BranchRelaxation/BranchRelaxer.h"

using namespace EzCodeEmitter;

// A short forward JMP stays 2 bytes and resolves to a positive disp8.
TEST_F(EzCodeEmitterTestSuite, TestShortForwardBranch)
{
    BranchRelaxer relaxer;
    // JMP forward to label 1
    relaxer.emitJmp(1);
    // 10 bytes of NOP
    uint8_t nops[10] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
    relaxer.emitBytes(nops, 10);
    // Label 1
    relaxer.defineLabel(1);

    std::vector<uint8_t> outCode;
    std::unordered_map<MirId, uint64_t> resolvedLabels;
    size_t relaxed = relaxer.relaxAndResolve(outCode, resolvedLabels);

    EXPECT_EQ(relaxed, 0u);
    // JMP short is 2 bytes: EB <disp8>
    ASSERT_EQ(outCode.size(), 12u);
    EXPECT_EQ(outCode[0], 0xEB);
    // Next IP is 2. Target offset is 12. Disp = 12 - 2 = 10.
    EXPECT_EQ(outCode[1], 10);
    EXPECT_EQ(resolvedLabels[1], 12u);
}

// A short backward JMP stays 2 bytes and resolves to a negative disp8.
TEST_F(EzCodeEmitterTestSuite, TestShortBackwardBranch)
{
    BranchRelaxer relaxer;
    // Label 1 at start
    relaxer.defineLabel(1);
    // 20 bytes of NOP
    std::vector<uint8_t> nops(20, 0x90);
    relaxer.emitBytes(nops);
    // JMP backward to label 1
    relaxer.emitJmp(1);

    std::vector<uint8_t> outCode;
    std::unordered_map<MirId, uint64_t> resolvedLabels;
    size_t relaxed = relaxer.relaxAndResolve(outCode, resolvedLabels);

    EXPECT_EQ(relaxed, 0u);
    ASSERT_EQ(outCode.size(), 22u);
    EXPECT_EQ(resolvedLabels[1], 0u);
    // Branch is at offset 20. Branch size is 2. Next IP is 22.
    // Disp = 0 - 22 = -22.
    EXPECT_EQ(outCode[20], 0xEB);
    EXPECT_EQ(outCode[21], static_cast<uint8_t>(-22));
}

// A forward JMP over more than 127 bytes is widened to a 5-byte near JMP with disp32.
TEST_F(EzCodeEmitterTestSuite, TestJumpRelaxationToNear)
{
    BranchRelaxer relaxer;
    // JMP forward to label 1
    relaxer.emitJmp(1);
    // 200 bytes of padding (> 127 bytes)
    std::vector<uint8_t> padding(200, 0x90);
    relaxer.emitBytes(padding);
    // Label 1
    relaxer.defineLabel(1);

    std::vector<uint8_t> outCode;
    std::unordered_map<MirId, uint64_t> resolvedLabels;
    size_t relaxed = relaxer.relaxAndResolve(outCode, resolvedLabels);

    EXPECT_EQ(relaxed, 1u); // Jump was expanded to Near!
    // Near JMP is 5 bytes (0xE9 disp32) + 200 bytes padding = 205 bytes
    ASSERT_EQ(outCode.size(), 205u);
    EXPECT_EQ(outCode[0], 0xE9);
    // Next IP is 5. Target offset is 205. Disp = 205 - 5 = 200.
    int32_t disp = 0;
    std::memcpy(&disp, &outCode[1], 4);
    EXPECT_EQ(disp, 200);
    EXPECT_EQ(resolvedLabels[1], 205u);
}

// A forward Jcc over more than 127 bytes is widened to a 6-byte near Jcc with disp32.
TEST_F(EzCodeEmitterTestSuite, TestConditionalJumpRelaxation)
{
    BranchRelaxer relaxer;
    // JE forward to label 42
    relaxer.emitJcc(TableGen::ConditionCode::E, 42);
    // 300 bytes of padding
    std::vector<uint8_t> padding(300, 0x90);
    relaxer.emitBytes(padding);
    // Label 42
    relaxer.defineLabel(42);

    std::vector<uint8_t> outCode;
    std::unordered_map<MirId, uint64_t> resolvedLabels;
    size_t relaxed = relaxer.relaxAndResolve(outCode, resolvedLabels);

    EXPECT_EQ(relaxed, 1u);
    // Near Jcc is 6 bytes (0x0F 0x84 disp32) + 300 bytes = 306 bytes
    ASSERT_EQ(outCode.size(), 306u);
    EXPECT_EQ(outCode[0], 0x0F);
    EXPECT_EQ(outCode[1], 0x84); // 0x80 | 0x04 (ConditionCode::E)
    int32_t disp = 0;
    std::memcpy(&disp, &outCode[2], 4);
    EXPECT_EQ(disp, 300);
    EXPECT_EQ(resolvedLabels[42], 306u);
}

// Widening one branch shifts later labels, forcing an earlier branch to expand as well.
TEST_F(EzCodeEmitterTestSuite, TestCascadingBranchRelaxation)
{
    BranchRelaxer relaxer;
    // Branch 1 targets label 1
    relaxer.emitJmp(1);
    // 126 bytes of padding
    std::vector<uint8_t> pad1(126, 0x90);
    relaxer.emitBytes(pad1);

    // Branch 2 targets label 2 (200 bytes ahead, forcing Branch 2 to expand)
    relaxer.emitJmp(2);
    std::vector<uint8_t> pad2(200, 0x90);
    relaxer.emitBytes(pad2);
    relaxer.defineLabel(2);

    // Label 1 is after Branch 2
    // Initially, before Branch 2 expands, distance to Label 1 is 126 + 2 = 128 (already borderline).
    relaxer.defineLabel(1);

    std::vector<uint8_t> outCode;
    std::unordered_map<MirId, uint64_t> resolvedLabels;
    size_t relaxed = relaxer.relaxAndResolve(outCode, resolvedLabels);

    // Both branches relaxed cleanly
    EXPECT_EQ(relaxed, 2u);
    EXPECT_EQ(outCode[0], 0xE9); // Branch 1 is near
}
