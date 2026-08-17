#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

class FunctionTest : public MirTestSuiteAsGtest
{
  public:
  private:
};

namespace
{
/**
 * Checks the signature of a function.
 */
::testing::AssertionResult
FuncSignature(MirFunction *func, MirType *retType, size_t paramCount, size_t stackObjCount, std::string_view name)
{
    if (!func)
        return ::testing::AssertionFailure() << "Function is nullptr";

    if (func->getReturnType() != retType)
        return ::testing::AssertionFailure() << "Function return type does not match";

    if (func->getParameters().size() != paramCount)
        return ::testing::AssertionFailure() << "Function parameter count does not match";

    if (func->getStackFrame()->getAllocatedObjectCount() != stackObjCount)
        return ::testing::AssertionFailure() << "Function stack object does not match";

    if (func->getName() != name)
        return ::testing::AssertionFailure() << "Function name does not match";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult FuncParam(MirFunction *func, size_t index, MirType *type, std::string_view name)
{
    if (!func)
        return ::testing::AssertionFailure() << "Function is nullptr";

    if (func->getParamCount() <= index)
        return ::testing::AssertionFailure() << "Function has not a parameter at given index";

    auto it = func->getParameters().begin();
    std::advance(it, index);

    MirRegister *param = *it;

    if (param->getMirType() != type)
        return ::testing::AssertionFailure() << "The parameter MIR type does not match";

    if (param->getName() != name)
        return ::testing::AssertionFailure() << "The parameter name does not match";

    return ::testing::AssertionSuccess();
}

::testing::AssertionResult FuncStackFrameObj(MirFunction *func, size_t index, MirType *type, StackFrameObjectSource src)
{
    if (!func)
        return ::testing::AssertionFailure() << "Function is nullptr";

    MirFunctionStackFrame *stackFrame = func->getStackFrame();

    if (stackFrame->getAllocatedObjectCount() <= index)
        return ::testing::AssertionFailure() << "Function has not a stack object at given index";

    const auto &objects = stackFrame->getObjects();
    StackFrameObject *obj = objects[index];

    if (obj->m_type != type)
        return ::testing::AssertionFailure() << "The stack object's MIR type does not match";

    if (obj->m_source != src)
        return ::testing::AssertionFailure() << "The stack object's source does not match";

    return ::testing::AssertionSuccess();
}
} // anonymous namespace

TEST_F(FunctionTest, TestFuncNoParameters)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunction *func = getTestFunc();

    // The default test function is named "TEST" and returns void
    EXPECT_TRUE(FuncSignature(func, ctx->getTypeTable()->getVoidType(), 0, 0, "TEST"));
}

TEST_F(FunctionTest, TestFunc1Parameter)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = ctx->getTypeTable();
    MirFunctionBuilder builder(ctx);

    builder.buildParam(types->i8(), "testParam");
    MirFunction *func = builder.build(types->i8(), "myFunc");

    EXPECT_TRUE(FuncSignature(func, types->i8(), 1, 0, "myFunc"));
    EXPECT_TRUE(FuncParam(func, 0, types->i8(), "testParam"));
}

TEST_F(FunctionTest, TestFuncNParameters)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = ctx->getTypeTable();
    MirFunctionBuilder builder(ctx);

    builder.buildParam(types->i8(), "testParam");
    builder.buildParam(types->i16(), "testParam2");
    builder.buildParam(types->i32(), "testParam3");
    builder.buildParam(types->i64(), "testParam4");

    MirFunction *func = builder.build(types->i8(), "myFunc");

    EXPECT_TRUE(FuncSignature(func, types->i8(), 4, 0, "myFunc"));
    EXPECT_TRUE(FuncParam(func, 0, types->i8(), "testParam"));
    EXPECT_TRUE(FuncParam(func, 1, types->i16(), "testParam2"));
    EXPECT_TRUE(FuncParam(func, 2, types->i32(), "testParam3"));
    EXPECT_TRUE(FuncParam(func, 3, types->i64(), "testParam4"));
}

TEST_F(FunctionTest, TestFunc1LocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirFunction *func = builder.build(i8, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStaticStackObj(i8);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 1, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
}

TEST_F(FunctionTest, TestFuncNLocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirType *i16 = ctx->getTypeTable()->i16();

    MirFunction *func = builder.build(i8, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStaticStackObj(i8);
    StackFrameObject *obj2 = func->getStackFrame()->createStaticStackObj(i16);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 2, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 1, i16, obj2->m_source));
}

TEST_F(FunctionTest, TestFunc1Spill1StackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirFunction *func = builder.build(i8, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStackSpill(i8);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 1, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
}

TEST_F(FunctionTest, TestFunc1SpillNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirType *i16 = ctx->getTypeTable()->i16();

    MirFunction *func = builder.build(i8, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStackSpill(i8);
    StackFrameObject *obj2 = func->getStackFrame()->createStackSpill(i16);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 2, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 1, i16, obj2->m_source));
}

TEST_F(FunctionTest, TestFunc1ParameterNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirType *i16 = ctx->getTypeTable()->i16();

    MirFunction *func = builder.build(i8, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStackParam(i8);
    StackFrameObject *obj2 = func->getStackFrame()->createStackParam(i16);
    StackFrameObject *obj3 = func->getStackFrame()->createStackParam(i16);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 3, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 1, i16, obj2->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 2, i16, obj3->m_source));
}