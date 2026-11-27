#include "EzCompilerTestSuite.h"
#include "CFrontend/ChibiccWrapper.h"
#include "chibicc.h"
#include <filesystem>

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
TEST_F(EzCompilerTestSuite, TestChibiccHostedMacros)
{
    ChibiccWrapper wrapper;
    wrapper.setHostedMode(true);
    const std::string hostedCheck = R"c(
#if !defined(__STDC_HOSTED__) || __STDC_HOSTED__ != 1
#error "Expected __STDC_HOSTED__ to be 1 in hosted environment"
#endif

#if !defined(__EZPACKER_HOSTED__) || __EZPACKER_HOSTED__ != 1
#error "Expected __EZPACKER_HOSTED__ to be defined as 1"
#endif

#if defined(__STDC_FREESTANDING__) || defined(__EZPACKER_STANDALONE__)
#error "Freestanding macros must not be defined in hosted mode"
#endif

int hosted_main(void) {
    return 0;
}
)c";

    auto result = wrapper.parseString(hostedCheck);
    ASSERT_TRUE(result.success) << "Failed to parse hosted macros: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *fn = findObject(result.astRoot, "hosted_main");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->is_function);
    EXPECT_STREQ(fn->name, "hosted_main");
}

TEST_F(EzCompilerTestSuite, TestChibiccGnuAttributes)
{
    ChibiccWrapper wrapper;
    const std::string source = R"c(
typedef int __attribute__((aligned(8))) aligned_int;
__attribute__((visibility("default"))) int g_public_var = 100;
__attribute__((nothrow, nonnull(1))) int helper_fn(int *ptr) {
    return *ptr;
}
)c";

    auto result = wrapper.parseString(source);
    ASSERT_TRUE(result.success) << "Failed to parse GNU attributes: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *var = findObject(result.astRoot, "g_public_var");
    ASSERT_NE(var, nullptr);
    EXPECT_FALSE(var->is_function);

    Obj *fn = findObject(result.astRoot, "helper_fn");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->is_function);
}

TEST_F(EzCompilerTestSuite, TestChibiccHostedHeaders)
{
    ChibiccWrapper wrapper;
    wrapper.setHostedMode(true);
    const std::string source = R"c(
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <math.h>

int hosted_demo(void) {
    char buf[64];
    size_t len = strlen("EzPacker C11");
    int c = toupper('a');
    assert(len > 0);
    return (int)len + c;
}
)c";

    auto result = wrapper.parseString(source);
    ASSERT_TRUE(result.success) << "Failed to parse hosted headers: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);

    Obj *fn = findObject(result.astRoot, "hosted_demo");
    ASSERT_NE(fn, nullptr);
    EXPECT_TRUE(fn->is_function);
    EXPECT_STREQ(fn->name, "hosted_demo");
}


TEST_F(EzCompilerTestSuite, TestChibiccBundledIncludeDirsResolved)
{
    std::string freestandingDir = ChibiccWrapper::getBundledFreestandingIncludeDir();
    std::string muslDir = ChibiccWrapper::getBundledHostedIncludeDir();
    std::string muslCompatDir = ChibiccWrapper::getBundledHostedCompatIncludeDir();

    ASSERT_FALSE(freestandingDir.empty());
    ASSERT_FALSE(muslDir.empty());
    ASSERT_FALSE(muslCompatDir.empty());

    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(freestandingDir) / "stdint.h"));
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(muslDir) / "stdio.h"));
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(muslDir) / "bits" / "alltypes.h"));
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(muslCompatDir) / "stdarg.h"));
}

TEST_F(EzCompilerTestSuite, TestChibiccTargetMacros)
{
    // Windows hosted macros
    {
        ChibiccWrapper wrapper;
        wrapper.setTargetTriple(EzCompiler::TargetTriple::parse("x86_64-pc-windows-msvc"));
        wrapper.setHostedMode(true);
        const std::string src = R"c(
#if !defined(_WIN32) || !defined(_WIN64)
#error "Expected _WIN32 and _WIN64 in Windows hosted mode"
#endif
#if defined(__ELF__) || defined(__linux__)
#error "ELF/Linux macros must not be defined for Windows target"
#endif
int win_fn(void) { return 1; }
)c";
        auto res = wrapper.parseString(src);
        ASSERT_TRUE(res.success) << res.errorMessage;
        EXPECT_NE(findObject(res.astRoot, "win_fn"), nullptr);
    }

    // Linux x86_64 hosted macros
    {
        ChibiccWrapper wrapper;
        wrapper.setTargetTriple(EzCompiler::TargetTriple::parse("x86_64-unknown-linux-gnu"));
        wrapper.setHostedMode(true);
        const std::string src = R"c(
#if !defined(__ELF__) || !defined(__linux__) || !defined(__gnu_linux__)
#error "Expected Linux ELF macros in Linux hosted mode"
#endif
#if !defined(_LP64) || !defined(__x86_64__)
#error "Expected LP64 and x86_64 macros"
#endif
int lin_fn(void) { return 2; }
)c";
        auto res = wrapper.parseString(src);
        ASSERT_TRUE(res.success) << res.errorMessage;
        EXPECT_NE(findObject(res.astRoot, "lin_fn"), nullptr);
    }

    // RISC-V64 hosted macros
    {
        ChibiccWrapper wrapper;
        wrapper.setTargetTriple(EzCompiler::TargetTriple::parse("riscv64-unknown-linux-gnu"));
        wrapper.setHostedMode(true);
        const std::string src = R"c(
#if !defined(__riscv) || __riscv_xlen != 64
#error "Expected RISC-V 64 macros"
#endif
#if defined(__x86_64__)
#error "x86_64 macro must not be defined for RISC-V target"
#endif
int rv_fn(void) { return 3; }
)c";
        auto res = wrapper.parseString(src);
        ASSERT_TRUE(res.success) << res.errorMessage;
        EXPECT_NE(findObject(res.astRoot, "rv_fn"), nullptr);
    }
}

TEST_F(EzCompilerTestSuite, TestChibiccMuslHeadersComprehensive)
{
    ChibiccWrapper wrapper;
    wrapper.setTargetTriple(EzCompiler::TargetTriple::parse("x86_64-unknown-linux-gnu"));
    wrapper.setHostedMode(true);
    const std::string source = R"c(
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

int musl_all_headers_eval(int argc, char **argv) {
    pid_t pid = getpid();
    int64_t val = INT64_C(42);
    char buf[32];
    snprintf(buf, sizeof(buf), "%" PRId64, val);
    return argc + (int)pid + (argv != NULL);
}
)c";

    auto result = wrapper.parseString(source);
    ASSERT_TRUE(result.success) << "Failed to parse comprehensive musl headers: " << result.errorMessage;
    ASSERT_NE(result.astRoot, nullptr);
    EXPECT_NE(findObject(result.astRoot, "musl_all_headers_eval"), nullptr);
}
