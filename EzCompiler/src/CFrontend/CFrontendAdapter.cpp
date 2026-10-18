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

namespace EzCompiler
{

using namespace CFrontend;

CFrontendAdapter::CFrontendAdapter() = default;

std::string CFrontendAdapter::getBundledFreestandingIncludeDir()
{
    return CFrontend::ChibiccWrapper::getBundledFreestandingIncludeDir();
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
    auto *typeTable = ctx.getTypeTable();
    MirFunctionBuilder funcBuilder(&outMirCtx);
    MirOperandBuilder opBuilder(&outMirCtx);

    // Pass 1: Global Variables and Function Declarations
    for (Obj *obj = astRoot; obj; obj = obj->next)
    {
        if (!obj->is_function)
        {
            // Global Variable
            MirType *gType = mapType(typeTable, obj->ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirGlobalVarBuilder gb(&outMirCtx);
            gb.setConstant(false);

            if (obj->init_data)
            {
                int64_t initVal = 0;
                if (obj->ty->size >= 8)
                    initVal = *(const int64_t *)obj->init_data;
                else if (obj->ty->size >= 4)
                    initVal = *(const int32_t *)obj->init_data;
                else if (obj->ty->size >= 2)
                    initVal = *(const int16_t *)obj->init_data;
                else
                    initVal = *(const int8_t *)obj->init_data;

                MirType *initType = (gType->getKind() == MirTypeKind::Integer) ? gType : typeTable->i64();
                gb.setInitializer(opBuilder.buildInt(initType, FlexInt(initVal, static_cast<uint16_t>(initType->getTotalSizeInBits()))));
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
            // External function prototype
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            std::vector<MirType *> paramTypes;
            for (Obj *p = obj->params; p; p = p->next)
            {
                paramTypes.push_back(mapType(typeTable, p->ty));
            }
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            funcBuilder.declare(retType,
                                std::span<MirType *const>(paramTypes.data(), paramTypes.size()),
                                obj->name,
                                linkage);
        }
    }

    // Pass 2: Function Definitions
    for (Obj *obj = astRoot; obj; obj = obj->next)
    {
        if (obj->is_function && obj->is_definition && obj->body)
        {
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirFunction *func = funcBuilder.build(retType, {}, obj->name, linkage);
            if (!func)
            {
                ctx.getDiagCollector()->error("CFrontend", "Failed to build MIR function for {}", obj->name);
                return false;
            }

            CFrontendFunctionLowerer lowerer(ctx, outMirCtx, func, obj);
            lowerer.lower();
        }
    }

    return true;
}

} // namespace EzCompiler
