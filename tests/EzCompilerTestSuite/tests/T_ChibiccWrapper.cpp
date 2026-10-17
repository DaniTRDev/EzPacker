#include "EzCompilerTestSuite.h"
#include "CFrontend/ChibiccWrapper.h"
#include "chibicc.h"

using namespace EzCompiler::CFrontend;

namespace
{
Obj *findObject(Obj *root, std::string_view name)
{
    for (Obj *cur = root; cur; cur = cur->next)
    {
        if (cur->name && cur->name == name)
            return cur;
    }
    return nullptr;
}
} // namespace

TEST_F(EzCompilerTestSuite, TestChibiccParseSimpleFunction)
{
    ChibiccWrapper wrapper;
    const std::string source = "int main() { return 42; }\n";

    auto result = wrapper.parseString(source);
    ASSERT_TRUE(result.success) << "Parsing failed with error: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *fn = findObject(result.astRoot, "main");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->is_function);
    EXPECT_STREQ(fn->name, "main");
    ASSERT_NE(fn->body, nullptr);
    EXPECT_EQ(fn->body->kind, ND_BLOCK);
}

TEST_F(EzCompilerTestSuite, TestChibiccParseFunctionWithArgsAndArithmetic)
{
    ChibiccWrapper wrapper;
    const std::string source = R"c(
int add(int a, int b) {
    int sum = a + b;
    return sum;
}
)c";

    auto result = wrapper.parseString(source);
    ASSERT_TRUE(result.success) << "Parsing failed: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *fn = findObject(result.astRoot, "add");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->is_function);
    EXPECT_STREQ(fn->name, "add");

    // Verify params
    ASSERT_NE(fn->params, nullptr);
    EXPECT_STREQ(fn->params->name, "a");
    ASSERT_NE(fn->params->next, nullptr);
    EXPECT_STREQ(fn->params->next->name, "b");
}

TEST_F(EzCompilerTestSuite, TestChibiccParseGlobalVariable)
{
    ChibiccWrapper wrapper;
    const std::string source = "int g_answer = 42;\n";

    auto result = wrapper.parseString(source);
    ASSERT_TRUE(result.success) << "Parsing failed: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *global = findObject(result.astRoot, "g_answer");
    ASSERT_NE(global, nullptr);
    EXPECT_FALSE(global->is_function);
    EXPECT_STREQ(global->name, "g_answer");
    EXPECT_TRUE(global->is_definition);
}

TEST_F(EzCompilerTestSuite, TestChibiccErrorInterceptionDoesNotCrash)
{
    ChibiccWrapper wrapper;
    // Malformed C code that triggers a syntax error
    const std::string invalidSource = "int = ;";

    auto result = wrapper.parseString(invalidSource);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorMessage.empty());
}

TEST_F(EzCompilerTestSuite, TestChibiccConsecutiveParsesWithStateReset)
{
    ChibiccWrapper wrapper;

    auto res1 = wrapper.parseString("int f1() { return 1; }");
    ASSERT_TRUE(res1.success);
    Obj *fn1 = findObject(res1.astRoot, "f1");
    ASSERT_NE(fn1, nullptr);
    EXPECT_STREQ(fn1->name, "f1");

    auto res2 = wrapper.parseString("int f2() { return 2; }");
    ASSERT_TRUE(res2.success);
    Obj *fn2 = findObject(res2.astRoot, "f2");
    ASSERT_NE(fn2, nullptr);
    EXPECT_STREQ(fn2->name, "f2");
}
