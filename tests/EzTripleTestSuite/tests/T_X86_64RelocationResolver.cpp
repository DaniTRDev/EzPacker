#include <gtest/gtest.h>

#include "CodeEmitterContext.h"
#include "Targets/X86_64/X86_64RelocationResolver.h"

#include <array>
#include <cstdint>
#include <span>

using namespace EzTriple;

namespace
{

/**
 * Reads a little-endian 32-bit value from a fixed byte array at the given offset.
 */
uint32_t read32(const std::array<uint8_t, 16> &bytes, size_t offset)
{
    return static_cast<uint32_t>(bytes[offset]) | (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
            (static_cast<uint32_t>(bytes[offset + 2]) << 16) | (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

} // namespace

/**
 * Fixture for the x86-64 relocation resolver's patch behavior.
 */
class X86_64RelocationResolverTest : public ::testing::Test
{
};

// Verifies a JMP rel32 is patched relative to the end of the 5-byte instruction.
TEST_F(X86_64RelocationResolverTest, PatchesNearJumpRel32)
{
    std::array<uint8_t, 16> bytes{};
    bytes[0] = 0xE9;

    CodeRelocation reloc{};
    reloc.m_address = 0;
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    X86_64RelocationResolver resolver;
    ASSERT_TRUE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x100, reloc.m_relocType));

    EXPECT_EQ(read32(bytes, 1), 0x100u - 5u);
}

// Verifies a CALL rel32 is patched relative to the end of the 5-byte instruction.
TEST_F(X86_64RelocationResolverTest, PatchesNearCallRel32)
{
    std::array<uint8_t, 16> bytes{};
    bytes[0] = 0xE8;

    CodeRelocation reloc{};
    reloc.m_address = 0;
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    X86_64RelocationResolver resolver;
    ASSERT_TRUE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x40, reloc.m_relocType));

    EXPECT_EQ(read32(bytes, 1), 0x40u - 5u);
}

// Verifies a Jcc rel32 is patched relative to the end of the 6-byte instruction.
TEST_F(X86_64RelocationResolverTest, PatchesNearConditionalJumpRel32)
{
    std::array<uint8_t, 16> bytes{};
    bytes[0] = 0x0F;
    bytes[1] = 0x84; // JE rel32

    CodeRelocation reloc{};
    reloc.m_address = 0;
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    X86_64RelocationResolver resolver;
    ASSERT_TRUE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x200, reloc.m_relocType));

    EXPECT_EQ(read32(bytes, 2), 0x200u - 6u);
}

// Verifies a PCRel32 relocation at a non-zero field computes the displacement from the field's end.
TEST_F(X86_64RelocationResolverTest, PatchesPcRelative32AtField)
{
    std::array<uint8_t, 16> bytes{};

    CodeRelocation reloc{};
    reloc.m_address = 2;
    reloc.m_relocType = TargetCodeRelocationType::PCRel32;

    X86_64RelocationResolver resolver;
    ASSERT_TRUE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x10, reloc.m_relocType));

    EXPECT_EQ(read32(bytes, 2), 0x10u - 6u);
}

// Verifies an unrecognized opcode or relocation type is rejected.
TEST_F(X86_64RelocationResolverTest, RejectsUnhandledTypesAndOpcodes)
{
    std::array<uint8_t, 16> bytes{};
    bytes[0] = 0x90; // NOP

    CodeRelocation reloc{};
    reloc.m_address = 0;
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    X86_64RelocationResolver resolver;
    EXPECT_FALSE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x10, reloc.m_relocType));
    EXPECT_FALSE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()),
                                reloc,
                                0x10,
                                TargetCodeRelocationType::Absolute32));
}

// Verifies a relocation that would write past the end of the buffer is rejected.
TEST_F(X86_64RelocationResolverTest, RejectsOutOfBoundsOffsets)
{
    std::array<uint8_t, 4> bytes{};
    bytes[3] = 0xE9;

    CodeRelocation reloc{};
    reloc.m_address = 3;
    reloc.m_relocType = TargetCodeRelocationType::BranchRel32;

    X86_64RelocationResolver resolver;
    EXPECT_FALSE(resolver.patch(std::span<uint8_t>(bytes.data(), bytes.size()), reloc, 0x10, reloc.m_relocType));
}
