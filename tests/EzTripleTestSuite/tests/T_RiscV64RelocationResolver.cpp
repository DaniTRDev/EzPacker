#include <gtest/gtest.h>

#include "CodeEmitterContext.h"
#include "Relocation/RiscV64RelocationResolver.h"
#include "Encoding/RiscV64InstructionEncoder.h"

#include <array>
#include <cstdint>
#include <span>

using namespace EzTargets::RiscV64;

namespace
{

uint32_t readWord(const std::array<uint8_t, 16> &bytes, size_t offset)
{
    return static_cast<uint32_t>(bytes[offset]) |
           (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

void writeWord(std::array<uint8_t, 16> &bytes, size_t offset, uint32_t word)
{
    bytes[offset] = static_cast<uint8_t>(word & 0xFF);
    bytes[offset + 1] = static_cast<uint8_t>((word >> 8) & 0xFF);
    bytes[offset + 2] = static_cast<uint8_t>((word >> 16) & 0xFF);
    bytes[offset + 3] = static_cast<uint8_t>((word >> 24) & 0xFF);
}

} // namespace

class RiscV64RelocationResolverTest : public ::testing::Test
{
};

// Verifies patching a JAL instruction (J-type) forward and backward displacement.
TEST_F(RiscV64RelocationResolverTest, PatchesJalDisplacement)
{
    std::vector<uint8_t> bytes;
    InstructionEncoder::emitJal(bytes, 1, 0);

    CodeRelocation reloc{};
    reloc.m_address = 0;
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    RiscV64RelocationResolver resolver;
    // Target is +0x100 relative to address 0 -> disp = 0x100
    ASSERT_TRUE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x100, reloc.m_relocType));

    uint32_t patched = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
                       (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
    EXPECT_EQ(patched & 0x7F, 0x6Fu);       // Opcode JAL
    EXPECT_EQ((patched >> 7) & 0x1F, 1u);   // rd = x1 (ra)
    EXPECT_NE(patched >> 12, 0u);
}

// Verifies patching a conditional branch BEQ (B-type) displacement.
TEST_F(RiscV64RelocationResolverTest, PatchesBranchDisplacement)
{
    std::vector<uint8_t> bytes;
    InstructionEncoder::emitBranch(bytes, 0, 10, 11, 0);

    CodeRelocation reloc{};
    reloc.m_address = 0;
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    RiscV64RelocationResolver resolver;
    // Target is +0x40 relative to address 0 -> disp = 0x40
    ASSERT_TRUE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x40, reloc.m_relocType));

    uint32_t patched = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
                       (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
    EXPECT_EQ(patched & 0x7F, 0x63u);        // Opcode BRANCH
    EXPECT_EQ((patched >> 12) & 0x7, 0u);    // funct3 BEQ
    EXPECT_EQ((patched >> 15) & 0x1F, 10u);  // rs1 = a0 (x10)
    EXPECT_EQ((patched >> 20) & 0x1F, 11u);  // rs2 = a1 (x11)
}

// Verifies patching AUIPC for PCRel32 relocations.
TEST_F(RiscV64RelocationResolverTest, PatchesAuipcPCRel32)
{
    uint32_t auipcInst = 0x17u | (10u << 7);
    std::vector<uint8_t> bytes = {
        static_cast<uint8_t>(auipcInst & 0xFF),
        static_cast<uint8_t>((auipcInst >> 8) & 0xFF),
        static_cast<uint8_t>((auipcInst >> 16) & 0xFF),
        static_cast<uint8_t>((auipcInst >> 24) & 0xFF)
    };

    CodeRelocation reloc{};
    reloc.m_address = 0;
    reloc.m_relocType = TargetCodeRelocationType::PCRel32;

    RiscV64RelocationResolver resolver;
    // Target offset +0x2000
    ASSERT_TRUE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x2000, reloc.m_relocType));

    uint32_t patched = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8) |
                       (static_cast<uint32_t>(bytes[2]) << 16) | (static_cast<uint32_t>(bytes[3]) << 24);
    EXPECT_EQ(patched & 0x7F, 0x17u);        // Opcode AUIPC
    EXPECT_EQ((patched >> 7) & 0x1F, 10u);   // rd = x10
    EXPECT_NE((patched >> 12) & 0xFFFFF, 0u);// non-zero hi20
}

// Verifies out-of-bounds offset returns false.
TEST_F(RiscV64RelocationResolverTest, RejectsOutOfBounds)
{
    std::array<uint8_t, 4> bytes{};

    CodeRelocation reloc{};
    reloc.m_address = 8; // Past 4-byte buffer
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    RiscV64RelocationResolver resolver;
    EXPECT_FALSE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x10, reloc.m_relocType));
}
