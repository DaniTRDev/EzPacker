#include <gtest/gtest.h>
#include "Decoder/Zydis/EzZydisDecoder.h"
#include "Architectures/x64.h" // Assume you mock IArchitecture
#include <vector>

/*
 * TESTS ARE MADE IN X64 ARCHITECTURE!
 */

TEST(EzZydisDecoderTests, InitializationFailsOnNullArchitecture)
{
    EzZydisDecoder decoder;
    EXPECT_FALSE(decoder.initialize(nullptr));
}


TEST(EzZydisDecoderTests, InitializationSucceedes)
{
    auto arch = std::make_shared<x64>();
    EXPECT_EQ(arch->getWordSize(), static_cast<size_t>(64));
    EXPECT_EQ(arch->getType(), ArchitectureType::x64);

    EzZydisDecoder decoder;
    EXPECT_TRUE(decoder.initialize(arch));
}

TEST(EzZydisDecoderTests, DecodeFailsOnNullBuffer)
{
    EzZydisDecoder decoder;
    size_t addr = 0;
    std::vector<std::shared_ptr<IDecodedOperand>> operands;
    auto result = decoder.decodeInstruction(nullptr, addr, 10, operands);
    EXPECT_EQ(result, nullptr);
}
