#include "Debug/CodeViewEmitter.h"

#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace EzCodeEmitter::Debug
{

namespace
{

constexpr uint32_t CV_SIGNATURE_C13 = 0x00000004;

// Predefined CodeView Simple Type Indices
constexpr uint32_t T_VOID = 0x0003;
constexpr uint32_t T_CHAR = 0x0010;
constexpr uint32_t T_UCHAR = 0x0020;
constexpr uint32_t T_BOOL08 = 0x0030;
constexpr uint32_t T_REAL32 = 0x0040;
constexpr uint32_t T_REAL64 = 0x0041;
constexpr uint32_t T_INT2 = 0x0072;
constexpr uint32_t T_UINT2 = 0x0073;
constexpr uint32_t T_INT4 = 0x0074;
constexpr uint32_t T_UINT4 = 0x0075;
constexpr uint32_t T_INT8 = 0x0076;
constexpr uint32_t T_UINT8 = 0x0077;
constexpr uint32_t T_64PVOID = 0x0603;

// CodeView Type Leaf IDs
constexpr uint16_t LF_POINTER = 0x1002;
constexpr uint16_t LF_PROCEDURE = 0x1008;
constexpr uint16_t LF_ARGLIST = 0x1201;
constexpr uint16_t LF_FIELDLIST = 0x1203;
constexpr uint16_t LF_ENUMERATE = 0x1502;
constexpr uint16_t LF_ARRAY = 0x1503;
constexpr uint16_t LF_STRUCTURE = 0x1505;
constexpr uint16_t LF_UNION = 0x1506;
constexpr uint16_t LF_ENUM = 0x1507;
constexpr uint16_t LF_MEMBER = 0x150D;
constexpr uint16_t LF_FUNC_ID = 0x1601;

// Numeric leaf tags
constexpr uint16_t LF_CHAR = 0x8000;
constexpr uint16_t LF_SHORT = 0x8001;
constexpr uint16_t LF_LONG = 0x8003;
constexpr uint16_t LF_ULONG = 0x8004;
constexpr uint16_t LF_QUADWORD = 0x800A;
constexpr uint16_t LF_UQUADWORD = 0x800B;

// CodeView C13 Subsection Types
constexpr uint32_t DEBUG_S_SYMBOLS = 0xF1;
constexpr uint32_t DEBUG_S_LINES = 0xF2;
constexpr uint32_t DEBUG_S_STRINGTABLE = 0xF3;
constexpr uint32_t DEBUG_S_FILECHKSMS = 0xF4;

// CodeView Symbol Record Kinds
constexpr uint16_t S_FRAMEPROC = 0x1012;
constexpr uint16_t S_OBJNAME = 0x1101;
constexpr uint16_t S_REGISTER = 0x1106;
constexpr uint16_t S_LDATA32 = 0x110C;
constexpr uint16_t S_GDATA32 = 0x110D;
constexpr uint16_t S_REGREL32 = 0x1111;
constexpr uint16_t S_COMPILE3 = 0x113C;
constexpr uint16_t S_LPROC32_ID = 0x1146;
constexpr uint16_t S_GPROC32_ID = 0x1147;
constexpr uint16_t S_PROC_ID_END = 0x114F;

constexpr uint16_t CV_CFL_X64 = 0x00D0;
constexpr uint16_t CV_LINES_HAVE_COLUMNS = 0x0001;
constexpr uint16_t CV_PROP_FWDREF = 0x0080;

void appendU8(std::vector<uint8_t> &out, uint8_t v) { out.push_back(v); }

void appendU16(std::vector<uint8_t> &out, uint16_t v)
{
    out.push_back(static_cast<uint8_t>(v & 0xFFU));
    out.push_back(static_cast<uint8_t>((v >> 8) & 0xFFU));
}

void appendU32(std::vector<uint8_t> &out, uint32_t v)
{
    for (int i = 0; i < 4; ++i)
    {
        out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFU));
    }
}

void appendU64(std::vector<uint8_t> &out, uint64_t v)
{
    for (int i = 0; i < 8; ++i)
    {
        out.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFU));
    }
}

void appendCString(std::vector<uint8_t> &out, std::string_view s)
{
    out.insert(out.end(), s.begin(), s.end());
    out.push_back(0);
}

void patchU32(std::vector<uint8_t> &out, size_t offset, uint32_t v)
{
    for (int i = 0; i < 4; ++i)
    {
        out[offset + i] = static_cast<uint8_t>((v >> (i * 8)) & 0xFFU);
    }
}

void padTypeRecord4(std::vector<uint8_t> &buf)
{
    const size_t rem = buf.size() % 4;
    if (rem == 0)
    {
        return;
    }
    const size_t pad = 4 - rem;
    for (size_t i = pad; i > 0; --i)
    {
        buf.push_back(static_cast<uint8_t>(0xF0U + i)); // 0xF3, 0xF2, 0xF1
    }
}

void padZero4(std::vector<uint8_t> &buf)
{
    while (buf.size() % 4 != 0)
    {
        buf.push_back(0);
    }
}

void appendUnsignedNumeric(std::vector<uint8_t> &out, uint64_t val)
{
    if (val < 0x8000ULL)
    {
        appendU16(out, static_cast<uint16_t>(val));
    }
    else if (val <= 0xFFFFFFFFULL)
    {
        appendU16(out, LF_ULONG);
        appendU32(out, static_cast<uint32_t>(val));
    }
    else
    {
        appendU16(out, LF_UQUADWORD);
        appendU64(out, val);
    }
}

void appendSignedNumeric(std::vector<uint8_t> &out, int64_t val)
{
    if (val >= 0 && val < 0x8000)
    {
        appendU16(out, static_cast<uint16_t>(val));
    }
    else if (val >= -128 && val <= 127)
    {
        appendU16(out, LF_CHAR);
        appendU8(out, static_cast<uint8_t>(static_cast<int8_t>(val)));
    }
    else if (val >= -32768 && val <= 32767)
    {
        appendU16(out, LF_SHORT);
        appendU16(out, static_cast<uint16_t>(static_cast<int16_t>(val)));
    }
    else if (val >= INT32_MIN && val <= INT32_MAX)
    {
        appendU16(out, LF_LONG);
        appendU32(out, static_cast<uint32_t>(static_cast<int32_t>(val)));
    }
    else
    {
        appendU16(out, LF_QUADWORD);
        appendU64(out, static_cast<uint64_t>(val));
    }
}

class TypeTableBuilder
{
  public:
    explicit TypeTableBuilder(const EzCore::Debug::DebugCompileUnit &cu) : m_cu(cu)
    {
        appendU32(m_data, CV_SIGNATURE_C13);
    }

    uint32_t resolveType(uint64_t typeId)
    {
        if (typeId == 0)
        {
            return T_VOID;
        }
        auto it = m_typeMap.find(typeId);
        if (it != m_typeMap.end())
        {
            return it->second;
        }

        const EzCore::Debug::DebugType *ty = m_cu.findType(typeId);
        if (!ty)
        {
            return T_VOID;
        }

        // Guard against non-aggregate cycles
        if (!m_active.insert(typeId).second)
        {
            return T_VOID;
        }

        uint32_t cvIdx = T_VOID;
        switch (ty->kind)
        {
            case EzCore::RttiTypeKind::Void:
                cvIdx = T_VOID;
                break;
            case EzCore::RttiTypeKind::Boolean:
                cvIdx = T_BOOL08;
                break;
            case EzCore::RttiTypeKind::SignedInt:
                if (ty->sizeInBytes == 1) cvIdx = T_CHAR;
                else if (ty->sizeInBytes == 2) cvIdx = T_INT2;
                else if (ty->sizeInBytes == 8) cvIdx = T_INT8;
                else cvIdx = T_INT4;
                break;
            case EzCore::RttiTypeKind::UnsignedInt:
                if (ty->sizeInBytes == 1) cvIdx = T_UCHAR;
                else if (ty->sizeInBytes == 2) cvIdx = T_UINT2;
                else if (ty->sizeInBytes == 8) cvIdx = T_UINT8;
                else cvIdx = T_UINT4;
                break;
            case EzCore::RttiTypeKind::Float:
                cvIdx = (ty->sizeInBytes == 8) ? T_REAL64 : T_REAL32;
                break;
            case EzCore::RttiTypeKind::Pointer:
            {
                const uint32_t pointee = resolveType(ty->referencedTypeId);
                if (pointee == T_VOID)
                {
                    cvIdx = T_64PVOID;
                }
                else
                {
                    std::vector<uint8_t> rec;
                    appendU32(rec, pointee);
                    appendU32(rec, 0x0001000C); // CV_PTR_64 (12) | (8 << 13)
                    cvIdx = emitRecord(LF_POINTER, rec);
                }
                break;
            }
            case EzCore::RttiTypeKind::Array:
            case EzCore::RttiTypeKind::Vector:
            {
                const uint32_t elemType = resolveType(ty->referencedTypeId);
                std::vector<uint8_t> rec;
                appendU32(rec, elemType);
                appendU32(rec, T_UINT8);
                appendUnsignedNumeric(rec, ty->sizeInBytes);
                appendCString(rec, "");
                cvIdx = emitRecord(LF_ARRAY, rec);
                break;
            }
            case EzCore::RttiTypeKind::Struct:
            case EzCore::RttiTypeKind::Union:
            {
                const uint16_t leaf = (ty->kind == EzCore::RttiTypeKind::Union) ? LF_UNION : LF_STRUCTURE;

                // Emit forward declaration first to break self-referential cycles
                const uint32_t fwdIdx = emitAggregate(leaf, 0, CV_PROP_FWDREF, 0, 0, ty->name);
                m_typeMap[typeId] = fwdIdx;

                // Resolve member types and build LF_FIELDLIST
                std::vector<uint8_t> flPayload;
                for (const auto &member : ty->members)
                {
                    const uint32_t mType = resolveType(member.typeId);
                    appendU16(flPayload, LF_MEMBER);
                    appendU16(flPayload, 0x0003); // public
                    appendU32(flPayload, mType);
                    appendUnsignedNumeric(flPayload, member.byteOffset);
                    appendCString(flPayload, member.name);
                    padTypeRecord4(flPayload);
                }
                const uint32_t flIdx = emitRecord(LF_FIELDLIST, flPayload);
                cvIdx = emitAggregate(leaf,
                                      static_cast<uint16_t>(ty->members.size()),
                                      0,
                                      flIdx,
                                      ty->sizeInBytes,
                                      ty->name);
                break;
            }
            case EzCore::RttiTypeKind::Enum:
            {
                std::vector<uint8_t> flPayload;
                for (const auto &etor : ty->enumerators)
                {
                    appendU16(flPayload, LF_ENUMERATE);
                    appendU16(flPayload, 0x0003); // public
                    appendSignedNumeric(flPayload, etor.value);
                    appendCString(flPayload, etor.name);
                    padTypeRecord4(flPayload);
                }
                const uint32_t flIdx = emitRecord(LF_FIELDLIST, flPayload);

                std::vector<uint8_t> enumRec;
                appendU16(enumRec, static_cast<uint16_t>(ty->enumerators.size()));
                appendU16(enumRec, 0);      // property
                appendU32(enumRec, T_INT4); // underlying type
                appendU32(enumRec, flIdx);
                appendCString(enumRec, ty->name);
                cvIdx = emitRecord(LF_ENUM, enumRec);
                break;
            }
            case EzCore::RttiTypeKind::Typedef:
                cvIdx = resolveType(ty->referencedTypeId);
                break;
            default:
                cvIdx = T_VOID;
                break;
        }

        m_active.erase(typeId);
        m_typeMap[typeId] = cvIdx;
        return cvIdx;
    }

    uint32_t emitFuncId(const EzCore::Debug::DebugFunction &fn, bool includeTypes)
    {
        std::vector<uint32_t> paramCvTypes;
        if (includeTypes)
        {
            paramCvTypes.reserve(fn.parameters.size());
            for (const auto &p : fn.parameters)
            {
                paramCvTypes.push_back(resolveType(p.typeId));
            }
        }

        std::vector<uint8_t> argRec;
        appendU32(argRec, static_cast<uint32_t>(paramCvTypes.size()));
        for (uint32_t pt : paramCvTypes)
        {
            appendU32(argRec, pt);
        }
        const uint32_t argListIdx = emitRecord(LF_ARGLIST, argRec);

        const uint32_t retType = includeTypes ? resolveType(fn.returnTypeId) : T_VOID;
        std::vector<uint8_t> procRec;
        appendU32(procRec, retType);
        appendU8(procRec, 0); // CV_CALL_NEAR_C
        appendU8(procRec, 0); // funcattr
        appendU16(procRec, static_cast<uint16_t>(paramCvTypes.size()));
        appendU32(procRec, argListIdx);
        const uint32_t procIdx = emitRecord(LF_PROCEDURE, procRec);

        std::vector<uint8_t> funcIdRec;
        appendU32(funcIdRec, 0); // scopeId
        appendU32(funcIdRec, procIdx);
        appendCString(funcIdRec, fn.name);
        return emitRecord(LF_FUNC_ID, funcIdRec);
    }

    std::vector<uint8_t> takeData() { return std::move(m_data); }

  private:
    uint32_t emitRecord(uint16_t leaf, const std::vector<uint8_t> &payload)
    {
        std::vector<uint8_t> rec;
        rec.reserve(2 + payload.size() + 3);
        appendU16(rec, leaf);
        rec.insert(rec.end(), payload.begin(), payload.end());
        // Pad so total record (2-byte length + rec) is 4-byte aligned
        const size_t rem = (2 + rec.size()) % 4;
        if (rem != 0)
        {
            const size_t pad = 4 - rem;
            for (size_t i = pad; i > 0; --i)
            {
                rec.push_back(static_cast<uint8_t>(0xF0U + i));
            }
        }
        appendU16(m_data, static_cast<uint16_t>(rec.size()));
        m_data.insert(m_data.end(), rec.begin(), rec.end());
        return m_nextTypeIndex++;
    }

    uint32_t emitAggregate(uint16_t leaf,
                           uint16_t count,
                           uint16_t prop,
                           uint32_t fieldList,
                           uint64_t sizeBytes,
                           std::string_view name)
    {
        std::vector<uint8_t> rec;
        appendU16(rec, count);
        appendU16(rec, prop);
        appendU32(rec, fieldList);
        if (leaf == LF_STRUCTURE)
        {
            appendU32(rec, 0); // derived
            appendU32(rec, 0); // vshape
        }
        appendUnsignedNumeric(rec, sizeBytes);
        appendCString(rec, name);
        return emitRecord(leaf, rec);
    }

    const EzCore::Debug::DebugCompileUnit &m_cu;
    std::vector<uint8_t> m_data;
    uint32_t m_nextTypeIndex{ 0x1000 };
    std::unordered_map<uint64_t, uint32_t> m_typeMap;
    std::unordered_set<uint64_t> m_active;
};

void emitSymRecord(std::vector<uint8_t> &buf, uint16_t kind, const std::vector<uint8_t> &payload)
{
    const uint16_t recLen = static_cast<uint16_t>(2 + payload.size());
    appendU16(buf, recLen);
    appendU16(buf, kind);
    buf.insert(buf.end(), payload.begin(), payload.end());
}

void appendC13Subsection(std::vector<uint8_t> &out,
                         std::vector<ObjectFormat::ExtraSectionReloc> &outRelocs,
                         uint32_t type,
                         const std::vector<uint8_t> &payload,
                         const std::vector<ObjectFormat::ExtraSectionReloc> &subRelocs)
{
    appendU32(out, type);
    appendU32(out, static_cast<uint32_t>(payload.size()));
    const size_t payloadBase = out.size();
    out.insert(out.end(), payload.begin(), payload.end());
    for (auto r : subRelocs)
    {
        r.m_offset += payloadBase;
        outRelocs.push_back(std::move(r));
    }
    padZero4(out);
}

void emitSecRelAndSectionRelocs(std::vector<uint8_t> &payload,
                                std::vector<ObjectFormat::ExtraSectionReloc> &relocs,
                                const std::string &targetSymName)
{
    ObjectFormat::ExtraSectionReloc secRel{};
    secRel.m_offset = payload.size();
    secRel.m_targetName = targetSymName;
    secRel.m_targetIsSection = false;
    secRel.m_type = TargetCodeRelocationType::SecRel32;
    secRel.m_addend = 0;
    relocs.push_back(std::move(secRel));
    appendU32(payload, 0);

    ObjectFormat::ExtraSectionReloc secIdx{};
    secIdx.m_offset = payload.size();
    secIdx.m_targetName = targetSymName;
    secIdx.m_targetIsSection = false;
    secIdx.m_type = TargetCodeRelocationType::SectionIdx16;
    secIdx.m_addend = 0;
    relocs.push_back(std::move(secIdx));
    appendU16(payload, 0);
}

} // namespace

std::vector<ObjectFormat::ExtraObjectSection> CodeViewEmitter::emit(const EzCore::Debug::DebugCompileUnit &cu)
{
    if (cu.level == EzCore::Debug::DebugLevel::None)
    {
        return {};
    }

    const bool fullDebug = (cu.level == EzCore::Debug::DebugLevel::Full);

    // 1. Build .debug$T
    TypeTableBuilder typeTab(cu);
    if (fullDebug)
    {
        for (const auto &ty : cu.types)
        {
            typeTab.resolveType(ty.typeId);
        }
    }

    std::vector<uint32_t> funcIds;
    funcIds.reserve(cu.functions.size());
    for (const auto &fn : cu.functions)
    {
        funcIds.push_back(typeTab.emitFuncId(fn, fullDebug));
    }

    ObjectFormat::ExtraObjectSection debugT;
    debugT.m_name = ".debug$T";
    debugT.m_data = typeTab.takeData();
    debugT.m_alignment = 4;
    debugT.m_isDebug = true;

    // 2. Build .debug$S
    ObjectFormat::ExtraObjectSection debugS;
    debugS.m_name = ".debug$S";
    debugS.m_alignment = 4;
    debugS.m_isDebug = true;
    appendU32(debugS.m_data, CV_SIGNATURE_C13);

    // Build DEBUG_S_STRINGTABLE & DEBUG_S_FILECHKSMS
    std::vector<uint8_t> strTable;
    strTable.push_back(0);
    std::unordered_map<std::string, uint32_t> strOffsets;
    auto addString = [&](const std::string &s) -> uint32_t
    {
        if (s.empty())
        {
            return 0;
        }
        auto it = strOffsets.find(s);
        if (it != strOffsets.end())
        {
            return it->second;
        }
        const uint32_t off = static_cast<uint32_t>(strTable.size());
        appendCString(strTable, s);
        strOffsets.emplace(s, off);
        return off;
    };

    std::vector<uint8_t> fileChecksums;
    std::unordered_map<uint32_t, uint32_t> fileIdToChecksumOff;
    auto registerFileChecksum = [&](uint32_t fileId, const std::string &path)
    {
        const uint32_t strOff = addString(path);
        const uint32_t chkOff = static_cast<uint32_t>(fileChecksums.size());
        fileIdToChecksumOff[fileId] = chkOff;
        appendU32(fileChecksums, strOff);
        appendU8(fileChecksums, 0); // checksumLen
        appendU8(fileChecksums, 0); // checksumType = None
        appendU16(fileChecksums, 0); // pad to 4 bytes
    };

    if (!cu.mainFile.empty())
    {
        registerFileChecksum(0, cu.mainFile);
    }
    for (const auto &f : cu.files)
    {
        registerFileChecksum(f.fileId, f.fullPath);
    }

    // Build DEBUG_S_SYMBOLS
    std::vector<uint8_t> symPayload;
    std::vector<ObjectFormat::ExtraSectionReloc> symRelocs;

    {
        std::vector<uint8_t> objRec;
        appendU32(objRec, 0); // signature
        appendCString(objRec, cu.mainFile);
        emitSymRecord(symPayload, S_OBJNAME, objRec);
    }

    {
        std::vector<uint8_t> compRec;
        appendU32(compRec, 0);           // flags (C11)
        appendU16(compRec, CV_CFL_X64);  // machine
        appendU16(compRec, 1);
        appendU16(compRec, 0);
        appendU16(compRec, 0);
        appendU16(compRec, 0);
        appendU16(compRec, 1);
        appendU16(compRec, 0);
        appendU16(compRec, 0);
        appendU16(compRec, 0);
        appendCString(compRec, cu.producer);
        emitSymRecord(symPayload, S_COMPILE3, compRec);
    }

    if (fullDebug)
    {
        for (const auto &global : cu.globals)
        {
            std::vector<uint8_t> gRec;
            appendU32(gRec, typeTab.resolveType(global.typeId));
            const size_t relBase = symPayload.size() + 4; // 2-byte len + 2-byte kind
            const size_t offInRec = gRec.size();
            ObjectFormat::ExtraSectionReloc sRel{};
            sRel.m_offset = relBase + offInRec;
            sRel.m_targetName = global.name;
            sRel.m_targetIsSection = false;
            sRel.m_type = TargetCodeRelocationType::SecRel32;
            symRelocs.push_back(std::move(sRel));
            appendU32(gRec, 0);

            ObjectFormat::ExtraSectionReloc iRel{};
            iRel.m_offset = relBase + gRec.size();
            iRel.m_targetName = global.name;
            iRel.m_targetIsSection = false;
            iRel.m_type = TargetCodeRelocationType::SectionIdx16;
            symRelocs.push_back(std::move(iRel));
            appendU16(gRec, 0);

            appendCString(gRec, global.name);
            emitSymRecord(symPayload, global.isExternal ? S_GDATA32 : S_LDATA32, gRec);
        }
    }

    for (size_t i = 0; i < cu.functions.size(); ++i)
    {
        const auto &fn = cu.functions[i];
        const std::string symName = fn.linkageName.empty() ? fn.name : fn.linkageName;

        std::vector<uint8_t> procRec;
        appendU32(procRec, 0); // pParent
        const size_t pEndFieldInSymPayload = symPayload.size() + 4 + procRec.size();
        appendU32(procRec, 0); // pEnd (patched below)
        appendU32(procRec, 0); // pNext
        appendU32(procRec, static_cast<uint32_t>(fn.codeSize));
        appendU32(procRec, fn.prologueSize);
        appendU32(procRec, static_cast<uint32_t>(fn.codeSize));
        appendU32(procRec, funcIds[i]);

        const size_t relBase = symPayload.size() + 4;
        ObjectFormat::ExtraSectionReloc sRel{};
        sRel.m_offset = relBase + procRec.size();
        sRel.m_targetName = symName;
        sRel.m_targetIsSection = false;
        sRel.m_type = TargetCodeRelocationType::SecRel32;
        symRelocs.push_back(std::move(sRel));
        appendU32(procRec, 0);

        ObjectFormat::ExtraSectionReloc iRel{};
        iRel.m_offset = relBase + procRec.size();
        iRel.m_targetName = symName;
        iRel.m_targetIsSection = false;
        iRel.m_type = TargetCodeRelocationType::SectionIdx16;
        symRelocs.push_back(std::move(iRel));
        appendU16(procRec, 0);

        appendU8(procRec, 0); // flags
        appendCString(procRec, fn.name);
        emitSymRecord(symPayload, fn.isExternal ? S_GPROC32_ID : S_LPROC32_ID, procRec);

        if (fullDebug)
        {
            std::vector<uint8_t> fpRec;
            appendU32(fpRec, static_cast<uint32_t>(fn.totalFrameSize));
            appendU32(fpRec, 0);
            appendU32(fpRec, 0);
            appendU32(fpRec, 0);
            appendU32(fpRec, 0);
            appendU16(fpRec, 0);
            appendU32(fpRec, fn.hasFramePointer ? 0x00020000U : 0U);
            emitSymRecord(symPayload, S_FRAMEPROC, fpRec);

            auto emitLocalOrParam = [&](const EzCore::Debug::DebugVariable &var)
            {
                const uint32_t varType = typeTab.resolveType(var.typeId);
                if (var.location.kind == EzCore::Debug::DebugLocationKind::FrameBaseOffset ||
                    var.location.kind == EzCore::Debug::DebugLocationKind::StackPointerOffset)
                {
                    const bool useFp = (var.location.kind == EzCore::Debug::DebugLocationKind::FrameBaseOffset &&
                                        fn.hasFramePointer);
                    const uint16_t cvReg = EzCore::Debug::mapX86_64ToCodeViewReg(useFp ? 5 : 4, false, 8);
                    std::vector<uint8_t> regRel;
                    appendU32(regRel, static_cast<uint32_t>(static_cast<int32_t>(var.location.offset)));
                    appendU32(regRel, varType);
                    appendU16(regRel, cvReg);
                    appendCString(regRel, var.name);
                    emitSymRecord(symPayload, S_REGREL32, regRel);
                }
                else if (var.location.kind == EzCore::Debug::DebugLocationKind::Register)
                {
                    const auto *ty = cu.findType(var.typeId);
                    const uint32_t sz = (ty && ty->sizeInBytes > 0) ? static_cast<uint32_t>(ty->sizeInBytes) : 8;
                    const uint16_t cvReg = EzCore::Debug::mapX86_64ToCodeViewReg(
                            var.location.regId, var.location.isFpr, sz);
                    if (cvReg != 0)
                    {
                        std::vector<uint8_t> regRec;
                        appendU32(regRec, varType);
                        appendU16(regRec, cvReg);
                        appendCString(regRec, var.name);
                        emitSymRecord(symPayload, S_REGISTER, regRec);
                    }
                }
            };

            for (const auto &p : fn.parameters)
            {
                emitLocalOrParam(p);
            }
            for (const auto &l : fn.locals)
            {
                emitLocalOrParam(l);
            }
        }

        const uint32_t endOffset = static_cast<uint32_t>(symPayload.size());
        patchU32(symPayload, pEndFieldInSymPayload, endOffset);
        emitSymRecord(symPayload, S_PROC_ID_END, {});
    }

    appendC13Subsection(debugS.m_data, debugS.m_relocs, DEBUG_S_SYMBOLS, symPayload, symRelocs);

    // Emit DEBUG_S_LINES per function
    for (const auto &fn : cu.functions)
    {
        if (fn.lineEntries.empty())
        {
            continue;
        }
        const std::string symName = fn.linkageName.empty() ? fn.name : fn.linkageName;
        bool hasColumns = false;
        for (const auto &le : fn.lineEntries)
        {
            if (le.column > 0)
            {
                hasColumns = true;
                break;
            }
        }

        std::vector<uint8_t> linesPayload;
        std::vector<ObjectFormat::ExtraSectionReloc> linesRelocs;
        emitSecRelAndSectionRelocs(linesPayload, linesRelocs, symName);
        appendU16(linesPayload, hasColumns ? CV_LINES_HAVE_COLUMNS : 0);
        appendU32(linesPayload, static_cast<uint32_t>(fn.codeSize));

        size_t idx = 0;
        while (idx < fn.lineEntries.size())
        {
            const uint32_t blockFileId = fn.lineEntries[idx].fileId;
            size_t endIdx = idx + 1;
            while (endIdx < fn.lineEntries.size() && fn.lineEntries[endIdx].fileId == blockFileId)
            {
                ++endIdx;
            }
            const uint32_t nLines = static_cast<uint32_t>(endIdx - idx);
            const uint32_t cbBlock = 12 + nLines * 8 + (hasColumns ? nLines * 4 : 0);
            const uint32_t chkOff = fileIdToChecksumOff.contains(blockFileId) ? fileIdToChecksumOff[blockFileId] : 0;

            appendU32(linesPayload, chkOff);
            appendU32(linesPayload, nLines);
            appendU32(linesPayload, cbBlock);

            for (size_t k = idx; k < endIdx; ++k)
            {
                const auto &le = fn.lineEntries[k];
                appendU32(linesPayload, static_cast<uint32_t>(le.codeOffset));
                const uint32_t packed = (le.line & 0x00FFFFFFU) | (le.isStmt ? 0x80000000U : 0U);
                appendU32(linesPayload, packed);
            }
            if (hasColumns)
            {
                for (size_t k = idx; k < endIdx; ++k)
                {
                    appendU16(linesPayload, static_cast<uint16_t>(fn.lineEntries[k].column));
                    appendU16(linesPayload, 0);
                }
            }
            idx = endIdx;
        }

        appendC13Subsection(debugS.m_data, debugS.m_relocs, DEBUG_S_LINES, linesPayload, linesRelocs);
    }

    if (!fileChecksums.empty())
    {
        appendC13Subsection(debugS.m_data, debugS.m_relocs, DEBUG_S_FILECHKSMS, fileChecksums, {});
    }
    appendC13Subsection(debugS.m_data, debugS.m_relocs, DEBUG_S_STRINGTABLE, strTable, {});

    std::vector<ObjectFormat::ExtraObjectSection> result;
    result.reserve(2);
    result.push_back(std::move(debugT));
    result.push_back(std::move(debugS));
    return result;
}

} // namespace EzCodeEmitter::Debug
