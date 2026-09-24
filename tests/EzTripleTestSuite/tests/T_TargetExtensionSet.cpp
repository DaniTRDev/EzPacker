#include "EzTripleTestSuite.h"
#include "Descriptors/TargetExtensionSet.h"
#include "X86_64TargetDesc.h"

using namespace EzTargets::X86_64;

TEST_F(EzTripleTestSuite, TestTargetExtensionSetBasics)
{
    TargetExtensionSet set;

    set.registerExtension("sse", "Streaming SIMD Extensions", true);
    set.registerExtension("sse2", "Streaming SIMD Extensions 2", true, { "sse" });
    set.registerExtension("avx", "Advanced Vector Extensions", false, { "sse2" });
    set.registerExtension("avx2", "Advanced Vector Extensions 2", false, { "avx" });

    EXPECT_TRUE(set.isSupported("sse"));
    EXPECT_TRUE(set.isSupported("SSE2")); // Case-insensitive
    EXPECT_TRUE(set.isSupported("avx"));
    EXPECT_TRUE(set.isSupported("AVX2"));
    EXPECT_FALSE(set.isSupported("neon"));

    // Check default baseline activation
    EXPECT_TRUE(set.has("sse"));
    EXPECT_TRUE(set.has("sse2"));
    EXPECT_FALSE(set.has("avx"));
    EXPECT_FALSE(set.has("avx2"));
}

TEST_F(EzTripleTestSuite, TestTargetExtensionSetImplications)
{
    TargetExtensionSet set;

    set.registerExtension("sse", "Streaming SIMD Extensions", true);
    set.registerExtension("sse2", "Streaming SIMD Extensions 2", true, { "sse" });
    set.registerExtension("avx", "Advanced Vector Extensions", false, { "sse2" });
    set.registerExtension("avx2", "Advanced Vector Extensions 2", false, { "avx" });

    // Enabling AVX2 should recursively enable AVX, SSE2, SSE
    EXPECT_TRUE(set.enable("avx2"));
    EXPECT_TRUE(set.has("avx2"));
    EXPECT_TRUE(set.has("avx"));
    EXPECT_TRUE(set.has("sse2"));
    EXPECT_TRUE(set.has("sse"));

    // Disabling SSE2 should recursively disable AVX and AVX2
    EXPECT_TRUE(set.disable("sse2"));
    EXPECT_FALSE(set.has("sse2"));
    EXPECT_FALSE(set.has("avx"));
    EXPECT_FALSE(set.has("avx2"));
    // SSE should remain enabled unless explicitly disabled
    EXPECT_TRUE(set.has("sse"));
}

TEST_F(EzTripleTestSuite, TestTargetExtensionSetModifiers)
{
    TargetExtensionSet set;

    set.registerExtension("sse", "Streaming SIMD Extensions", true);
    set.registerExtension("sse2", "Streaming SIMD Extensions 2", true, { "sse" });
    set.registerExtension("avx", "Advanced Vector Extensions", false, { "sse2" });

    std::string err;
    EXPECT_TRUE(set.applyModifier("+avx", &err));
    EXPECT_TRUE(set.has("avx"));

    EXPECT_TRUE(set.applyModifier("-avx", &err));
    EXPECT_FALSE(set.has("avx"));

    // Unknown modifier
    EXPECT_FALSE(set.applyModifier("+neon", &err));
    EXPECT_FALSE(err.empty());

    // Feature string
    err.clear();
    EXPECT_TRUE(set.applyFeatureString("+avx,-sse2", &err));
    EXPECT_FALSE(set.has("sse2"));
    EXPECT_FALSE(set.has("avx")); // -sse2 cascaded to avx
}

TEST_F(EzTripleTestSuite, TestX86_64TargetDescExtensions)
{
    X86_64TargetDesc target(getBuilderCtx());
    target.initialize();

    // Default baseline
    EXPECT_TRUE(target.hasExtension("sse"));
    EXPECT_TRUE(target.hasExtension("SSE2"));
    EXPECT_FALSE(target.hasExtension("avx"));
    EXPECT_FALSE(target.hasExtension("avx2"));

    // Enable AVX
    target.applyFeatures({ "+avx" });
    EXPECT_TRUE(target.hasExtension("avx"));
    EXPECT_TRUE(target.hasExtension("sse2"));

    // Disable SSE
    target.applyFeatures({ "-sse" });
    EXPECT_FALSE(target.hasExtension("sse"));
    EXPECT_FALSE(target.hasExtension("sse2"));
    EXPECT_FALSE(target.hasExtension("avx"));
}
