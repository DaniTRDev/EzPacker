#include "FrontendAdapter.h"
#include "DriverContext.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "Parser/MirParser.h"
#include <fstream>

namespace EzCompiler
{

bool MirModuleLoader::compileSourceToMir(DriverContext &ctx, std::string_view sourcePath, MirBuilderContext &outMirCtx)
{
    if (sourcePath.empty())
    {
        // Default standalone synthetic program for driver verification
        createReturnConstFunction(ctx, "main", 42);
        return true;
    }

    std::filesystem::path p(sourcePath);
    std::error_code ec;
    if (!std::filesystem::exists(p, ec))
    {
        if (ec)
        {
            ctx.getDiagCollector()->error("EzCompiler", "Cannot access input file {}: {}", sourcePath, ec.message());
        }
        else
        {
            ctx.getDiagCollector()->error("EzCompiler", "Input file not found: {}", sourcePath);
        }
        return false;
    }

    if (p.extension() == ".mir")
    {
        return loadMirFile(ctx, sourcePath, outMirCtx);
    }

    // The real language frontend is not wired in yet: reject unsupported inputs rather than
    // silently synthesizing a main() that hides the missing translation.
    ctx.getDiagCollector()->error("EzCompiler",
                                  "Unsupported input format '{}': only .mir input is supported",
                                  p.extension().string());
    return false;
}

bool MirModuleLoader::loadMirFile(DriverContext &ctx, std::string_view mirPath, MirBuilderContext &outMirCtx)
{
    std::filesystem::path p(mirPath);
    std::error_code ec;
    if (!std::filesystem::exists(p, ec))
    {
        if (ec)
        {
            ctx.getDiagCollector()->error("EzCompiler", "Cannot access MIR file {}: {}", mirPath, ec.message());
        }
        else
        {
            ctx.getDiagCollector()->error("EzCompiler", "Input file not found: {}", mirPath);
        }
        return false;
    }

    std::ifstream file(p, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to open input MIR file: {}", mirPath);
        return false;
    }

    // Size the buffer up front and read it in one shot instead of growing a string byte by byte.
    const std::streamsize fileSize = file.tellg();
    if (fileSize < 0)
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to determine size of input MIR file: {}", mirPath);
        return false;
    }
    file.seekg(0, std::ios::beg);

    std::string content(static_cast<size_t>(fileSize), '\0');
    if (fileSize > 0 && !file.read(content.data(), fileSize))
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to read input MIR file: {}", mirPath);
        return false;
    }

    EzMir::MirParserOptions parserOptions;
    // Enforce SSA well-formedness only when optimizing, where the invariant matters.
    parserOptions.verifySsa = (ctx.getOptions().optLevel != OptimizationLevel::O0);

    // Share the session SourceManager so source spans retained by parsed MIR stay resolvable after
    // parsing finishes.
    EzMir::MirParser parser(&outMirCtx, ctx.getDiagCollector(), parserOptions, ctx.getSourceManager());
    if (!parser.parseModule(content, mirPath))
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to parse MIR file: {}", mirPath);
        return false;
    }

    return true;
}

MirFunction *MirModuleLoader::createReturnConstFunction(DriverContext &ctx, std::string_view funcName, int64_t retVal)
{
    MirBuilderContext *bCtx = ctx.getBuilderContext();
    auto *typeTable = ctx.getTypeTable();
    auto *i64 = typeTable->i64();

    MirFunctionBuilder funcBuilder(bCtx);
    MirFunction *func = funcBuilder.build(i64, {}, funcName);

    MirBlock *entryBlock = func->getEntryPoint();

    MirOperandBuilder opBuilder(bCtx);
    MirInstructionBuilder instBuilder(bCtx, entryBlock, InsertionType::Append);

    MirRegister *vreg0 = opBuilder.buildVReg(i64);
    MirInteger *immVal = opBuilder.buildInt(i64, FlexInt(retVal, 64));

    // Emit: %vreg0 = mov retVal; ret %vreg0.
    instBuilder.MOV(vreg0, immVal);
    instBuilder.RET(vreg0);

    return func;
}

MirFunction *MirModuleLoader::createArithmeticFunction(DriverContext &ctx, std::string_view funcName)
{
    MirBuilderContext *bCtx = ctx.getBuilderContext();
    auto *typeTable = ctx.getTypeTable();
    auto *i64 = typeTable->i64();

    MirFunctionBuilder funcBuilder(bCtx);
    MirFunction *func = funcBuilder.build(i64, {}, funcName);

    MirBlock *entryBlock = func->getEntryPoint();

    MirOperandBuilder opBuilder(bCtx);
    MirInstructionBuilder instBuilder(bCtx, entryBlock, InsertionType::Append);

    MirRegister *v0 = opBuilder.buildVReg(i64);
    MirRegister *v1 = opBuilder.buildVReg(i64);
    MirRegister *v2 = opBuilder.buildVReg(i64);

    MirInteger *imm10 = opBuilder.buildInt(i64, FlexInt(10, 64));
    MirInteger *imm32 = opBuilder.buildInt(i64, FlexInt(32, 64));

    // Emit: v0 = 10; v1 = 32; v2 = v0 + v1; ret v2.
    instBuilder.MOV(v0, imm10);
    instBuilder.MOV(v1, imm32);
    instBuilder.ADD(v2, v0, v1);
    instBuilder.RET(v2);

    return func;
}

} // namespace EzCompiler
