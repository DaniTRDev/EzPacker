#include "ObjectFormat/CoffWriter.h"
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace EzCodeEmitter::ObjectFormat
{

namespace
{

#pragma pack(push, 1)
// IMAGE_FILE_HEADER: fixed-size header preceding the section table.
struct CoffFileHeader
{
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

// IMAGE_SECTION_HEADER: describes one section's location and attributes.
struct CoffSectionHeader
{
    uint8_t Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};

// IMAGE_RELOCATION: one entry in a section's relocation table.
struct CoffRelocation
{
    uint32_t VirtualAddress;   // Offset of the field to patch within the section.
    uint32_t SymbolTableIndex; // Index of the target symbol in the symbol table.
    uint16_t Type;             // COFF relocation type (AMD64_*).
};

// COFF symbol table record, including the 8-byte/4-byte name union.
struct CoffSymbol
{
    union
    {
        uint8_t ShortName[8];
        struct
        {
            uint32_t Zeroes;
            uint32_t Offset;
        } LongName;
    } N;
    uint32_t Value;
    int16_t SectionNumber;
    uint16_t Type;
    uint8_t StorageClass;
    uint8_t NumberOfAuxSymbols;
};
#pragma pack(pop)

constexpr uint16_t COFF_MACHINE_AMD64 = 0x8664; // IMAGE_FILE_MACHINE_AMD64.

constexpr uint32_t SCN_TEXT = 0x60500020;  // Code, Execute, Read, Align 16
constexpr uint32_t SCN_RDATA = 0x40500040; // InitData, Read, Align 16
constexpr uint32_t SCN_DATA = 0xC0300040;  // InitData, Read, Write, Align 8
constexpr uint32_t SCN_BSS = 0xC0300080;   // UninitData, Read, Write, Align 8
constexpr uint32_t COFF_SCN_LNK_NRELOC_OVFL = 0x01000000; // IMAGE_SCN_LNK_NRELOC_OVFL: extended reloc count in first entry.

constexpr uint16_t COFF_REL_AMD64_ADDR64 = 0x0001;  // 64-bit absolute address.
constexpr uint16_t COFF_REL_AMD64_ADDR32 = 0x0002;  // 32-bit absolute address.
constexpr uint16_t COFF_REL_AMD64_REL32 = 0x0004;   // 32-bit PC-relative address.
constexpr uint16_t COFF_REL_AMD64_SECTION = 0x000A; // 16-bit section index.
constexpr uint16_t COFF_REL_AMD64_SECREL = 0x000B;  // 32-bit section-relative offset.

constexpr uint8_t COFF_SYM_CLASS_EXTERNAL = 2; // Externally visible symbol.
constexpr uint8_t COFF_SYM_CLASS_STATIC = 3;   // File-local symbol.

// Translates a target-agnostic relocation kind into its COFF AMD64 encoding; 0 = none.
uint16_t mapCoffRelocType(TargetCodeRelocationType type)
{
    switch (type)
    {
        case TargetCodeRelocationType::Absolute64:
            return COFF_REL_AMD64_ADDR64;
        case TargetCodeRelocationType::Absolute32:
            return COFF_REL_AMD64_ADDR32;
        case TargetCodeRelocationType::PCRel32:
        case TargetCodeRelocationType::BranchRel32:
        case TargetCodeRelocationType::PLTRel32:
        case TargetCodeRelocationType::GOTPCREL:
            return COFF_REL_AMD64_REL32;
        case TargetCodeRelocationType::SectionIdx16:
            return COFF_REL_AMD64_SECTION;
        case TargetCodeRelocationType::SecRel32:
            return COFF_REL_AMD64_SECREL;
        default:
            return 0;
    }
}

// Returns the IMAGE_SCN_ALIGN_*BYTES characteristic flag for the given alignment.
uint32_t coffAlignFlag(uint64_t align)
{
    if (align <= 1)
    {
        return 0x00100000;
    }
    if (align <= 2)
    {
        return 0x00200000;
    }
    if (align <= 4)
    {
        return 0x00300000;
    }
    if (align <= 8)
    {
        return 0x00400000;
    }
    return 0x00500000;
}

// Rounds offset up to the next multiple of alignment (alignment assumed power-of-two).
uint64_t alignTo(uint64_t offset, uint64_t alignment)
{
    if (alignment <= 1)
        return offset;
    return (offset + alignment - 1) & ~(alignment - 1);
}

} // namespace

void CoffWriter::addSymbol(const ObjectSymbol &sym) { m_symbols.push_back(sym); }

void CoffWriter::addRelocation(const ObjectRelocEntry &reloc) { m_relocs.push_back(reloc); }

void CoffWriter::addExtraSection(const ExtraObjectSection &section) { m_extraSections.push_back(section); }

void CoffWriter::clear()
{
    m_symbols.clear();
    m_relocs.clear();
    m_extraSections.clear();
}

std::vector<uint8_t> CoffWriter::write(const std::pmr::unordered_map<SectionType, CodeSection *> &sections)
{
    std::vector<uint8_t> fileBuf;

    // Internal per-section staging record used while laying out the COFF file.
    struct SectionInfo
    {
        std::string name;
        uint32_t characteristics{ 0 };
        uint32_t alignment{ 16 };
        std::vector<uint8_t> data;
        bool isBss{ false };
        SectionType secType{ SectionType::Text };
        std::vector<CoffRelocation> relocs;
    };

    std::vector<SectionInfo> secInfos;
    secInfos.reserve(4 + m_extraSections.size());
    std::unordered_map<SectionType, int16_t> sectionIndexMap;     // 1-based for COFF
    std::unordered_map<std::string, int16_t> sectionNameIndexMap; // 1-based for COFF

    // Copies a code section (if present) into the staging list and records its 1-based index.
    auto addSec = [&](SectionType type, const char *name, uint32_t charact, uint32_t align)
    {
        auto it = sections.find(type);
        if (it != sections.end() && it->second)
        {
            CodeSection *sec = it->second;
            std::span<const uint8_t> data = sec->getData();
            SectionInfo info;
            info.name = name;
            info.characteristics = charact;
            info.alignment = static_cast<uint32_t>(sec->getAlignment() ? sec->getAlignment() : align);
            info.secType = type;
            if (type == SectionType::NonInitialized)
            {
                info.isBss = true;
                info.data.resize(data.size(), 0);
            }
            else
            {
                info.data.assign(data.begin(), data.end());
            }

            secInfos.push_back(std::move(info));
            int16_t secIdx = static_cast<int16_t>(secInfos.size()); // 1-based
            sectionIndexMap[type] = secIdx;
            sectionNameIndexMap[name] = secIdx;
        }
    };

    addSec(SectionType::Text, ".text", SCN_TEXT, 16);
    addSec(SectionType::ReadOnly, ".rdata", SCN_RDATA, 16);
    addSec(SectionType::Data, ".data", SCN_DATA, 8);
    addSec(SectionType::NonInitialized, ".bss", SCN_BSS, 8);

    // Stage extra raw sections (e.g., CodeView debug sections).
    std::vector<size_t> extraSectionIndices;
    extraSectionIndices.reserve(m_extraSections.size());
    for (const auto &extra : m_extraSections)
    {
        SectionInfo info;
        info.name = extra.m_name;
        uint32_t baseChars = extra.m_isDebug ? 0x42000040u : 0x40000040u;
        info.characteristics = baseChars | coffAlignFlag(extra.m_alignment);
        info.alignment = static_cast<uint32_t>(extra.m_alignment ? extra.m_alignment : 1);
        info.data = extra.m_data;

        size_t vecIdx = secInfos.size();
        secInfos.push_back(std::move(info));
        int16_t secIdx = static_cast<int16_t>(secInfos.size()); // 1-based
        sectionNameIndexMap[extra.m_name] = secIdx;
        extraSectionIndices.push_back(vecIdx);
    }

    // Build Symbols and COFF String Table
    std::vector<uint8_t> stringTable;
    // Initial 4 bytes are size of string table
    stringTable.resize(4, 0);
    stringTable.reserve(m_symbols.size() * 16 + 4);

    auto addCoffString = [&](std::string_view str) -> uint32_t
    {
        uint32_t offset = static_cast<uint32_t>(stringTable.size());
        stringTable.insert(stringTable.end(), str.begin(), str.end());
        stringTable.push_back(0);
        return offset;
    };

    std::vector<CoffSymbol> symbolRecords;
    symbolRecords.reserve(m_symbols.size());
    std::unordered_map<std::string_view, uint32_t> symIndexMap;

    for (const auto &sym : m_symbols)
    {
        CoffSymbol rec{};
        if (sym.m_name.size() <= 8)
        {
            std::memcpy(rec.N.ShortName, sym.m_name.data(), sym.m_name.size());
        }
        else
        {
            rec.N.LongName.Zeroes = 0;
            rec.N.LongName.Offset = addCoffString(sym.m_name);
        }

        rec.Value = static_cast<uint32_t>(sym.m_offset);
        if (sym.m_section == SectionType::Undefined)
        {
            // Undefined symbols have no section (COFF section number 0).
            rec.SectionNumber = 0;
        }
        else
        {
            auto itSec = sectionIndexMap.find(sym.m_section);
            if (itSec == sectionIndexMap.end())
            {
                // A defined symbol in a section the writer does not emit would silently become
                // section 0, producing a malformed object; fail loudly instead.
                throw std::runtime_error(std::format("CoffWriter: symbol '{}' references a section that is not emitted",
                                                     sym.m_name));
            }
            rec.SectionNumber = itSec->second;
        }
        rec.Type = sym.m_isFunction ? 0x0020 : 0x0000;
        // Note: PE/COFF weak externals (IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105) require a mandatory 18-byte
        // auxiliary symbol record pointing to a default/alias symbol index. ObjectSymbol does not carry
        // an alias target, so weak symbols map to COFF_SYM_CLASS_EXTERNAL / COFF_SYM_CLASS_STATIC here.
        rec.StorageClass = sym.m_isGlobal ? COFF_SYM_CLASS_EXTERNAL : COFF_SYM_CLASS_STATIC;
        rec.NumberOfAuxSymbols = 0;

        uint32_t symIdx = static_cast<uint32_t>(symbolRecords.size());
        symbolRecords.push_back(rec);
        symIndexMap[sym.m_name] = symIdx;
    }

    // Build Relocations for standard sections
    for (const auto &reloc : m_relocs)
    {
        auto itSec = sectionIndexMap.find(reloc.m_section);
        if (itSec == sectionIndexMap.end())
        {
            // Dropping the relocation silently would leave an unresolved field in the object.
            throw std::runtime_error("CoffWriter: relocation references a section that is not emitted");
        }
        size_t secIdx = static_cast<size_t>(itSec->second - 1);
        CoffRelocation r{};
        r.VirtualAddress = static_cast<uint32_t>(reloc.m_offset);
        auto itSym = symIndexMap.find(reloc.m_symbolName);
        if (itSym == symIndexMap.end())
        {
            throw std::runtime_error(
                    std::format("CoffWriter: relocation references unknown symbol '{}'", reloc.m_symbolName));
        }
        r.SymbolTableIndex = itSym->second;
        r.Type = mapCoffRelocType(reloc.m_type);
        secInfos[secIdx].relocs.push_back(r);
    }

    // Build Relocations for extra sections (emitting COFF section symbols on demand)
    std::unordered_map<std::string, uint32_t> sectionSymIndexMap;
    for (size_t i = 0; i < m_extraSections.size(); ++i)
    {
        const auto &extra = m_extraSections[i];
        if (extra.m_relocs.empty())
        {
            continue;
        }

        size_t targetSecVecIdx = extraSectionIndices[i];
        for (const auto &reloc : extra.m_relocs)
        {
            uint32_t symIdx = 0;
            if (reloc.m_targetIsSection)
            {
                auto itSecSym = sectionSymIndexMap.find(reloc.m_targetName);
                if (itSecSym == sectionSymIndexMap.end())
                {
                    auto itSecNum = sectionNameIndexMap.find(reloc.m_targetName);
                    if (itSecNum == sectionNameIndexMap.end())
                    {
                        throw std::runtime_error(std::format(
                                "CoffWriter: extra section '{}' relocation references non-existent section '{}'",
                                extra.m_name,
                                reloc.m_targetName));
                    }

                    CoffSymbol secSym{};
                    if (reloc.m_targetName.size() <= 8)
                    {
                        std::memcpy(secSym.N.ShortName, reloc.m_targetName.data(), reloc.m_targetName.size());
                    }
                    else
                    {
                        secSym.N.LongName.Zeroes = 0;
                        secSym.N.LongName.Offset = addCoffString(reloc.m_targetName);
                    }
                    secSym.Value = 0;
                    secSym.SectionNumber = itSecNum->second;
                    secSym.Type = 0x0000;
                    secSym.StorageClass = COFF_SYM_CLASS_STATIC;
                    secSym.NumberOfAuxSymbols = 0;

                    symIdx = static_cast<uint32_t>(symbolRecords.size());
                    symbolRecords.push_back(secSym);
                    sectionSymIndexMap[reloc.m_targetName] = symIdx;
                }
                else
                {
                    symIdx = itSecSym->second;
                }
            }
            else
            {
                auto itSym = symIndexMap.find(reloc.m_targetName);
                if (itSym == symIndexMap.end())
                {
                    throw std::runtime_error(std::format(
                            "CoffWriter: extra section '{}' relocation references unknown symbol '{}'",
                            extra.m_name,
                            reloc.m_targetName));
                }
                symIdx = itSym->second;
            }

            if (reloc.m_addend != 0)
            {
                auto &secData = secInfos[targetSecVecIdx].data;
                size_t patchBytes = 0;
                switch (reloc.m_type)
                {
                    case TargetCodeRelocationType::Absolute64:
                        patchBytes = 8;
                        break;
                    case TargetCodeRelocationType::Absolute32:
                    case TargetCodeRelocationType::PCRel32:
                    case TargetCodeRelocationType::BranchRel32:
                    case TargetCodeRelocationType::PLTRel32:
                    case TargetCodeRelocationType::GOTPCREL:
                    case TargetCodeRelocationType::SecRel32:
                        patchBytes = 4;
                        break;
                    case TargetCodeRelocationType::SectionIdx16:
                        patchBytes = 2;
                        break;
                    default:
                        throw std::runtime_error(std::format(
                                "CoffWriter: extra section '{}' relocation at offset {} has non-zero addend {} for unsupported type",
                                extra.m_name,
                                reloc.m_offset,
                                reloc.m_addend));
                }

                if (reloc.m_offset > secData.size() || patchBytes > secData.size() - reloc.m_offset)
                {
                    throw std::runtime_error(std::format(
                            "CoffWriter: extra section '{}' relocation at offset {} (width {}) with addend {} exceeds section size {}",
                            extra.m_name,
                            reloc.m_offset,
                            patchBytes,
                            reloc.m_addend,
                            secData.size()));
                }

                if (patchBytes == 8)
                {
                    int64_t val = 0;
                    std::memcpy(&val, secData.data() + reloc.m_offset, 8);
                    val += reloc.m_addend;
                    std::memcpy(secData.data() + reloc.m_offset, &val, 8);
                }
                else if (patchBytes == 4)
                {
                    int32_t val = 0;
                    std::memcpy(&val, secData.data() + reloc.m_offset, 4);
                    val += static_cast<int32_t>(reloc.m_addend);
                    std::memcpy(secData.data() + reloc.m_offset, &val, 4);
                }
                else
                {
                    int16_t val = 0;
                    std::memcpy(&val, secData.data() + reloc.m_offset, 2);
                    val += static_cast<int16_t>(reloc.m_addend);
                    std::memcpy(secData.data() + reloc.m_offset, &val, 2);
                }
            }

            CoffRelocation r{};
            r.VirtualAddress = static_cast<uint32_t>(reloc.m_offset);
            r.SymbolTableIndex = symIdx;
            r.Type = mapCoffRelocType(reloc.m_type);
            secInfos[targetSecVecIdx].relocs.push_back(r);
        }
    }

    // Compute File Layout
    uint32_t headerSize = sizeof(CoffFileHeader) + static_cast<uint32_t>(secInfos.size() * sizeof(CoffSectionHeader));
    uint32_t currentOffset = headerSize;

    std::vector<CoffSectionHeader> sectionHeaders(secInfos.size());

    // Section raw data & headers (long section names allocate into stringTable before strTableSize is finalized)
    for (size_t i = 0; i < secInfos.size(); ++i)
    {
        auto &info = secInfos[i];
        auto &hdr = sectionHeaders[i];

        std::memset(hdr.Name, 0, 8);
        if (info.name.size() <= 8)
        {
            std::memcpy(hdr.Name, info.name.data(), info.name.size());
        }
        else
        {
            uint32_t nameOffset = addCoffString(info.name);
            std::string longName = "/" + std::to_string(nameOffset);
            if (longName.size() > 8)
            {
                throw std::runtime_error(std::format(
                        "CoffWriter: long section name '{}' string table offset '{}' exceeds 8 bytes",
                        info.name,
                        longName));
            }
            std::memcpy(hdr.Name, longName.data(), longName.size());
        }
        hdr.VirtualSize = 0;
        hdr.VirtualAddress = 0;
        hdr.Characteristics = info.characteristics;

        if (info.isBss)
        {
            hdr.SizeOfRawData = static_cast<uint32_t>(info.data.size());
            hdr.PointerToRawData = 0;
        }
        else
        {
            currentOffset = static_cast<uint32_t>(alignTo(currentOffset, 4));
            hdr.SizeOfRawData = static_cast<uint32_t>(info.data.size());
            hdr.PointerToRawData = info.data.empty() ? 0 : currentOffset;
            currentOffset += hdr.SizeOfRawData;
        }
    }

    // Finalize String Table size after all symbols and long section names are added
    uint32_t strTableSize = static_cast<uint32_t>(stringTable.size());
    std::memcpy(stringTable.data(), &strTableSize, 4);

    // Relocations
    for (size_t i = 0; i < secInfos.size(); ++i)
    {
        auto &info = secInfos[i];
        auto &hdr = sectionHeaders[i];

        if (info.relocs.size() >= 0xFFFF)
        {
            hdr.Characteristics |= COFF_SCN_LNK_NRELOC_OVFL;
            CoffRelocation ovfl{};
            ovfl.VirtualAddress = static_cast<uint32_t>(info.relocs.size() + 1);
            ovfl.SymbolTableIndex = 0;
            ovfl.Type = 0;
            info.relocs.insert(info.relocs.begin(), ovfl);

            currentOffset = static_cast<uint32_t>(alignTo(currentOffset, 4));
            hdr.PointerToRelocations = currentOffset;
            hdr.NumberOfRelocations = 0xFFFF;
            currentOffset += static_cast<uint32_t>(info.relocs.size() * sizeof(CoffRelocation));
        }
        else if (!info.relocs.empty())
        {
            currentOffset = static_cast<uint32_t>(alignTo(currentOffset, 4));
            hdr.PointerToRelocations = currentOffset;
            hdr.NumberOfRelocations = static_cast<uint16_t>(info.relocs.size());
            currentOffset += static_cast<uint32_t>(info.relocs.size() * sizeof(CoffRelocation));
        }
        else
        {
            hdr.PointerToRelocations = 0;
            hdr.NumberOfRelocations = 0;
        }
        hdr.PointerToLinenumbers = 0;
        hdr.NumberOfLinenumbers = 0;
    }

    // Symbol Table
    currentOffset = static_cast<uint32_t>(alignTo(currentOffset, 4));
    uint32_t symTableOffset = (symbolRecords.empty() && stringTable.size() <= 4) ? 0 : currentOffset;
    uint32_t totalFileSize =
            currentOffset + static_cast<uint32_t>(symbolRecords.size() * sizeof(CoffSymbol)) + strTableSize;

    fileBuf.resize(totalFileSize, 0);

    // Write File Header
    CoffFileHeader fileHeader{};
    fileHeader.Machine = COFF_MACHINE_AMD64;
    fileHeader.NumberOfSections = static_cast<uint16_t>(secInfos.size());
    fileHeader.TimeDateStamp = 0; // Deterministic
    fileHeader.PointerToSymbolTable = symTableOffset;
    fileHeader.NumberOfSymbols = static_cast<uint32_t>(symbolRecords.size());
    fileHeader.SizeOfOptionalHeader = 0;
    fileHeader.Characteristics = 0;

    std::memcpy(fileBuf.data(), &fileHeader, sizeof(CoffFileHeader));

    // Write Section Headers
    std::memcpy(fileBuf.data() + sizeof(CoffFileHeader),
                sectionHeaders.data(),
                sectionHeaders.size() * sizeof(CoffSectionHeader));

    // Write Section Raw Data
    for (size_t i = 0; i < secInfos.size(); ++i)
    {
        const auto &info = secInfos[i];
        const auto &hdr = sectionHeaders[i];
        if (!info.isBss && hdr.PointerToRawData != 0 && !info.data.empty())
        {
            std::memcpy(fileBuf.data() + hdr.PointerToRawData, info.data.data(), info.data.size());
        }
    }

    // Write Relocations
    for (size_t i = 0; i < secInfos.size(); ++i)
    {
        const auto &info = secInfos[i];
        const auto &hdr = sectionHeaders[i];
        if (hdr.PointerToRelocations != 0 && !info.relocs.empty())
        {
            std::memcpy(fileBuf.data() + hdr.PointerToRelocations,
                        info.relocs.data(),
                        info.relocs.size() * sizeof(CoffRelocation));
        }
    }

    // Write Symbol Table
    if (symTableOffset != 0 && !symbolRecords.empty())
    {
        std::memcpy(fileBuf.data() + symTableOffset, symbolRecords.data(), symbolRecords.size() * sizeof(CoffSymbol));
    }

    // Write String Table
    if (symTableOffset != 0)
    {
        uint32_t strTableOffset = symTableOffset + static_cast<uint32_t>(symbolRecords.size() * sizeof(CoffSymbol));
        std::memcpy(fileBuf.data() + strTableOffset, stringTable.data(), stringTable.size());
    }

    return fileBuf;
}

} // namespace EzCodeEmitter::ObjectFormat
