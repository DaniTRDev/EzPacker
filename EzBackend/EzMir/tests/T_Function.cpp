#include <gtest/gtest.h>
#include "MirTestSuite.h"

class FunctionTest : public MirTestSuiteAsGtest
{
  public:
  private:
};

TEST_F(FunctionTest, TestFuncNoParameters)
{
    MirFunction *func = getTestFunc();
    MirFunctionVerifier verifier(func);
    verifier.paramCount(0).stackFrameVerifier().stackFrameObjCount(0);
}

TEST_F(FunctionTest, TestFunc1Parameter)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);
    builder.buildParam(getTypeTable()->i8(), nullptr, "testParam");

    MirFunction *func = builder.build(ctx->getTypeTable()->i8(), nullptr, {}, "myFunc");

    MirFunctionVerifier verifier(func);
    verifier.paramCount(1).stackFrameVerifier().stackFrameObjCount(0);
    verifier.paramType({ ctx->getTypeTable()->i8()->getId() });
}

TEST_F(FunctionTest, TestFuncNParameters)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);
    builder.buildParam(getTypeTable()->i8(), nullptr, "testParam");
    builder.buildParam(getTypeTable()->i16(), nullptr, "testParam2");
    builder.buildParam(getTypeTable()->i32(), nullptr, "testParam3");
    builder.buildParam(getTypeTable()->i64(), nullptr, "testParam4");

    MirFunction *func = builder.build(ctx->getTypeTable()->i8(), nullptr, {}, "myFunc");

    MirFunctionVerifier verifier(func);
    verifier.paramCount(4).stackFrameVerifier().stackFrameObjCount(0);
    verifier.paramType({ ctx->getTypeTable()->i8()->getId(),
                         ctx->getTypeTable()->i16()->getId(),
                         ctx->getTypeTable()->i32()->getId(),
                         ctx->getTypeTable()->i64()->getId() });
}

TEST_F(FunctionTest, TestFunc1LocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    size_t i8Size = i8->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildLocalStackObj(i8Size, i8Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(1);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
}

TEST_F(FunctionTest, TestFuncNLocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8(), *i16 = ctx->getTypeTable()->i16();
    size_t i8Size = i8->getTotalSizeInBytes(), i16Size = i16->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildLocalStackObj(i8Size, i8Size),
                     *obj2 = builder.buildLocalStackObj(i16Size, i16Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(2);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
    stackFrameVerifier.checkStackFrameObj(obj2->m_id,
                                          obj2->m_offset,
                                          obj2->m_align,
                                          obj2->m_sizeInBytes,
                                          obj2->m_source);
}

TEST_F(FunctionTest, TestFunc1Spill1StackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    size_t i8Size = i8->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildStackSpill(i8Size, i8Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(1);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
}

TEST_F(FunctionTest, TestFunc1SpillNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8(), *i16 = ctx->getTypeTable()->i16();
    size_t i8Size = i8->getTotalSizeInBytes(), i16Size = i16->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildStackSpill(i8Size, i8Size), *obj2 = builder.buildStackSpill(i16Size, i16Size);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(2);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
    stackFrameVerifier.checkStackFrameObj(obj2->m_id,
                                          obj2->m_offset,
                                          obj2->m_align,
                                          obj2->m_sizeInBytes,
                                          obj2->m_source);
}

TEST_F(FunctionTest, TestFunc1ParameterNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirOperandBuilder operandBuilder(ctx);

    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8(), *i16 = ctx->getTypeTable()->i16();
    size_t i8Size = i8->getTotalSizeInBytes(), i16Size = i16->getTotalSizeInBytes();

    MirFunction *func = builder.build(i8, nullptr, {}, "myFunc");
    StackFrameObject *obj = builder.buildStackParam(i8Size, i8Size, 0),
                     *obj2 = builder.buildStackParam(i16Size, i16Size, 2),
                     *obj3 = builder.buildStackParam(i16Size, i16Size, 4);

    MirFunctionVerifier verifier(func);
    MirFunctionStackFrameVerifier stackFrameVerifier = verifier.stackFrameVerifier();

    stackFrameVerifier.stackFrameObjCount(3);
    stackFrameVerifier.checkStackFrameObj(obj->m_id, obj->m_offset, obj->m_align, obj->m_sizeInBytes, obj->m_source);
    stackFrameVerifier.checkStackFrameObj(obj2->m_id,
                                          obj2->m_offset,
                                          obj2->m_align,
                                          obj2->m_sizeInBytes,
                                          obj2->m_source);
    stackFrameVerifier.checkStackFrameObj(obj3->m_id,
                                          obj3->m_offset,
                                          obj3->m_align,
                                          obj3->m_sizeInBytes,
                                          obj3->m_source);
}