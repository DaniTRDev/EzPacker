#include "CFrontend/CFrontendAdapter.h"
#include "CFrontend/CFrontendTypeMapper.h"
#include "CFrontend/CFrontendFunctionLowerer.h"
#include "DriverContext.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "GlobalVar/MirGlobalVar.h"
#include "GlobalVar/MirGlobalVarBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "chibicc.h"
#include <span>
#include <unordered_map>
#include <unordered_set>

namespace EzCompiler
{

using namespace CFrontend;

CFrontendAdapter::CFrontendAdapter() = default;

std::string CFrontendAdapter::getBundledFreestandingIncludeDir()
{
    return CFrontend::ChibiccWrapper::getBundledFreestandingIncludeDir();
}

std::string CFrontendAdapter::getBundledHostedIncludeDir()
{
    return CFrontend::ChibiccWrapper::getBundledHostedIncludeDir();
}

void CFrontendAdapter::addIncludePath(std::string_view path)
{
    m_wrapper.addIncludePath(path);
}

void CFrontendAdapter::defineMacro(std::string_view name, std::string_view value)
{
    m_wrapper.defineMacro(name, value);
}

bool CFrontendAdapter::compileSourceToMir(DriverContext &ctx, std::string_view sourcePath, MirBuilderContext &outMirCtx)
{
    m_wrapper.setHostedMode(!ctx.getOptions().freestanding);
    auto parseRes = m_wrapper.parseFile(sourcePath);
    if (!parseRes.success)
    {
        ctx.getDiagCollector()->error("CFrontend", "{}", parseRes.errorMessage);
        return false;
    }

    return lowerAstToMir(ctx, parseRes.astRoot, outMirCtx);
}

bool CFrontendAdapter::compileStringToMir(DriverContext &ctx, std::string_view sourceCode, MirBuilderContext &outMirCtx, std::string_view virtualFileName)
{
    m_wrapper.setHostedMode(!ctx.getOptions().freestanding);
    auto parseRes = m_wrapper.parseString(sourceCode, virtualFileName);
    if (!parseRes.success)
    {
        ctx.getDiagCollector()->error("CFrontend", "{}", parseRes.errorMessage);
        return false;
    }

    return lowerAstToMir(ctx, parseRes.astRoot, outMirCtx);
}

bool CFrontendAdapter::lowerAstToMir(DriverContext &ctx, Obj *astRoot, MirBuilderContext &outMirCtx)
{
    if (!astRoot)
    {
        return true;
    }

    MirTypeTable *typeTable = outMirCtx.getTypeTable();
    MirOperandBuilder opBuilder(&outMirCtx);
    MirFunctionBuilder funcBuilder(&outMirCtx);

    std::unordered_map<Obj *, MirFunction *> funcMap;
    std::unordered_map<std::string_view, MirFunction *> nameToFunc;
    std::unordered_map<std::string_view, MirGlobalVar *> nameToGlobal;

    // Pass 1: Declare all Global Variables and Functions upfront
    for (Obj *obj = astRoot; obj; obj = obj->next)
    {
        if (!obj->is_function)
        {
            // Global Variable
            if (auto it = nameToGlobal.find(obj->name); it != nameToGlobal.end())
            {
                continue;
            }

            MirType *gType = mapType(typeTable, obj->ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirGlobalVarBuilder gb(&outMirCtx);
            gb.setConstant(false);
            if (!obj->is_definition)
            {
                gb.setIsDeclaration(true);
            }

            if (obj->init_data && obj->ty->size > 0)
            {
                size_t bitWidth = static_cast<size_t>(obj->ty->size) * 8;
                FlexInt rawVal = FlexInt::readBytes(
                    std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(obj->init_data), obj->ty->size),
                    bitWidth,
                    false,
                    Endianness::Little
                );
                MirType *initType = (gType->getKind() == MirTypeKind::Integer && gType->getTotalSizeInBits() == bitWidth)
                    ? gType
                    : typeTable->create(MirTypeKind::Integer, bitWidth, 8, {}, "init_" + std::to_string(bitWidth), MirTypeTable::CustomCompactId);
                gb.setInitializer(opBuilder.buildInt(initType, rawVal));
            }
            else if (obj->is_definition && gType->getKind() == MirTypeKind::Integer)
            {
                gb.setInitializer(opBuilder.buildInt(gType, FlexInt(0, static_cast<uint16_t>(gType->getTotalSizeInBits()))));
            }

            MirGlobalVar *gv = gb.build(linkage, gType, std::pmr::string(obj->name, outMirCtx.getGlobalAllocator()));
            outMirCtx.appendGlobalVar(gv);
        }
        else if (!obj->is_definition || !obj->body)
        {
            if (auto it = nameToFunc.find(obj->name); it != nameToFunc.end())
            {
                funcMap[obj] = it->second;
                continue;
            }

            // External function prototype
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            std::vector<MirType *> paramTypes;
            for (Obj *p = obj->params; p; p = p->next)
            {
                paramTypes.push_back(mapType(typeTable, p->ty));
            }
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirFunction *func = funcBuilder.declare(retType,
                                                    std::span<MirType *const>(paramTypes.data(), paramTypes.size()),
                                                    obj->name,
                                                    linkage);
            funcMap[obj] = func;
            nameToFunc[obj->name] = func;
        }
        else
        {
            if (auto it = nameToFunc.find(obj->name); it != nameToFunc.end())
            {
                funcMap[obj] = it->second;
                continue;
            }

            // Function definition: build MirFunction upfront so it exists in outMirCtx
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirFunction *func = funcBuilder.build(retType, {}, obj->name, linkage);
            if (!func)
            {
                ctx.getDiagCollector()->error("CFrontend", "Failed to build MIR function for {}", obj->name);
                return false;
            }
            funcMap[obj] = func;
            nameToFunc[obj->name] = func;
        }
    }

    // Pass 2: Lower Function Bodies
    for (Obj *obj = astRoot; obj; obj = obj->next)
    {
        if (obj->is_function && obj->is_definition && obj->body)
        {
            MirFunction *func = funcMap[obj];
            CFrontendFunctionLowerer lowerer(ctx, outMirCtx, func, obj);
            if (!lowerer.lower())
            {
                return false;
            }
        }
    }

    return true;
}

} // namespace EzCompiler
