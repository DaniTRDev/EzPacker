#include "EzMirTestSuite.h"
#include "GlobalVar/MirGlobalVar.h"
#include "GlobalVar/MirGlobalVarBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

class GlobalVarTest : public MirTestSuiteAsGtest
{
  public:
  protected:
};

namespace
{

::testing::AssertionResult IsGlobalVar(MirGlobalVar *global,
                                       const std::string_view &expectedName,
                                       MirType *expectedType,
                                       bool expectedConstant,
                                       MirGlobalVarLinkage expectedLinkage)
{
    if (!global)
        return ::testing::AssertionFailure() << "Global variable is nullptr";

    if (global->getName() != expectedName)
        return ::testing::AssertionFailure()
                << "Expected name '" << expectedName << "', got '" << global->getName() << "'";

    if (global->getType() != expectedType)
        return ::testing::AssertionFailure() << "Expected type " << expectedType << ", got " << global->getType();

    if (global->isConstant() != expectedConstant)
        return ::testing::AssertionFailure()
                << "Expected constant=" << expectedConstant << ", got " << global->isConstant();

    if (global->getLinkage() != expectedLinkage)
        return ::testing::AssertionFailure() << "Expected linkage " << static_cast<int>(expectedLinkage) << ", got "
                                             << static_cast<int>(global->getLinkage());

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult IsZeroInitialized(MirGlobalVar *global)
{
    if (!global)
        return ::testing::AssertionFailure() << "Global variable is nullptr";

    if (global->getInitializer() != nullptr)
        return ::testing::AssertionFailure() << "Expected global variable '" << global->getName()
                                             << "' to be zero-initialized (nullptr initializer), but got one.";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult HasInitializer(MirGlobalVar *global, MirOperand *expectedInit)
{
    if (!global)
        return ::testing::AssertionFailure() << "Global variable is nullptr";
    if (!expectedInit)
        return ::testing::AssertionFailure() << "Expected initializer is nullptr (use IsZeroInitialized instead)";

    MirOperand *actualInit = global->getInitializer();
    if (!actualInit)
        return ::testing::AssertionFailure()
                << "Global variable '" << global->getName() << "' has no initializer, expected one.";

    if (actualInit->getType() != expectedInit->getType())
        return ::testing::AssertionFailure()
                << "Initializer type mismatch for global variable '" << global->getName() << "'";

    if (actualInit->getMirType() != expectedInit->getMirType())
        return ::testing::AssertionFailure()
                << "Initializer MIR type mismatch for global variable '" << global->getName() << "'";

    return ::testing::AssertionSuccess();
}

} // anonymous namespace

TEST_F(GlobalVarTest, TestDefaultGlobalVariable)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirGlobalVarBuilder builder(ctx);
    MirGlobalVar *global = builder.build(MirGlobalVarLinkage::External, types->i32(), "g_defaultVar");

    EXPECT_TRUE(IsGlobalVar(global,
                            "g_defaultVar",
                            types->i32(),
                            true /* Assumes true by default */,
                            MirGlobalVarLinkage::External));
    EXPECT_TRUE(IsZeroInitialized(global));
}

TEST_F(GlobalVarTest, TestMutableGlobalVariable)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirGlobalVarBuilder builder(ctx);
    builder.setConstant(false);

    MirGlobalVar *global = builder.build(MirGlobalVarLinkage::Internal, types->i8(), "g_mutableVar");

    EXPECT_TRUE(IsGlobalVar(global, "g_mutableVar", types->i8(), false, MirGlobalVarLinkage::Internal));
    EXPECT_TRUE(IsZeroInitialized(global));
}

TEST_F(GlobalVarTest, TestGlobalVariableWithInitializer)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();
    MirOperandBuilder opBuilder(ctx);

    // Imagine we are writing a 32-bit floating point value 1.0f (0x3F800000)
    MirFloat *_float = opBuilder.buildFloat(types->f32(), FlexFloat(1.f));

    MirGlobalVarBuilder builder(ctx);
    builder.setConstant(true).setInitializer(_float);

    MirGlobalVar *global = builder.build(MirGlobalVarLinkage::External, types->f32(), "g_floatConst");

    EXPECT_TRUE(IsGlobalVar(global, "g_floatConst", types->f32(), true, MirGlobalVarLinkage::External));
    EXPECT_TRUE(HasInitializer(global, _float));
}

TEST_F(GlobalVarTest, TestGlobalVariableWeakLinkage)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirGlobalVarBuilder builder(ctx);
    builder.setConstant(false);

    MirGlobalVar *global = builder.build(MirGlobalVarLinkage::Weak, types->i64(), "g_weakVar");

    EXPECT_TRUE(IsGlobalVar(global, "g_weakVar", types->i64(), false, MirGlobalVarLinkage::Weak));
    EXPECT_TRUE(IsZeroInitialized(global));
}