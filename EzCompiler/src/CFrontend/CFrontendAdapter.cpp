#include "CFrontend/CFrontendAdapter.h"
#include "CFrontend/CFrontendTypeMapper.h"
#include "CFrontend/CFrontendFunctionLowerer.h"
#include "Debug/MirDebugMetadata.h"
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
#include <filesystem>
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

std::string CFrontendAdapter::getBundledHostedCompatIncludeDir()
{
    return CFrontend::ChibiccWrapper::getBundledHostedCompatIncludeDir();
}

void CFrontendAdapter::addIncludePath(std::string_view path)
{
    m_wrapper.addIncludePath(path);
}

void CFrontendAdapter::defineMacro(std::string_view name, std::string_view value)
{
    m_wrapper.defineMacro(name, value);
}

namespace CFrontend
{

SourceReference *CFrontendDebugContext::createSourceRefFromToken(DriverContext &ctx, const Token *tok) const
{
    if (!tok || !tok->file || !tok->loc || !tok->file->contents)
    {
        return nullptr;
    }

    auto it = fileToSrcMgrId.find(tok->file);
    if (it == fileToSrcMgrId.end() || it->second == 0)
    {
        return nullptr;
    }

    const char *contents = tok->file->contents;
    if (tok->loc < contents)
    {
        return nullptr;
    }

    const size_t srcId = it->second;
    const size_t offset = static_cast<size_t>(tok->loc - contents);
    const size_t contentLen = ctx.getSourceManager()->getSourceContent(srcId).size();
    if (offset > contentLen)
    {
        return nullptr;
    }

    size_t len = tok->len > 0 ? static_cast<size_t>(tok->len) : 1;
    if (offset + len > contentLen)
    {
        len = contentLen - offset;
    }

    return ctx.getSourceManager()->createReference(offset, len, srcId);
}

EzCore::Debug::DebugSourceLoc CFrontendDebugContext::debugLocFromToken(DriverContext &ctx, const Token *tok) const
{
    EzCore::Debug::DebugSourceLoc loc;
    if (!tok || !tok->file)
    {
        return loc;
    }

    auto it = fileToDebugFileId.find(tok->file);
    if (it == fileToDebugFileId.end() || it->second == 0)
    {
        return loc;
    }

    loc.fileId = it->second;
    if (SourceReference *srcRef = createSourceRefFromToken(ctx, tok))
    {
        if (const SourceLineRange *lineRange = ctx.getSourceManager()->getReferenceLine(srcRef))
        {
            loc.line = static_cast<uint32_t>(lineRange->m_lineNumber);
            loc.column = static_cast<uint32_t>(srcRef->m_beginOffset - lineRange->m_beginOffset + 1);
            return loc;
        }
    }

    if (tok->line_no > 0)
    {
        loc.line = static_cast<uint32_t>(tok->line_no);
        loc.column = 1;
    }
    return loc;
}

uint64_t CFrontendDebugContext::registerCType(DriverContext &ctx, const Type *ty, MirDebugMetadata &meta)
{
    if (!ty || ty->kind == TY_VOID)
    {
        return 0;
    }

    if (auto it = visitedTypes.find(ty); it != visitedTypes.end())
    {
        return it->second;
    }

    const uint64_t typeId = nextTypeId++;
    visitedTypes[ty] = typeId; // Pre-insert before recursing to break self-referential cycles.

    EzCore::Debug::DebugType dt;
    dt.typeId = typeId;
    dt.sizeInBytes = ty->size > 0 ? static_cast<uint64_t>(ty->size) : 0;
    dt.alignment = ty->align > 0 ? static_cast<uint32_t>(ty->align) : 1;
    if (ty->name_pos)
    {
        dt.declLoc = debugLocFromToken(ctx, ty->name_pos);
    }

    switch (ty->kind)
    {
        case TY_BOOL:
            dt.kind = EzCore::RttiTypeKind::Boolean;
            dt.encoding = EzCore::Debug::DebugBaseEncoding::Boolean;
            dt.name = "_Bool";
            break;
        case TY_CHAR:
            dt.kind = ty->is_unsigned ? EzCore::RttiTypeKind::UnsignedInt : EzCore::RttiTypeKind::SignedInt;
            dt.encoding = ty->is_unsigned ? EzCore::Debug::DebugBaseEncoding::UnsignedChar
                                          : EzCore::Debug::DebugBaseEncoding::SignedChar;
            dt.name = ty->is_unsigned ? "unsigned char" : "char";
            break;
        case TY_SHORT:
            dt.kind = ty->is_unsigned ? EzCore::RttiTypeKind::UnsignedInt : EzCore::RttiTypeKind::SignedInt;
            dt.encoding = ty->is_unsigned ? EzCore::Debug::DebugBaseEncoding::Unsigned
                                          : EzCore::Debug::DebugBaseEncoding::Signed;
            dt.name = ty->is_unsigned ? "unsigned short" : "short";
            break;
        case TY_INT:
            dt.kind = ty->is_unsigned ? EzCore::RttiTypeKind::UnsignedInt : EzCore::RttiTypeKind::SignedInt;
            dt.encoding = ty->is_unsigned ? EzCore::Debug::DebugBaseEncoding::Unsigned
                                          : EzCore::Debug::DebugBaseEncoding::Signed;
            dt.name = ty->is_unsigned ? "unsigned int" : "int";
            break;
        case TY_LONG:
            dt.kind = ty->is_unsigned ? EzCore::RttiTypeKind::UnsignedInt : EzCore::RttiTypeKind::SignedInt;
            dt.encoding = ty->is_unsigned ? EzCore::Debug::DebugBaseEncoding::Unsigned
                                          : EzCore::Debug::DebugBaseEncoding::Signed;
            dt.name = ty->is_unsigned ? "unsigned long" : "long";
            break;
        case TY_FLOAT:
            dt.kind = EzCore::RttiTypeKind::Float;
            dt.encoding = EzCore::Debug::DebugBaseEncoding::Float;
            dt.name = "float";
            break;
        case TY_DOUBLE:
            dt.kind = EzCore::RttiTypeKind::Float;
            dt.encoding = EzCore::Debug::DebugBaseEncoding::Float;
            dt.name = "double";
            break;
        case TY_LDOUBLE:
            dt.kind = EzCore::RttiTypeKind::Float;
            dt.encoding = EzCore::Debug::DebugBaseEncoding::Float;
            dt.name = "long double";
            break;
        case TY_ENUM:
            dt.kind = EzCore::RttiTypeKind::Enum;
            dt.encoding = ty->is_unsigned ? EzCore::Debug::DebugBaseEncoding::Unsigned
                                          : EzCore::Debug::DebugBaseEncoding::Signed;
            if (ty->tag_name && ty->tag_name->loc && ty->tag_name->len > 0)
                dt.name = std::string(ty->tag_name->loc, static_cast<size_t>(ty->tag_name->len));
            else if (ty->name && ty->name->loc && ty->name->len > 0)
                dt.name = std::string(ty->name->loc, static_cast<size_t>(ty->name->len));
            if (dt.sizeInBytes == 0)
                dt.sizeInBytes = 4;
            break;
        case TY_PTR:
        case TY_VLA:
            dt.kind = EzCore::RttiTypeKind::Pointer;
            dt.referencedTypeId = registerCType(ctx, ty->base, meta);
            if (dt.sizeInBytes == 0)
                dt.sizeInBytes = 8;
            break;
        case TY_ARRAY:
            dt.kind = EzCore::RttiTypeKind::Array;
            dt.referencedTypeId = registerCType(ctx, ty->base, meta);
            dt.arrayLength = ty->array_len > 0 ? static_cast<uint64_t>(ty->array_len) : 0;
            break;
        case TY_STRUCT:
        case TY_UNION:
            dt.kind = (ty->kind == TY_UNION) ? EzCore::RttiTypeKind::Union : EzCore::RttiTypeKind::Struct;
            if (ty->tag_name && ty->tag_name->loc && ty->tag_name->len > 0)
            {
                dt.name = std::string(ty->tag_name->loc, static_cast<size_t>(ty->tag_name->len));
            }
            else if (ty->name && ty->name->loc && ty->name->len > 0)
            {
                dt.name = std::string(ty->name->loc, static_cast<size_t>(ty->name->len));
            }
            for (const Member *m = ty->members; m; m = m->next)
            {
                EzCore::Debug::DebugMember dm;
                if (m->name && m->name->loc && m->name->len > 0)
                {
                    dm.name = std::string(m->name->loc, static_cast<size_t>(m->name->len));
                }
                dm.typeId = registerCType(ctx, m->ty, meta);
                dm.byteOffset = m->offset >= 0 ? static_cast<uint32_t>(m->offset) : 0;
                dm.isBitfield = m->is_bitfield;
                dm.bitOffset = m->bit_offset >= 0 ? static_cast<uint16_t>(m->bit_offset) : 0;
                dm.bitWidth = m->bit_width >= 0 ? static_cast<uint16_t>(m->bit_width) : 0;
                dt.members.push_back(std::move(dm));
            }
            break;
        case TY_FUNC:
            dt.kind = EzCore::RttiTypeKind::Function;
            dt.returnTypeId = registerCType(ctx, ty->return_ty, meta);
            dt.isVariadic = ty->is_variadic;
            for (const Type *p = ty->params; p; p = p->next)
            {
                dt.paramTypeIds.push_back(registerCType(ctx, p, meta));
            }
            break;
        default:
            dt.kind = EzCore::RttiTypeKind::Unknown;
            break;
    }

    meta.compileUnit.addType(std::move(dt));
    return typeId;
}

} // namespace CFrontend

bool CFrontendAdapter::compileSourceToMir(DriverContext &ctx, std::string_view sourcePath, MirBuilderContext &outMirCtx)
{
    m_wrapper.setTargetTriple(ctx.getOptions().target);
    m_wrapper.setHostedMode(!ctx.getOptions().freestanding);
    auto parseRes = m_wrapper.parseFile(sourcePath);
    if (!parseRes.success)
    {
        ctx.getDiagCollector()->error("CFrontend", "{}", parseRes.errorMessage);
        return false;
    }

    return lowerAstToMir(ctx, parseRes.astRoot, outMirCtx, sourcePath);
}

bool CFrontendAdapter::compileStringToMir(DriverContext &ctx, std::string_view sourceCode, MirBuilderContext &outMirCtx, std::string_view virtualFileName)
{
    m_wrapper.setTargetTriple(ctx.getOptions().target);
    m_wrapper.setHostedMode(!ctx.getOptions().freestanding);
    auto parseRes = m_wrapper.parseString(sourceCode, virtualFileName);
    if (!parseRes.success)
    {
        ctx.getDiagCollector()->error("CFrontend", "{}", parseRes.errorMessage);
        return false;
    }

    return lowerAstToMir(ctx, parseRes.astRoot, outMirCtx, parseRes.virtualFileName);
}

bool CFrontendAdapter::lowerAstToMir(DriverContext &ctx,
                                     Obj *astRoot,
                                     MirBuilderContext &outMirCtx,
                                     std::string_view virtualFileName)
{
    if (!astRoot)
    {
        return true;
    }

    CFrontendDebugContext debugCtx;
    const bool emitDebug = ctx.getOptions().emitDebugInfo();
    MirDebugMetadata *debugMeta = emitDebug ? outMirCtx.getOrCreateDebugMetadata() : nullptr;
    if (debugMeta)
    {
        debugMeta->compileUnit.language = EzCore::Debug::DebugLanguage::C11;
        debugMeta->compileUnit.level = ctx.getOptions().debugLevel;
        std::error_code ec;
        debugMeta->compileUnit.compDir = std::filesystem::current_path(ec).string();
        debugMeta->compileUnit.arch = ctx.getOptions().target.isRiscV()
                                              ? EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64
                                              : EzCore::Debug::DebugCompileUnit::TargetArch::X86_64;
        debugMeta->compileUnit.pointerSize = 8;
        debugMeta->compileUnit.isLittleEndian = true;
    }

    if (File **files = get_input_files())
    {
        for (int i = 0; files[i] != nullptr; ++i)
        {
            File *f = files[i];
            std::string displayPath;
            if (i == 0 && !virtualFileName.empty())
            {
                displayPath = std::string(virtualFileName);
            }
            else if (f->display_name && f->display_name[0] != '\0')
            {
                displayPath = f->display_name;
            }
            else if (f->name && f->name[0] != '\0')
            {
                displayPath = f->name;
            }

            if (displayPath.empty())
            {
                continue;
            }

            std::string_view contents = f->contents ? std::string_view(f->contents) : std::string_view{};
            size_t srcId = ctx.getSourceManager()->addSourceContent(displayPath, contents);
            if (srcId == 0)
            {
                if (auto *existingRef = ctx.getSourceManager()->createReference(0, 0, displayPath))
                {
                    srcId = existingRef->m_sourceFileId;
                }
            }
            if (srcId != 0)
            {
                debugCtx.fileToSrcMgrId[f] = srcId;
            }

            if (debugMeta)
            {
                uint32_t dbgFileId = debugMeta->compileUnit.addOrGetFile(displayPath);
                if (dbgFileId != 0)
                {
                    debugCtx.fileToDebugFileId[f] = dbgFileId;
                }
                if (i == 0 && debugMeta->compileUnit.mainFile.empty())
                {
                    debugMeta->compileUnit.mainFile = displayPath;
                }
            }
        }
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

            SourceReference *gSrcRef = debugCtx.createSourceRefFromToken(ctx, obj->tok);
            MirGlobalVar *gv = gb.build(linkage, gType, std::pmr::string(obj->name, outMirCtx.getGlobalAllocator()), gSrcRef);
            outMirCtx.appendGlobalVar(gv);
            nameToGlobal[obj->name] = gv;

            if (debugMeta)
            {
                MirDebugGlobalBinding gBind;
                gBind.gvarId = gv->getId();
                gBind.name = obj->name ? obj->name : "";
                gBind.typeId = debugCtx.registerCType(ctx, obj->ty, *debugMeta);
                gBind.declLoc = debugCtx.debugLocFromToken(ctx, obj->tok);
                gBind.isExternal = !obj->is_static;
                gBind.isConstant = false;
                debugMeta->globalBindings.push_back(std::move(gBind));
            }
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
            SourceReference *fSrcRef = debugCtx.createSourceRefFromToken(ctx, obj->tok);
            MirFunction *func = funcBuilder.declare(retType,
                                                    std::span<MirType *const>(paramTypes.data(), paramTypes.size()),
                                                    obj->name,
                                                    linkage,
                                                    nullptr,
                                                    fSrcRef);
            funcMap[obj] = func;
            nameToFunc[obj->name] = func;

            if (debugMeta && func)
            {
                MirDebugFuncBinding fBind;
                fBind.funcId = func->getId();
                fBind.name = obj->name ? obj->name : "";
                fBind.returnTypeId = debugCtx.registerCType(ctx, obj->ty->return_ty, *debugMeta);
                fBind.declLoc = debugCtx.debugLocFromToken(ctx, obj->tok);
                fBind.isExternal = !obj->is_static;
                debugMeta->funcBindings.push_back(std::move(fBind));
            }
        }
        else
        {
            if (obj->is_inline && !obj->is_live)
            {
                continue;
            }
            if (auto it = nameToFunc.find(obj->name); it != nameToFunc.end())
            {
                funcMap[obj] = it->second;
                continue;
            }

            // Function definition: build MirFunction upfront so it exists in outMirCtx
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            SourceReference *fSrcRef = debugCtx.createSourceRefFromToken(ctx, obj->tok);
            MirFunction *func = funcBuilder.build(retType, {}, obj->name, linkage, nullptr, fSrcRef);
            if (!func)
            {
                ctx.getDiagCollector()->error("CFrontend", "Failed to build MIR function for {}", obj->name);
                return false;
            }
            funcMap[obj] = func;
            nameToFunc[obj->name] = func;

            if (debugMeta)
            {
                MirDebugFuncBinding fBind;
                fBind.funcId = func->getId();
                fBind.name = obj->name ? obj->name : "";
                fBind.returnTypeId = debugCtx.registerCType(ctx, obj->ty->return_ty, *debugMeta);
                fBind.declLoc = debugCtx.debugLocFromToken(ctx, obj->tok);
                fBind.isExternal = !obj->is_static;
                debugMeta->funcBindings.push_back(std::move(fBind));
            }
        }
    }

    // Pass 2: Lower Function Bodies
    for (Obj *obj = astRoot; obj; obj = obj->next)
    {
        if (obj->is_function && obj->is_definition && obj->body)
        {
            if (obj->is_inline && !obj->is_live)
            {
                continue;
            }
            MirFunction *func = funcMap[obj];
            CFrontendFunctionLowerer lowerer(ctx, outMirCtx, func, obj, &debugCtx);
            if (!lowerer.lower())
            {
                return false;
            }
        }
    }

    return true;
}

} // namespace EzCompiler
