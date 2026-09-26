#include "EzMirTestSuite.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

/**
 * Test fixture for MIR Function creation, parameter configuration, and stack frame object management.
 */
class FunctionTest : public MirTestSuiteAsGtest
{
  public:
  private:
};

namespace
{
/**
 * Custom GoogleTest assertion verifying complete function signature attributes:
 * return type, parameter count, allocated stack frame object count, and function name.
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

/**
 * Custom GoogleTest assertion verifying parameter type and identifier at a specific zero-based parameter index.
 */
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

/**
 * Custom GoogleTest assertion verifying stack frame object properties (type and allocation source) at a specific index.
 */
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

/**
 * Verifies that the default test function is constructed with void return type, 0 parameters,
 * and 0 stack objects under the name "TEST".
 */
TEST_F(FunctionTest, TestFuncNoParameters)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunction *func = getTestFunc();

    // The default test function is named "TEST" and returns void
    EXPECT_TRUE(FuncSignature(func, ctx->getTypeTable()->_void(), 0, 0, "TEST"));
}

/**
 * Verifies function builder constructing a function with a single i8 parameter and matching return type.
 */
TEST_F(FunctionTest, TestFunc1Parameter)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = ctx->getTypeTable();
    MirFunctionBuilder builder(ctx);
    MirOperandBuilder oBuilder(ctx);

    MirFunction *func = builder.build(types->i8(), { oBuilder.buildVReg(types->i8(), "testParam") }, "myFunc");

    EXPECT_TRUE(FuncSignature(func, types->i8(), 1, 0, "myFunc"));
    EXPECT_TRUE(FuncParam(func, 0, types->i8(), "testParam"));
}

/**
 * Verifies function builder handling multiple heterogeneous parameters (i8, i16, i32, i64).
 */
TEST_F(FunctionTest, TestFuncNParameters)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirTypeTable *types = ctx->getTypeTable();
    MirFunctionBuilder builder(ctx);
    MirOperandBuilder oBuilder(ctx);

    MirFunction *func = builder.build(types->i8(),
                                      { oBuilder.buildVReg(types->i8(), "testParam"),
                                        oBuilder.buildVReg(types->i16(), "testParam2"),
                                        oBuilder.buildVReg(types->i32(), "testParam3"),
                                        oBuilder.buildVReg(types->i64(), "testParam4") },
                                      "myFunc");

    EXPECT_TRUE(FuncSignature(func, types->i8(), 4, 0, "myFunc"));
    EXPECT_TRUE(FuncParam(func, 0, types->i8(), "testParam"));
    EXPECT_TRUE(FuncParam(func, 1, types->i16(), "testParam2"));
    EXPECT_TRUE(FuncParam(func, 2, types->i32(), "testParam3"));
    EXPECT_TRUE(FuncParam(func, 3, types->i64(), "testParam4"));
}

/**
 * Verifies allocating a single static local variable on the function's stack frame.
 */
TEST_F(FunctionTest, TestFunc1LocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirFunction *func = builder.build(i8, {}, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStaticStackObj(i8);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 1, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
}

/**
 * Verifies allocating multiple static local variables of differing types on the stack frame.
 */
TEST_F(FunctionTest, TestFuncNLocalStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirType *i16 = ctx->getTypeTable()->i16();

    MirFunction *func = builder.build(i8, {}, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStaticStackObj(i8);
    StackFrameObject *obj2 = func->getStackFrame()->createStaticStackObj(i16);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 2, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 1, i16, obj2->m_source));
}

/**
 * Verifies allocating a register spill slot on the function's stack frame.
 */
TEST_F(FunctionTest, TestFunc1Spill1StackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirFunction *func = builder.build(i8, {}, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStackSpill(i8);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 1, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
}

/**
 * Verifies allocating multiple register spill slots on the stack frame.
 */
TEST_F(FunctionTest, TestFunc1SpillNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirType *i16 = ctx->getTypeTable()->i16();

    MirFunction *func = builder.build(i8, {}, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStackSpill(i8);
    StackFrameObject *obj2 = func->getStackFrame()->createStackSpill(i16);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 2, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 1, i16, obj2->m_source));
}

/**
 * Verifies allocating stack space reserved for passed parameters.
 */
TEST_F(FunctionTest, TestFunc1ParameterNStackObj)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);

    MirType *i8 = ctx->getTypeTable()->i8();
    MirType *i16 = ctx->getTypeTable()->i16();

    MirFunction *func = builder.build(i8, {}, "myFunc");
    StackFrameObject *obj = func->getStackFrame()->createStackParam(i8);
    StackFrameObject *obj2 = func->getStackFrame()->createStackParam(i16);
    StackFrameObject *obj3 = func->getStackFrame()->createStackParam(i16);

    EXPECT_TRUE(FuncSignature(func, i8, 0, 3, "myFunc"));
    EXPECT_TRUE(FuncStackFrameObj(func, 0, i8, obj->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 1, i16, obj2->m_source));
    EXPECT_TRUE(FuncStackFrameObj(func, 2, i16, obj3->m_source));
}

/**
 * Verifies function linkage options (External, Internal, Weak) and linkage modification.
 */
TEST_F(FunctionTest, TestFunctionLinkage)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);
    MirType *i32 = ctx->getTypeTable()->i32();

    // Default linkage is External
    MirFunction *defaultFunc = builder.build(i32, {}, "defaultFunc");
    EXPECT_EQ(defaultFunc->getLinkage(), MirLinkage::External);

    // Explicit Internal linkage
    MirFunction *internalFunc = builder.build(i32, {}, "internalFunc", MirLinkage::Internal);
    EXPECT_EQ(internalFunc->getLinkage(), MirLinkage::Internal);

    // Explicit Weak linkage
    MirFunction *weakFunc = builder.build(i32, {}, "weakFunc", MirLinkage::Weak);
    EXPECT_EQ(weakFunc->getLinkage(), MirLinkage::Weak);

    // Mutating linkage via setLinkage
    defaultFunc->setLinkage(MirLinkage::Internal);
    EXPECT_EQ(defaultFunc->getLinkage(), MirLinkage::Internal);
    defaultFunc->setLinkage(MirLinkage::Weak);
    EXPECT_EQ(defaultFunc->getLinkage(), MirLinkage::Weak);
    defaultFunc->setLinkage(MirLinkage::External);
    EXPECT_EQ(defaultFunc->getLinkage(), MirLinkage::External);
}

/**
 * Verifies declaring an extern function prototype (no basic blocks, isDeclaration() == true).
 */
TEST_F(FunctionTest, TestFunctionDeclaration)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);
    MirOperandBuilder opBuilder(ctx);
    MirTypeTable *types = ctx->getTypeTable();
    MirType *i32 = types->i32();
    MirType *ptr = types->getPtr(types->_void());

    MirRegister *arg0 = opBuilder.buildVReg(ptr, "arg0");
    MirFunction *declFunc = builder.declare(i32, { arg0 }, "puts", MirLinkage::External);

    ASSERT_NE(declFunc, nullptr);
    EXPECT_EQ(declFunc->getName(), "puts");
    EXPECT_EQ(declFunc->getReturnType(), i32);
    EXPECT_EQ(declFunc->getLinkage(), MirLinkage::External);
    EXPECT_TRUE(declFunc->isDeclaration());
    EXPECT_FALSE(declFunc->isDefinition());
    EXPECT_EQ(declFunc->getBlockCount(), 0);
    EXPECT_EQ(declFunc->getParamCount(), 1);
    EXPECT_TRUE(FuncParam(declFunc, 0, ptr, "arg0"));
}

/**
 * Verifies declaring an extern function prototype from type signatures alone.
 */
TEST_F(FunctionTest, TestFunctionDeclareWithTypes)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);
    MirTypeTable *types = ctx->getTypeTable();
    MirType *i32 = types->i32();
    MirType *i64 = types->i64();
    MirType *ptr = types->getPtr(types->_void());

    MirFunction *declFunc = builder.declare(i32, { i64, ptr }, "read_bytes", MirLinkage::Weak);

    ASSERT_NE(declFunc, nullptr);
    EXPECT_EQ(declFunc->getName(), "read_bytes");
    EXPECT_EQ(declFunc->getReturnType(), i32);
    EXPECT_EQ(declFunc->getLinkage(), MirLinkage::Weak);
    EXPECT_TRUE(declFunc->isDeclaration());
    EXPECT_FALSE(declFunc->isDefinition());
    EXPECT_EQ(declFunc->getBlockCount(), 0);
    EXPECT_EQ(declFunc->getParamCount(), 2);

    auto it = declFunc->getParameters().begin();
    EXPECT_EQ((*it)->getMirType(), i64);
    std::advance(it, 1);
    EXPECT_EQ((*it)->getMirType(), ptr);
}

/**
 * Verifies that definitions created via builder.build(...) have entry block and isDefinition() == true.
 */
TEST_F(FunctionTest, TestFunctionDefinitionSemantics)
{
    MirBuilderContext *ctx = getBuilderCtx();
    MirFunctionBuilder builder(ctx);
    MirType *voidType = ctx->getTypeTable()->_void();

    MirFunction *defFunc = builder.build(voidType, {}, "defined_function");

    ASSERT_NE(defFunc, nullptr);
    EXPECT_FALSE(defFunc->isDeclaration());
    EXPECT_TRUE(defFunc->isDefinition());
    EXPECT_EQ(defFunc->getBlockCount(), 1);
    EXPECT_NE(defFunc->getEntryPoint(), nullptr);
}