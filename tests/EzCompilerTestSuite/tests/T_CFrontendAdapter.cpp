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

TEST_F(CFrontendAdapterTest, CompilesStandaloneKernelWithoutStandardLibrary)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        // Freestanding standalone device driver routine
        struct UartDriver {
            unsigned int rx_fifo;
            unsigned int tx_fifo;
            unsigned int status;
            unsigned int control;
        };

        static unsigned int s_tx_counter = 0;

        int uart_transmit_byte(struct UartDriver *dev, unsigned int byte_val) {
            if (!dev)
                return -1;
            while ((dev->status & 0x01) == 0) {
                // busy wait
            }
            dev->tx_fifo = byte_val & 0xFF;
            s_tx_counter = s_tx_counter + 1;
            return 0;
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "uart_driver.c");
    ASSERT_TRUE(success);

    // Verify MIR function generated
    ASSERT_EQ(bCtx->getFunctions().size(), 1u);
    MirFunction *func = bCtx->getFunctions().front();
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(std::string_view(func->getName()), "uart_transmit_byte");
    EXPECT_EQ(func->getParamCount(), 2u);

    // Verify static global counter variable
    bool foundCounter = false;
    for (MirGlobalVar *gv : bCtx->getGlobalVars())
    {
        if (gv && std::string_view(gv->getName()) == "s_tx_counter")
        {
            foundCounter = true;
            EXPECT_EQ(gv->getLinkage(), MirLinkage::Internal);
            break;
        }
    }
    EXPECT_TRUE(foundCounter);

    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, CompilesFreestandingKernelWithStdintAndStddef)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        #include <stdint.h>
        #include <stddef.h>
        #include <stdbool.h>

        struct PacketHeader {
            uint32_t magic;
            uint16_t length;
            uint16_t sequence;
        };

        static uint32_t s_packet_count = 0;

        bool process_packet(struct PacketHeader *pkt) {
            if (pkt == NULL)
                return false;

            if (pkt->magic == 0xDEADBEEF) {
                s_packet_count = s_packet_count + 1;
                return true;
            }
            return false;
        }

        uint32_t get_total_packets(void) {
            return s_packet_count;
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "kernel_net.c");
    ASSERT_TRUE(success);

    // Verify functions
    EXPECT_EQ(bCtx->getFunctions().size(), 2u);

    bool foundProcess = false;
    bool foundGetTotal = false;
    for (MirFunction *fn : bCtx->getFunctions())
    {
        if (fn && std::string_view(fn->getName()) == "process_packet")
            foundProcess = true;
        if (fn && std::string_view(fn->getName()) == "get_total_packets")
            foundGetTotal = true;
    }
    EXPECT_TRUE(foundProcess);
    EXPECT_TRUE(foundGetTotal);

    // Verify global variable s_packet_count
    bool foundGlobal = false;
    for (MirGlobalVar *gv : bCtx->getGlobalVars())
    {
        if (gv && std::string_view(gv->getName()) == "s_packet_count")
        {
            foundGlobal = true;
            EXPECT_EQ(gv->getLinkage(), MirLinkage::Internal);
            break;
        }
    }
    EXPECT_TRUE(foundGlobal);

    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, CompilesSwitchCaseWithDefaultAndRanges)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        int test_switch(int x) {
            int res = 0;
            switch (x) {
                case 1:
                    res = 10;
                    break;
                case 2 ... 5:
                    res = 20;
                    break;
                case 10:
                    res = 30;
                default:
                    res = res + 5;
                    break;
            }
            return res;
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "switch.c");
    ASSERT_TRUE(success);

    ASSERT_EQ(bCtx->getFunctions().size(), 1u);
    MirFunction *func = bCtx->getFunctions().front();
    ASSERT_NE(func, nullptr);
    EXPECT_GT(func->getBlockCount(), 4u);

    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, CompilesGotoAndLabels)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        int test_goto(int n) {
            int sum = 0;
            int i = 0;
        loop_start:
            if (i >= n)
                goto loop_end;
            sum = sum + i;
            i = i + 1;
            goto loop_start;
        loop_end:
            return sum;
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "goto.c");
    ASSERT_TRUE(success);

    ASSERT_EQ(bCtx->getFunctions().size(), 1u);
    MirFunction *func = bCtx->getFunctions().front();
    ASSERT_NE(func, nullptr);

    verifyMir(bCtx);
}

TEST_F(CFrontendAdapterTest, CompilesLabelsAsValuesAndComputedGoto)
{
    auto ctx = createDriverContext();
    auto *bCtx = ctx->getBuilderContext();

    CFrontendAdapter adapter;
    const char *code = R"c(
        int test_computed_goto(int op) {
            void *dispatch_table[2];
            dispatch_table[0] = &&do_add;
            dispatch_table[1] = &&do_sub;
            int val = 10;
            if (op == 0)
                goto *dispatch_table[0];
            else
                goto *dispatch_table[1];

        do_add:
            val = val + 5;
            return val;

        do_sub:
            val = val - 5;
            return val;
        }
    )c";

    bool success = adapter.compileStringToMir(*ctx, code, *bCtx, "computed_goto.c");
    ASSERT_TRUE(success);

    ASSERT_EQ(bCtx->getFunctions().size(), 1u);
    MirFunction *func = bCtx->getFunctions().front();
    ASSERT_NE(func, nullptr);

    verifyMir(bCtx);
}


