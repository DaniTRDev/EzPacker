#include "FrontendCompilerTestFixture.h"

TEST_F(FrontendCompilerTestFixture, Driver_Compile_MultiFiles_Success)
{
    std::string fullPipeStress = readProgramFile("full_pipeline_stress.ez");
    ASSERT_FALSE(fullPipeStress.empty());

    std::string heavyArithmetic = readProgramFile("heavy_arithmetic.ez");
    ASSERT_FALSE(heavyArithmetic.empty());

    std::string crossRefs = readProgramFile("cross_refs.ez");
    ASSERT_FALSE(crossRefs.empty());

    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(fullPipeStress, "full_pipeline_stress.ez"));
    ASSERT_TRUE(driver->addSource(heavyArithmetic, "heavy_arithmetic.ez"));
    ASSERT_TRUE(driver->addSource(crossRefs, "cross_refs.ez"));

    EXPECT_TRUE(driver->compile());
}

TEST_F(FrontendCompilerTestFixture, Driver_Compile_MultiFiles_MethodNotIncluded)
{
    std::string fullPipeStress = readProgramFile("full_pipeline_stress.ez");
    ASSERT_FALSE(fullPipeStress.empty());

    std::string crossRefs = readProgramFile("cross_refs.ez");
    ASSERT_FALSE(crossRefs.empty());

    // Note: heavy_arithmetic.ez is not included, so the method calls in cross_refs.ez that reference it
    // should be ignored

    auto driver = createDriver();
    ASSERT_TRUE(driver->addSource(fullPipeStress, "full_pipeline_stress.ez"));
    ASSERT_TRUE(driver->addSource(crossRefs, "cross_refs.ez"));

    EXPECT_FALSE(driver->compile());
}
