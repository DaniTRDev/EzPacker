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

TEST_F(EzCompilerTestSuite, TestChibiccFreestandingMacros)
{
    ChibiccWrapper wrapper;
    const std::string standaloneCheck = R"c(
#if __STDC_HOSTED__ != 0
#error "Expected __STDC_HOSTED__ to be 0 in freestanding environment"
#endif

#if !defined(__EZPACKER_STANDALONE__) || __EZPACKER_STANDALONE__ != 1
#error "Expected __EZPACKER_STANDALONE__ to be defined as 1"
#endif

#if !defined(__STDC_FREESTANDING__) || __STDC_FREESTANDING__ != 1
#error "Expected __STDC_FREESTANDING__ to be defined as 1"
#endif

#if defined(linux) || defined(unix) || defined(__gnu_linux__)
#error "Hosted Linux/Unix macros must not be predefined"
#endif

int standalone_kernel_entry(void) {
    return 0xCAFE;
}
)c";

    auto result = wrapper.parseString(standaloneCheck);
    ASSERT_TRUE(result.success) << "Failed to parse freestanding macros: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *fn = findObject(result.astRoot, "standalone_kernel_entry");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->is_function);
    EXPECT_STREQ(fn->name, "standalone_kernel_entry");
}

TEST_F(EzCompilerTestSuite, TestChibiccFreestandingHeaders)
{
    ChibiccWrapper wrapper;
    const std::string source = R"c(
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include <stdalign.h>
#include <stdnoreturn.h>
#include <iso646.h>
#include <float.h>
#include <stdarg.h>

noreturn void system_halt(void) {
    while (1) {}
}

int freestanding_eval(int count, ...) {
    uint32_t u = UINT32_MAX;
    int32_t i = INT32_MIN;
    size_t s = sizeof(u);
    bool flag = true;
    alignas(8) int x = 42;
    float f = FLT_MAX;
    int char_bits = CHAR_BIT;

    if (flag and u != 0) {
        return (int)s + char_bits;
    }
    return 0;
}
)c";

    auto result = wrapper.parseString(source);
    ASSERT_TRUE(result.success) << "Failed to parse freestanding headers: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *evalFn = findObject(result.astRoot, "freestanding_eval");
    ASSERT_NE(evalFn, nullptr);
    EXPECT_TRUE(evalFn->is_function);
    EXPECT_STREQ(evalFn->name, "freestanding_eval");

    Obj *haltFn = findObject(result.astRoot, "system_halt");
    ASSERT_NE(haltFn, nullptr);
    EXPECT_TRUE(haltFn->is_function);
    EXPECT_STREQ(haltFn->name, "system_halt");
}


