#include "EzCompilerTestSuite.h"
#include "CFrontend/CFrontendAdapter.h"
#include "DriverContext.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "GlobalVar/MirGlobalVar.h"
#include "MirPasses/Passes/MirVerifierPass.h"
#include "Type/MirTypeTable.h"
#include <memory>

using namespace EzCompiler;

class CFrontendAdapterTest : public EzCompilerTestSuite
{
  protected:
    std::unique_ptr<DriverContext> createDriverContext()
    {
        CommandLineOptions options;
        options.target = TargetTriple::parse("x86_64-unknown-linux-gnu");
        options.optLevel = OptimizationLevel::O0;
        options.emissionStage = EmissionStage::GenericMir;

        auto ctx = std::make_unique<DriverContext>(options);
        EXPECT_TRUE(ctx->initialize());
        return ctx;
    }

    void verifyMir(MirBuilderContext *bCtx)
    {
        MirVerifierPass verifier(bCtx);
        for (auto it = bCtx->getFunctions().begin(); it != bCtx->getFunctions().end(); ++it)
        {
            verifier.run(it, nullptr);
            EXPECT_TRUE(verifier.getResult().isValid());
            EXPECT_EQ(verifier.getResult().m_errorCount, 0u);
        }
    }
};

TEST_F(CFrontendAdapterTest, CompilesSimpleReturn)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = "int main() { return 42; }";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "main.c");
    ASSERT_TRUE(success);

    // Verify MIR function exists
    ASSERT_EQ(bCtx->getFunctions().size(), 1u);
    MirFunction *func = bCtx->getFunctions().front();
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(std::string_view(func->getName()), "main");
    EXPECT_EQ(func->getReturnType()->getId(), ctx->getTypeTable()->i32()->getId());
    EXPECT_GT(func->getBlockCount(), 0u);

    // Run MIR Verifier Pass
    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, CompilesArithmeticAndLocals)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        int compute(int a, int b) {
            int x = a + b * 3;
            int y = x / 2 - 1;
            return y;
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "compute.c");
    ASSERT_TRUE(success);

    ASSERT_EQ(bCtx->getFunctions().size(), 1u);
    MirFunction *func = bCtx->getFunctions().front();
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(std::string_view(func->getName()), "compute");
    EXPECT_EQ(func->getParamCount(), 2u);

    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, CompilesIfElseAndLoops)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        int gcd(int a, int b) {
            while (b != 0) {
                int t = b;
                b = a % b;
                a = t;
            }
            return a;
        }

        int max(int a, int b) {
            if (a > b) {
                return a;
            } else {
                return b;
            }
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "control_flow.c");
    ASSERT_TRUE(success);

    EXPECT_EQ(bCtx->getFunctions().size(), 2u);
    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, CompilesGlobalVariablesAndCalls)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        int global_val = 123;

        int helper(int x) {
            return x + global_val;
        }

        int caller() {
            global_val = 456;
            return helper(10);
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "globals.c");
    ASSERT_TRUE(success);

    // Verify global variable registration
    bool foundGlobal = false;
    for (MirGlobalVar *gv : bCtx->getGlobalVars())
    {
        if (gv && std::string_view(gv->getName()) == "global_val")
        {
            foundGlobal = true;
            break;
        }
    }
    EXPECT_TRUE(foundGlobal);

    // Verify functions
    EXPECT_EQ(bCtx->getFunctions().size(), 2u);
    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, ReportsSyntaxErrorGracefully)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *badCode = "int invalid( { return ; "; // syntax error

    bool success = adapter.compileStringToMir(*ctx, badCode, *bCtx, "bad.c");
    EXPECT_FALSE(success);
}
