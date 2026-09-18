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

bool MirModuleLoader::compileSourceToMir(DriverContext &ctx,
                                         std::string_view sourcePath,
                                         MirBuilderContext &outMirCtx)
{
    if (sourcePath.empty())
    {
        // Default standalone synthetic program for driver verification
        createReturnConstFunction(ctx, "main", 42);
        return true;
    }

    std::filesystem::path p(sourcePath);
    if (!std::filesystem::exists(p))
    {
        ctx.getDiagCollector()->error("EzCompiler", "Input file not found: {}", sourcePath);
        return false;
    }

    if (p.extension() == ".mir")
    {
        return loadMirFile(ctx, sourcePath, outMirCtx);
    }

    // When the future EzFrontend 2.0 is connected, it will be invoked here.
    // For now, generate the main entrypoint function representing the module.
    createReturnConstFunction(ctx, "main", 42);
    return true;
}

bool MirModuleLoader::loadMirFile(DriverContext &ctx,
                                  std::string_view mirPath,
                                  MirBuilderContext &outMirCtx)
{
    std::filesystem::path p(mirPath);
    if (!std::filesystem::exists(p))
    {
        ctx.getDiagCollector()->error("EzCompiler", "Input file not found: {}", mirPath);
        return false;
    }

    std::ifstream file(p, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to open input MIR file: {}", mirPath);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    EzMir::MirParserOptions parserOptions;
    parserOptions.verifySsa = (ctx.getOptions().optLevel != OptimizationLevel::O0);

    EzMir::MirParser parser(&outMirCtx, ctx.getDiagCollector(), parserOptions);
    if (!parser.parseModule(content, mirPath))
    {
        ctx.getDiagCollector()->error("EzCompiler", "Failed to parse MIR file: {}", mirPath);
        return false;
    }

    return true;
}

MirFunction *MirModuleLoader::createReturnConstFunction(DriverContext &ctx,
                                                       std::string_view funcName,
                                                       int64_t retVal)
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

    instBuilder.MOV(vreg0, immVal);
    instBuilder.RET(vreg0);

    return func;
}

MirFunction *MirModuleLoader::createArithmeticFunction(DriverContext &ctx,
                                                      std::string_view funcName)
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

    instBuilder.MOV(v0, imm10);
    instBuilder.MOV(v1, imm32);
    instBuilder.ADD(v2, v0, v1);
    instBuilder.RET(v2);

    return func;
}

} // namespace EzCompiler
