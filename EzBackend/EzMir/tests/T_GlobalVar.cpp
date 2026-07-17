#include <gtest/gtest.h>
#include "EzMirTestSuite/EzMirTestSuite.h"

class GlobalVarTest : public MirTestSuiteAsGtest
{
  public:
  protected:
};

TEST_F(GlobalVarTest, TestDefaultGlobalVariable)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirGlobalVarBuilder builder(ctx);
    MirGlobalVar *global = builder.build(MirGlobalVarLinkage::External, types->i32(), "g_defaultVar");

    MirGlobalVarVerifier(global)
            .name("g_defaultVar")
            .type(types->i32())
            .constant(true) // Assumes true by default.
            .linkage(MirGlobalVarLinkage::External)
            .zeroInitialized();
}

TEST_F(GlobalVarTest, TestMutableGlobalVariable)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirGlobalVarBuilder builder(ctx);
    builder.setConstant(false);

    MirGlobalVar *global = builder.build(MirGlobalVarLinkage::Internal, types->i8(), "g_mutableVar");

    MirGlobalVarVerifier(global)
            .name("g_mutableVar")
            .type(types->i8())
            .constant(false)
            .linkage(MirGlobalVarLinkage::Internal)
            .zeroInitialized();
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

    MirGlobalVarVerifier(global)
            .name("g_floatConst")
            .type(types->f32())
            .constant(true)
            .linkage(MirGlobalVarLinkage::External)
            .initializer(_float);
}

TEST_F(GlobalVarTest, TestGlobalVariableWeakLinkage)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = getTypeTable();

    MirGlobalVarBuilder builder(ctx);
    builder.setConstant(false);

    MirGlobalVar *global = builder.build(MirGlobalVarLinkage::Weak, types->i64(), "g_weakVar");

    MirGlobalVarVerifier(global)
            .name("g_weakVar")
            .type(types->i64())
            .constant(false)
            .linkage(MirGlobalVarLinkage::Weak)
            .zeroInitialized();
}