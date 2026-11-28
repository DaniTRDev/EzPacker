#include "Debug/DebugInfo.h"

#include <algorithm>
#include <unordered_set>

namespace EzCore::Debug
{

namespace
{

std::string normalizePathSeparators(std::string_view path)
{
    std::string result(path);
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

} // namespace

void splitDirectoryAndFilename(std::string_view path, std::string &outDir, std::string &outFile)
{
    if (path.empty())
    {
        outDir.clear();
        outFile.clear();
        return;
    }

    const size_t sepPos = path.find_last_of("/\\");
    if (sepPos == std::string_view::npos)
    {
        outDir.clear();
        outFile.assign(path);
        return;
    }

    // Preserve root slash ("/foo" -> "/", "foo") or Windows drive root ("C:\foo" -> "C:\", "foo").
    if (sepPos == 0 || (sepPos == 2 && path[1] == ':'))
    {
        outDir.assign(path.substr(0, sepPos + 1));
    }
    else
    {
        outDir.assign(path.substr(0, sepPos));
    }
    outFile.assign(path.substr(sepPos + 1));
}

uint32_t DebugCompileUnit::addOrGetFile(std::string_view fullPath)
{
    if (fullPath.empty())
    {
        return 0;
    }

    std::string normalized = normalizePathSeparators(fullPath);
    for (const auto &entry : files)
    {
        if (entry.fullPath == normalized)
        {
            return entry.fileId;
        }
    }

    DebugSourceFile fileEntry;
    fileEntry.fileId = static_cast<uint32_t>(files.size()) + 1;
    splitDirectoryAndFilename(normalized, fileEntry.directory, fileEntry.filename);
    fileEntry.fullPath = std::move(normalized);
    files.push_back(std::move(fileEntry));
    return files.back().fileId;
}

void DebugCompileUnit::addType(DebugType type)
{
    for (auto &existing : types)
    {
        if (existing.typeId == type.typeId)
        {
            existing = std::move(type);
            return;
        }
    }
    types.push_back(std::move(type));
}

const DebugType *DebugCompileUnit::findType(uint64_t typeId) const
{
    for (const auto &entry : types)
    {
        if (entry.typeId == typeId)
        {
            return &entry;
        }
    }
    return nullptr;
}

bool DebugCompileUnit::validate(std::string &outError) const
{
    outError.clear();

    if (pointerSize != 4 && pointerSize != 8)
    {
        outError = "DebugCompileUnit: invalid pointerSize (expected 4 or 8)";
        return false;
    }

    std::unordered_set<uint32_t> fileIds;
    fileIds.reserve(files.size());
    for (const auto &file : files)
    {
        if (file.fileId == 0)
        {
            outError = "DebugCompileUnit: fileId must be 1-based (non-zero)";
            return false;
        }
        if (!fileIds.insert(file.fileId).second)
        {
            outError = "DebugCompileUnit: duplicate fileId in file table";
            return false;
        }
    }

    std::unordered_set<uint64_t> typeIds;
    typeIds.reserve(types.size());
    for (const auto &type : types)
    {
        if (type.typeId == 0)
        {
            outError = "DebugCompileUnit: DebugType::typeId must be non-zero";
            return false;
        }
        if (!typeIds.insert(type.typeId).second)
        {
            outError = "DebugCompileUnit: duplicate typeId in type table";
            return false;
        }
        if (type.declLoc.fileId != 0 && !fileIds.contains(type.declLoc.fileId))
        {
            outError = "DebugCompileUnit: DebugType::declLoc references unknown fileId";
            return false;
        }
    }

    for (const auto &func : functions)
    {
        if (func.name.empty())
        {
            outError = "DebugCompileUnit: DebugFunction::name must not be empty";
            return false;
        }
        if (func.declLoc.fileId != 0 && !fileIds.contains(func.declLoc.fileId))
        {
            outError = "DebugCompileUnit: DebugFunction::declLoc references unknown fileId";
            return false;
        }
        for (const auto &line : func.lineEntries)
        {
            if (line.fileId != 0 && !fileIds.contains(line.fileId))
            {
                outError = "DebugCompileUnit: DebugLineEntry references unknown fileId";
                return false;
            }
        }
    }

    for (const auto &global : globals)
    {
        if (global.name.empty())
        {
            outError = "DebugCompileUnit: DebugGlobalVar::name must not be empty";
            return false;
        }
        if (global.declLoc.fileId != 0 && !fileIds.contains(global.declLoc.fileId))
        {
            outError = "DebugCompileUnit: DebugGlobalVar::declLoc references unknown fileId";
            return false;
        }
    }

    return true;
}

uint16_t mapX86_64ToDwarfReg(uint32_t regId, bool isFpr)
{
    if (isFpr)
    {
        if (regId < 16)
        {
            return static_cast<uint16_t>(17 + regId);
        }
        if (regId < 32)
        {
            return static_cast<uint16_t>(67 + (regId - 16));
        }
        return 0xFFFF;
    }

    // Hardware x86-64 GPR encoding:
    // 0=RAX, 1=RCX, 2=RDX, 3=RBX, 4=RSP, 5=RBP, 6=RSI, 7=RDI, 8..15=R8..R15.
    // System V AMD64 DWARF encoding:
    // 0=RAX, 1=RDX, 2=RCX, 3=RBX, 4=RSI, 5=RDI, 6=RBP, 7=RSP, 8..15=R8..R15.
    static constexpr uint16_t kGprToDwarf[16] = {
        0,  // RAX -> 0
        2,  // RCX -> 2
        1,  // RDX -> 1
        3,  // RBX -> 3
        7,  // RSP -> 7
        6,  // RBP -> 6
        4,  // RSI -> 4
        5,  // RDI -> 5
        8,  // R8  -> 8
        9,  // R9  -> 9
        10, // R10 -> 10
        11, // R11 -> 11
        12, // R12 -> 12
        13, // R13 -> 13
        14, // R14 -> 14
        15  // R15 -> 15
    };

    if (regId < 16)
    {
        return kGprToDwarf[regId];
    }
    return 0xFFFF;
}

uint16_t mapRiscV64ToDwarfReg(uint32_t regId, bool isFpr)
{
    if (regId >= 32)
    {
        return 0xFFFF;
    }
    return static_cast<uint16_t>(isFpr ? (32 + regId) : regId);
}

uint16_t mapX86_64ToCodeViewReg(uint32_t regId, bool isFpr, uint32_t byteSize)
{
    if (regId >= 16)
    {
        return 0; // CV_AMD64_NOREG
    }

    if (isFpr)
    {
        // CV_AMD64_XMM0..XMM7 = 154..161, CV_AMD64_XMM8..XMM15 = 252..259.
        if (regId < 8)
        {
            return static_cast<uint16_t>(154 + regId);
        }
        return static_cast<uint16_t>(252 + (regId - 8));
    }

    if (byteSize == 1)
    {
        static constexpr uint16_t kGpr8[16] = {
            1,   // AL
            2,   // CL
            3,   // DL
            4,   // BL
            327, // SPL
            326, // BPL
            324, // SIL
            325, // DIL
            344, 345, 346, 347, 348, 349, 350, 351 // R8B..R15B
        };
        return kGpr8[regId];
    }

    if (byteSize == 2)
    {
        if (regId < 8)
        {
            return static_cast<uint16_t>(9 + regId); // AX..DI
        }
        return static_cast<uint16_t>(352 + (regId - 8)); // R8W..R15W
    }

    if (byteSize == 4)
    {
        if (regId < 8)
        {
            return static_cast<uint16_t>(17 + regId); // EAX..EDI
        }
        return static_cast<uint16_t>(360 + (regId - 8)); // R8D..R15D
    }

    // 64-bit (default): CV_AMD64_RAX..R15 = 328..343.
    return static_cast<uint16_t>(328 + regId);
}

} // namespace EzCore::Debug
