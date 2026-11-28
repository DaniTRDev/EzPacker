#include "FrontendAdapter.h"
#include "CFrontend/CFrontendAdapter.h"
#include "Debug/MirDebugMetadata.h"
#include "DriverContext.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "Parser/MirParser.h"
#include <fstream>
#include <unordered_map>

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

    if (p.extension() == ".c" || p.extension() == ".i")
    {
        CFrontendAdapter cAdapter;
        const bool isHosted = ctx.getOptions().isHosted();
        cAdapter.setHostedMode(isHosted);

        for (const auto &inc : ctx.getOptions().includePaths)
        {
            cAdapter.addIncludePath(inc);
        }
        if (isHosted)
        {
            std::string compatDir = CFrontendAdapter::getBundledHostedCompatIncludeDir();
            if (!compatDir.empty())
            {
                cAdapter.addIncludePath(compatDir);
            }
            std::string hDir = CFrontendAdapter::getBundledHostedIncludeDir();
            if (!hDir.empty())
            {
                cAdapter.addIncludePath(hDir);
            }
        }
        else
        {
            std::string fsDir = CFrontendAdapter::getBundledFreestandingIncludeDir();
            if (!fsDir.empty())
            {
                cAdapter.addIncludePath(fsDir);
            }
        }
        for (const auto &def : ctx.getOptions().macroDefines)
        {
            size_t eqPos = def.find('=');
            if (eqPos != std::string::npos)
            {
                cAdapter.defineMacro(def.substr(0, eqPos), def.substr(eqPos + 1));
            }
            else
            {
                cAdapter.defineMacro(def, "1");
            }
        }
        return cAdapter.compileSourceToMir(ctx, sourcePath, outMirCtx);
    }

    ctx.getDiagCollector()->error("EzCompiler",
                                  "Unsupported input format '{}': only .c, .i, and .mir inputs are supported",
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

    if (ctx.getOptions().emitDebugInfo())
    {
        MirDebugMetadata *meta = outMirCtx.getOrCreateDebugMetadata();
        meta->compileUnit.language = EzCore::Debug::DebugLanguage::Mir;
        meta->compileUnit.level = ctx.getOptions().debugLevel;
        std::error_code cwdEc;
        meta->compileUnit.compDir = std::filesystem::current_path(cwdEc).string();
        meta->compileUnit.mainFile = std::string(mirPath);
        meta->compileUnit.arch = ctx.getOptions().target.isRiscV()
                                         ? EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64
                                         : EzCore::Debug::DebugCompileUnit::TargetArch::X86_64;
        meta->compileUnit.pointerSize = 8;
        meta->compileUnit.isLittleEndian = true;
        meta->compileUnit.addOrGetFile(mirPath);

        std::unordered_map<const MirType *, uint64_t> visitedTypes;
        uint64_t nextTypeId = 1;

        auto debugLocFromRef = [&](SourceReference *ref) -> EzCore::Debug::DebugSourceLoc {
            EzCore::Debug::DebugSourceLoc loc;
            if (!ref || ref->m_sourceFileId == 0)
            {
                return loc;
            }
            std::string_view fname = ctx.getSourceManager()->getSourceName(ref->m_sourceFileId);
            loc.fileId = meta->compileUnit.addOrGetFile(fname);
            if (const SourceLineRange *lr = ctx.getSourceManager()->getReferenceLine(ref))
            {
                loc.line = static_cast<uint32_t>(lr->m_lineNumber);
                loc.column = static_cast<uint32_t>(ref->m_beginOffset - lr->m_beginOffset + 1);
            }
            return loc;
        };

        std::function<uint64_t(const MirType *)> registerMirType = [&](const MirType *ty) -> uint64_t {
            if (!ty || ty->getKind() == MirTypeKind::Void)
            {
                return 0;
            }
            if (auto it = visitedTypes.find(ty); it != visitedTypes.end())
            {
                return it->second;
            }

            const uint64_t typeId = nextTypeId++;
            visitedTypes[ty] = typeId;

            EzCore::Debug::DebugType dt;
            dt.typeId = typeId;
            dt.name = std::string(ty->getName());
            dt.sizeInBytes = ty->getTotalSizeInBytes();
            dt.alignment = static_cast<uint32_t>(std::max<size_t>(1, ty->getMaxAlignmentInBits() / 8));

            const auto &subs = ty->getSubTypes();
            switch (ty->getKind())
            {
                case MirTypeKind::Integer:
                    if (ty->getTotalSizeInBits() == 1)
                    {
                        dt.kind = EzCore::RttiTypeKind::Boolean;
                        dt.encoding = EzCore::Debug::DebugBaseEncoding::Boolean;
                    }
                    else
                    {
                        dt.kind = EzCore::RttiTypeKind::SignedInt;
                        dt.encoding = EzCore::Debug::DebugBaseEncoding::Signed;
                    }
                    break;
                case MirTypeKind::FloatingPoint:
                    dt.kind = EzCore::RttiTypeKind::Float;
                    dt.encoding = EzCore::Debug::DebugBaseEncoding::Float;
                    break;
                case MirTypeKind::Pointer:
                    dt.kind = EzCore::RttiTypeKind::Pointer;
                    dt.referencedTypeId = subs.empty() ? 0 : registerMirType(subs[0]);
                    if (dt.sizeInBytes == 0)
                        dt.sizeInBytes = 8;
                    break;
                case MirTypeKind::Array:
                    dt.kind = EzCore::RttiTypeKind::Array;
                    dt.referencedTypeId = subs.empty() ? 0 : registerMirType(subs[0]);
                    if (!subs.empty() && subs[0] && subs[0]->getTotalSizeInBytes() > 0)
                    {
                        dt.arrayLength = dt.sizeInBytes / subs[0]->getTotalSizeInBytes();
                    }
                    break;
                case MirTypeKind::Vector:
                    dt.kind = EzCore::RttiTypeKind::Vector;
                    dt.referencedTypeId = subs.empty() ? 0 : registerMirType(subs[0]);
                    if (!subs.empty() && subs[0] && subs[0]->getTotalSizeInBytes() > 0)
                    {
                        dt.arrayLength = dt.sizeInBytes / subs[0]->getTotalSizeInBytes();
                    }
                    break;
                case MirTypeKind::Function:
                    dt.kind = EzCore::RttiTypeKind::Function;
                    if (!subs.empty())
                    {
                        dt.returnTypeId = registerMirType(subs[0]);
                        for (size_t i = 1; i < subs.size(); ++i)
                        {
                            dt.paramTypeIds.push_back(registerMirType(subs[i]));
                        }
                    }
                    break;
                default:
                    dt.kind = EzCore::RttiTypeKind::Unknown;
                    break;
            }

            meta->compileUnit.addType(std::move(dt));
            return typeId;
        };

        for (MirGlobalVar *gv : outMirCtx.getGlobalVars())
        {
            if (!gv)
            {
                continue;
            }
            MirDebugGlobalBinding gBind;
            gBind.gvarId = gv->getId();
            gBind.name = std::string(gv->getName());
            gBind.typeId = registerMirType(gv->getType());
            gBind.declLoc = debugLocFromRef(gv->getSourceRef());
            gBind.isExternal = (gv->getLinkage() != MirLinkage::Internal);
            gBind.isConstant = gv->isConstant();
            meta->globalBindings.push_back(std::move(gBind));
        }

        for (MirFunction *fn : outMirCtx.getFunctions())
        {
            if (!fn)
            {
                continue;
            }
            MirDebugFuncBinding fBind;
            fBind.funcId = fn->getId();
            fBind.name = std::string(fn->getName());
            fBind.returnTypeId = registerMirType(fn->getReturnType());
            fBind.declLoc = debugLocFromRef(fn->getSourceRef());
            fBind.isExternal = (fn->getLinkage() != MirLinkage::Internal);
            meta->funcBindings.push_back(std::move(fBind));

            uint32_t paramIdx = 0;
            for (MirRegister *param : fn->getParameters())
            {
                if (!param)
                {
                    continue;
                }
                MirDebugVarBinding vBind;
                vBind.funcName = std::string(fn->getName());
                vBind.varName = param->getName().empty() ? ("arg" + std::to_string(paramIdx))
                                                         : std::string(param->getName());
                vBind.typeId = registerMirType(param->getMirType());
                vBind.isParameter = true;
                vBind.paramIndex = paramIdx;
                vBind.declLoc = fBind.declLoc;
                vBind.vregId = param->getRef().getId();
                meta->varBindings.push_back(std::move(vBind));
                ++paramIdx;
            }

            if (fn->getStackFrame())
            {
                size_t localIdx = 0;
                for (StackFrameObject *obj : fn->getStackFrame()->getObjects())
                {
                    if (!obj || obj->m_source != StackFrameObjectSource::Variable)
                    {
                        continue;
                    }
                    MirDebugVarBinding vBind;
                    vBind.funcName = std::string(fn->getName());
                    vBind.varName = "local_" + std::to_string(localIdx++);
                    vBind.typeId = registerMirType(obj->m_type);
                    vBind.isParameter = false;
                    vBind.declLoc = fBind.declLoc;
                    vBind.stackObjId = obj->m_id;
                    meta->varBindings.push_back(std::move(vBind));
                }
            }
        }
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
