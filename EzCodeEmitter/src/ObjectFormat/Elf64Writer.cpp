#include "ObjectFormat/Elf64Writer.h"
#include <algorithm>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>

namespace EzCodeEmitter::ObjectFormat
{

namespace
{

#pragma pack(push, 1)
// ELF64 file header (Elf64_Ehdr): identifies the object and locates the section table.
struct Elf64_Ehdr
{
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

// ELF64 section header (Elf64_Shdr): describes one section's type, flags and file location.
struct Elf64_Shdr
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};

// ELF64 symbol table entry (Elf64_Sym): name offset, binding/type, section and value.
struct Elf64_Sym
{
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
};

// ELF64 relocation with explicit addend (Elf64_Rela): offset, symbol+type, addend.
struct Elf64_Rela
{
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
};
#pragma pack(pop)

// Section type (sh_type) values.
constexpr uint32_t SHT_NULL = 0;
constexpr uint32_t SHT_PROGBITS = 1;
constexpr uint32_t SHT_SYMTAB = 2;
constexpr uint32_t SHT_STRTAB = 3;
constexpr uint32_t SHT_RELA = 4;
constexpr uint32_t SHT_NOBITS = 8;

// Section attribute (sh_flags) bits.
constexpr uint64_t SHF_WRITE = 0x1;
constexpr uint64_t SHF_ALLOC = 0x2;
constexpr uint64_t SHF_EXECINSTR = 0x4;
constexpr uint64_t SHF_MERGE = 0x10;
constexpr uint64_t SHF_STRINGS = 0x20;
constexpr uint64_t SHF_INFO_LINK = 0x40;

// Symbol binding (high nibble of st_info) and type (low nibble).
constexpr uint8_t STB_LOCAL = 0;
constexpr uint8_t STB_GLOBAL = 1;
constexpr uint8_t STB_WEAK = 2;
constexpr uint8_t STT_NOTYPE = 0;
constexpr uint8_t STT_OBJECT = 1;
constexpr uint8_t STT_FUNC = 2;
constexpr uint8_t STT_SECTION = 3;

// x86-64 relocation types (R_X86_64_*).
constexpr uint32_t R_X86_64_NONE = 0;
constexpr uint32_t R_X86_64_64 = 1;
constexpr uint32_t R_X86_64_PC32 = 2;
constexpr uint32_t R_X86_64_PLT32 = 4;
constexpr uint32_t R_X86_64_GOTPCREL = 9;
constexpr uint32_t R_X86_64_32 = 10;

// RISC-V relocation types (R_RISCV_*).
constexpr uint16_t EM_RISCV = 243;
constexpr uint32_t R_RISCV_NONE = 0;
constexpr uint32_t R_RISCV_32 = 1;
constexpr uint32_t R_RISCV_64 = 2;
constexpr uint32_t R_RISCV_BRANCH = 16;
constexpr uint32_t R_RISCV_JAL = 17;
constexpr uint32_t R_RISCV_CALL = 18;
constexpr uint32_t R_RISCV_CALL_PLT = 19;
constexpr uint32_t R_RISCV_GOT_HI20 = 20;
constexpr uint32_t R_RISCV_PCREL_HI20 = 23;
constexpr uint32_t R_RISCV_PCREL_LO12_I = 24;
constexpr uint32_t R_RISCV_PCREL_LO12_S = 25;

// Translates a target-agnostic relocation kind into architecture-specific ELF type; NONE = no fixup.
uint32_t mapRelocType(TargetCodeRelocationType type, uint16_t machine)
{
    if (machine == EM_RISCV)
    {
        switch (type)
        {
            case TargetCodeRelocationType::Absolute64:
                return R_RISCV_64;
            case TargetCodeRelocationType::Absolute32:
                return R_RISCV_32;
            case TargetCodeRelocationType::PCRel32:
                return R_RISCV_PCREL_HI20;
            case TargetCodeRelocationType::BranchRel32:
                return R_RISCV_CALL_PLT;
            case TargetCodeRelocationType::GOTPCREL:
                return R_RISCV_GOT_HI20;
            case TargetCodeRelocationType::PLTRel32:
                return R_RISCV_CALL_PLT;
            default:
                return R_RISCV_NONE;
        }
    }

    switch (type)
    {
        case TargetCodeRelocationType::Absolute64:
            return R_X86_64_64;
        case TargetCodeRelocationType::Absolute32:
            return R_X86_64_32;
        case TargetCodeRelocationType::PCRel32:
            return R_X86_64_PC32;
        case TargetCodeRelocationType::BranchRel32:
            return R_X86_64_PLT32;
        case TargetCodeRelocationType::GOTPCREL:
            return R_X86_64_GOTPCREL;
        case TargetCodeRelocationType::PLTRel32:
            return R_X86_64_PLT32;
        default:
            return R_X86_64_NONE;
    }
}

// Rounds offset up to the next multiple of alignment (alignment assumed power-of-two).
uint64_t alignTo(uint64_t offset, uint64_t alignment)
{
    if (alignment <= 1)
        return offset;
    return (offset + alignment - 1) & ~(alignment - 1);
}

// Grows buf with padByte until it reaches targetOffset.
void padTo(std::vector<uint8_t> &buf, uint64_t targetOffset, uint8_t padByte = 0)
{
    if (buf.size() < targetOffset)
    {
        buf.resize(targetOffset, padByte);
    }
}

// Interns str into a string table, returning its byte offset (0 for the empty initial entry).
uint32_t addString(std::vector<uint8_t> &strtab, std::string_view str)
{
    if (strtab.empty())
    {
        strtab.push_back(0); // initial null byte
    }
    if (str.empty())
    {
        return 0;
    }
    uint32_t offset = static_cast<uint32_t>(strtab.size());
    strtab.insert(strtab.end(), str.begin(), str.end());
    strtab.push_back(0);
    return offset;
}

} // namespace

void Elf64Writer::addSymbol(const ObjectSymbol &sym) { m_symbols.push_back(sym); }

void Elf64Writer::addRelocation(const ObjectRelocEntry &reloc) { m_relocs.push_back(reloc); }

void Elf64Writer::addExtraSection(const ExtraObjectSection &section) { m_extraSections.push_back(section); }

void Elf64Writer::clear()
{
    m_symbols.clear();
    m_relocs.clear();
    m_extraSections.clear();
}

std::vector<uint8_t> Elf64Writer::write(const std::pmr::unordered_map<SectionType, CodeSection *> &sections)
{
    std::vector<uint8_t> fileBuf;
    std::vector<uint8_t> shstrtab;
    std::vector<uint8_t> strtab;
    strtab.reserve(m_symbols.size() * 16 + 1);
    shstrtab.reserve(64);

    addString(shstrtab, "");
    addString(strtab, "");

    // Structure defining internal section representation during layout
    struct SectionDesc
    {
        std::string name;
        uint32_t type{ 0 };
        uint64_t flags{ 0 };
        uint64_t align{ 1 };
        uint64_t entsize{ 0 };
        uint32_t link{ 0 };
        uint32_t info{ 0 };
        std::vector<uint8_t> data;
        bool isNoBits{ false };
        uint64_t noBitsSize{ 0 };
        SectionType secType{ SectionType::Text };
    };

    std::vector<SectionDesc> sectionDescs;
    sectionDescs.reserve(8);

    // 0. NULL section
    sectionDescs.push_back(SectionDesc{ "", SHT_NULL, 0, 0, 0, 0, 0, {}, false, 0 });

    // Mapping from SectionType to 1-based section header index
    std::unordered_map<SectionType, uint16_t> sectionIndexMap;
    // Mapping from section name to 1-based section header index (for all emitted sections)
    std::unordered_map<std::string, uint16_t> sectionNameIndexMap;

    // Copies a code section (if present) into the section list, recording NOBITS for .bss.
    auto processCodeSection =
            [&](SectionType type, const char *secName, uint32_t shType, uint64_t flags, uint64_t alignment)
    {
        auto it = sections.find(type);
        if (it != sections.end() && it->second)
        {
            CodeSection *sec = it->second;
            std::span<const uint8_t> data = sec->getData();
            SectionDesc desc;
            desc.name = secName;
            desc.type = shType;
            desc.flags = flags;
            desc.align = sec->getAlignment() ? sec->getAlignment() : alignment;
            desc.secType = type;

            if (type == SectionType::NonInitialized)
            {
                desc.isNoBits = true;
                desc.noBitsSize = data.size();
            }
            else
            {
                desc.data.assign(data.begin(), data.end());
            }

            uint16_t idx = static_cast<uint16_t>(sectionDescs.size());
            sectionNameIndexMap[desc.name] = idx;
            sectionDescs.push_back(std::move(desc));
            sectionIndexMap[type] = idx;
        }
    };

    processCodeSection(SectionType::Text, ".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 16);
    processCodeSection(SectionType::ReadOnly, ".rodata", SHT_PROGBITS, SHF_ALLOC, 16);
    processCodeSection(SectionType::Data, ".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 8);
    processCodeSection(SectionType::NonInitialized, ".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 8);

    // Emit extra raw sections (e.g., DWARF debug sections).
    std::vector<uint16_t> extraSectionIndices;
    extraSectionIndices.reserve(m_extraSections.size());
    for (const auto &extra : m_extraSections)
    {
        SectionDesc desc;
        desc.name = extra.m_name;
        desc.type = SHT_PROGBITS;
        uint64_t flags = 0;
        if (extra.m_isAlloc)
        {
            flags |= SHF_ALLOC;
        }
        if (extra.m_isMergeStrings)
        {
            flags |= (SHF_MERGE | SHF_STRINGS);
            desc.entsize = 1;
        }
        desc.flags = flags;
        desc.align = extra.m_alignment ? extra.m_alignment : 1;
        desc.data = extra.m_data;

        uint16_t idx = static_cast<uint16_t>(sectionDescs.size());
        sectionNameIndexMap[desc.name] = idx;
        sectionDescs.push_back(std::move(desc));
        extraSectionIndices.push_back(idx);
    }

    // Determine whether any extra section relocation targets a section (requiring STT_SECTION symbols).
    bool needSectionSymbols = false;
    for (const auto &extra : m_extraSections)
    {
        for (const auto &reloc : extra.m_relocs)
        {
            if (reloc.m_targetIsSection)
            {
                needSectionSymbols = true;
                break;
            }
        }
        if (needSectionSymbols)
        {
            break;
        }
    }

    // Build Symbols
    // In ELF, all symbols with STB_LOCAL binding must precede weak and global symbols.
    // sh_info of the SHT_SYMTAB section holds the index of the first non-local symbol (1 + numLocalSymbols).
    std::vector<EzCodeEmitter::ObjectFormat::ObjectSymbol> sortedSymbols = m_symbols;
    std::stable_partition(sortedSymbols.begin(), sortedSymbols.end(), [](const auto &sym) {
        return !sym.m_isWeak && !sym.m_isGlobal;
    });

    uint32_t numLocalSymbols = 0;
    for (const auto &sym : sortedSymbols)
    {
        if (!sym.m_isWeak && !sym.m_isGlobal)
        {
            numLocalSymbols++;
        }
    }

    std::vector<Elf64_Sym> symTable;
    symTable.reserve(sortedSymbols.size() + 1 + (needSectionSymbols ? sectionDescs.size() : 0));
    // 0. NULL symbol
    symTable.push_back(Elf64_Sym{ 0, 0, 0, 0, 0, 0 });

    std::unordered_map<std::string, uint32_t> sectionSymIndexMap;
    if (needSectionSymbols)
    {
        for (size_t secIdx = 1; secIdx < sectionDescs.size(); ++secIdx)
        {
            Elf64_Sym secSym{};
            secSym.st_name = 0;
            secSym.st_info = (STB_LOCAL << 4) | (STT_SECTION & 0x0F);
            secSym.st_other = 0;
            secSym.st_shndx = static_cast<uint16_t>(secIdx);
            secSym.st_value = 0;
            secSym.st_size = 0;

            uint32_t symIdx = static_cast<uint32_t>(symTable.size());
            symTable.push_back(secSym);
            sectionSymIndexMap[sectionDescs[secIdx].name] = symIdx;
            numLocalSymbols++;
        }
    }

    std::unordered_map<std::string_view, uint32_t> symIndexMap;

    for (const auto &sym : sortedSymbols)
    {
        Elf64_Sym elfSym{};
        elfSym.st_name = addString(strtab, sym.m_name);
        uint8_t bind = sym.m_isWeak ? STB_WEAK : (sym.m_isGlobal ? STB_GLOBAL : STB_LOCAL);
        uint8_t symType = sym.m_isFunction ? STT_FUNC : STT_OBJECT;
        // st_info packs binding in the high nibble and symbol type in the low nibble.
        elfSym.st_info = (bind << 4) | (symType & 0x0F);
        elfSym.st_other = 0;
        if (sym.m_section == SectionType::Undefined)
        {
            // Undefined symbols have no section (SHN_UNDEF).
            elfSym.st_shndx = 0;
        }
        else
        {
            auto itSec = sectionIndexMap.find(sym.m_section);
            if (itSec == sectionIndexMap.end())
            {
                // A defined symbol in a section the writer does not emit would silently become
                // SHN_UNDEF, producing a malformed object; fail loudly instead.
                throw std::runtime_error(std::format("Elf64Writer: symbol '{}' references a section that is not emitted",
                                                     sym.m_name));
            }
            elfSym.st_shndx = itSec->second;
        }
        elfSym.st_value = sym.m_offset;
        elfSym.st_size = sym.m_size;

        uint32_t symIdx = static_cast<uint32_t>(symTable.size());
        symTable.push_back(elfSym);
        symIndexMap[sym.m_name] = symIdx;
    }

    // Build Relocations for standard sections, grouped by SectionType.
    if (!m_relocs.empty())
    {
        std::unordered_map<SectionType, std::vector<Elf64_Rela>> relocsBySection;
        for (const auto &reloc : m_relocs)
        {
            auto itSec = sectionIndexMap.find(reloc.m_section);
            if (itSec == sectionIndexMap.end())
            {
                throw std::runtime_error(
                        std::format("Elf64Writer: relocation references a section that is not emitted"));
            }

            auto itSym = symIndexMap.find(reloc.m_symbolName);
            if (itSym == symIndexMap.end())
            {
                throw std::runtime_error(
                        std::format("Elf64Writer: relocation references unknown symbol '{}'", reloc.m_symbolName));
            }

            Elf64_Rela r{};
            r.r_offset = reloc.m_offset;
            uint32_t symIdx = itSym->second;
            uint32_t rType = mapRelocType(reloc.m_type, m_machine);
            // r_info packs the symbol index in the high 32 bits and the type in the low 32.
            r.r_info = (static_cast<uint64_t>(symIdx) << 32) | (rType & 0xFFFFFFFFULL);
            r.r_addend = reloc.m_addend;
            relocsBySection[reloc.m_section].push_back(r);
        }

        constexpr SectionType stdSecOrder[] = {
            SectionType::Text,
            SectionType::ReadOnly,
            SectionType::Data,
            SectionType::NonInitialized
        };
        for (SectionType secType : stdSecOrder)
        {
            auto itRelocs = relocsBySection.find(secType);
            if (itRelocs == relocsBySection.end() || itRelocs->second.empty())
            {
                continue;
            }
            uint16_t targetSecIdx = sectionIndexMap[secType];
            const auto &relaTable = itRelocs->second;

            SectionDesc relaDesc;
            relaDesc.name = ".rela" + sectionDescs[targetSecIdx].name;
            relaDesc.type = SHT_RELA;
            relaDesc.flags = SHF_INFO_LINK;
            relaDesc.align = 8;
            relaDesc.entsize = sizeof(Elf64_Rela);
            relaDesc.info = targetSecIdx;
            relaDesc.data.resize(relaTable.size() * sizeof(Elf64_Rela));
            std::memcpy(relaDesc.data.data(), relaTable.data(), relaDesc.data.size());
            sectionDescs.push_back(std::move(relaDesc));
        }
    }

    // Build Relocations for extra sections (if any)
    for (size_t i = 0; i < m_extraSections.size(); ++i)
    {
        const auto &extra = m_extraSections[i];
        if (extra.m_relocs.empty())
        {
            continue;
        }

        std::vector<Elf64_Rela> relaTable;
        relaTable.reserve(extra.m_relocs.size());
        for (const auto &reloc : extra.m_relocs)
        {
            uint32_t symIdx = 0;
            if (reloc.m_targetIsSection)
            {
                auto itSecSym = sectionSymIndexMap.find(reloc.m_targetName);
                if (itSecSym == sectionSymIndexMap.end())
                {
                    throw std::runtime_error(std::format(
                            "Elf64Writer: extra section '{}' relocation references non-existent section '{}'",
                            extra.m_name,
                            reloc.m_targetName));
                }
                symIdx = itSecSym->second;
            }
            else
            {
                auto itSym = symIndexMap.find(reloc.m_targetName);
                if (itSym == symIndexMap.end())
                {
                    throw std::runtime_error(std::format(
                            "Elf64Writer: extra section '{}' relocation references unknown symbol '{}'",
                            extra.m_name,
                            reloc.m_targetName));
                }
                symIdx = itSym->second;
            }

            Elf64_Rela r{};
            r.r_offset = reloc.m_offset;
            uint32_t rType = mapRelocType(reloc.m_type, m_machine);
            r.r_info = (static_cast<uint64_t>(symIdx) << 32) | (rType & 0xFFFFFFFFULL);
            r.r_addend = reloc.m_addend;
            relaTable.push_back(r);
        }

        SectionDesc relaDesc;
        relaDesc.name = ".rela" + extra.m_name;
        relaDesc.type = SHT_RELA;
        relaDesc.flags = SHF_INFO_LINK;
        relaDesc.align = 8;
        relaDesc.entsize = sizeof(Elf64_Rela);
        relaDesc.info = extraSectionIndices[i];
        relaDesc.data.resize(relaTable.size() * sizeof(Elf64_Rela));
        std::memcpy(relaDesc.data.data(), relaTable.data(), relaDesc.data.size());
        sectionDescs.push_back(std::move(relaDesc));
    }

    // Append .symtab
    uint32_t symtabIndex = static_cast<uint32_t>(sectionDescs.size());
    {
        SectionDesc symDesc;
        symDesc.name = ".symtab";
        symDesc.type = SHT_SYMTAB;
        symDesc.flags = 0;
        symDesc.align = 8;
        symDesc.entsize = sizeof(Elf64_Sym);
        symDesc.info = 1 + numLocalSymbols; // 1 + number of local symbols (index of first non-local symbol)
        symDesc.data.resize(symTable.size() * sizeof(Elf64_Sym));
        std::memcpy(symDesc.data.data(), symTable.data(), symDesc.data.size());
        sectionDescs.push_back(std::move(symDesc));
    }

    // Append .strtab
    uint32_t strtabIndex = static_cast<uint32_t>(sectionDescs.size());
    {
        SectionDesc strDesc;
        strDesc.name = ".strtab";
        strDesc.type = SHT_STRTAB;
        strDesc.flags = 0;
        strDesc.align = 1;
        strDesc.entsize = 0;
        strDesc.data = std::move(strtab);
        sectionDescs.push_back(std::move(strDesc));
    }

    // Link .symtab to .strtab
    sectionDescs[symtabIndex].link = strtabIndex;

    // Link .rela.text to .symtab
    for (auto &desc : sectionDescs)
    {
        if (desc.type == SHT_RELA)
        {
            desc.link = symtabIndex;
        }
    }

    // Append .shstrtab
    uint32_t shstrtabIndex = static_cast<uint32_t>(sectionDescs.size());
    // Pre-calculate section header string table strings
    std::vector<uint32_t> shNameOffsets(sectionDescs.size() + 1, 0);
    for (size_t i = 0; i < sectionDescs.size(); ++i)
    {
        shNameOffsets[i] = addString(shstrtab, sectionDescs[i].name);
    }
    shNameOffsets[shstrtabIndex] = addString(shstrtab, ".shstrtab");

    {
        SectionDesc shstrDesc;
        shstrDesc.name = ".shstrtab";
        shstrDesc.type = SHT_STRTAB;
        shstrDesc.flags = 0;
        shstrDesc.align = 1;
        shstrDesc.entsize = 0;
        shstrDesc.data = std::move(shstrtab);
        sectionDescs.push_back(std::move(shstrDesc));
    }

    // =========================================================================
    // Layout and Offset Calculation
    // =========================================================================

    uint64_t currentFileOffset = sizeof(Elf64_Ehdr);

    std::vector<Elf64_Shdr> sectionHeaders(sectionDescs.size());

    for (size_t i = 0; i < sectionDescs.size(); ++i)
    {
        auto &desc = sectionDescs[i];
        auto &shdr = sectionHeaders[i];

        shdr.sh_name = shNameOffsets[i];
        shdr.sh_type = desc.type;
        shdr.sh_flags = desc.flags;
        shdr.sh_addr = 0;
        shdr.sh_addralign = desc.align;
        shdr.sh_entsize = desc.entsize;
        shdr.sh_link = desc.link;
        shdr.sh_info = desc.info;

        if (desc.type == SHT_NULL)
        {
            shdr.sh_offset = 0;
            shdr.sh_size = 0;
            continue;
        }

        if (desc.isNoBits)
        {
            currentFileOffset = alignTo(currentFileOffset, desc.align);
            shdr.sh_offset = currentFileOffset;
            shdr.sh_size = desc.noBitsSize;
        }
        else
        {
            currentFileOffset = alignTo(currentFileOffset, desc.align);
            shdr.sh_offset = currentFileOffset;
            shdr.sh_size = desc.data.size();
            currentFileOffset += desc.data.size();
        }
    }

    // Align section header table to 8 bytes
    currentFileOffset = alignTo(currentFileOffset, 8);
    uint64_t shoff = currentFileOffset;
    uint64_t totalFileSize = shoff + (sectionHeaders.size() * sizeof(Elf64_Shdr));

    fileBuf.resize(totalFileSize, 0);

    // Write ELF Header
    Elf64_Ehdr ehdr{};
    ehdr.e_ident[0] = 0x7f;
    ehdr.e_ident[1] = 'E';
    ehdr.e_ident[2] = 'L';
    ehdr.e_ident[3] = 'F';
    ehdr.e_ident[4] = 2; // ELFCLASS64
    ehdr.e_ident[5] = 1; // ELFDATA2LSB (little endian)
    ehdr.e_ident[6] = 1; // EV_CURRENT
    ehdr.e_ident[7] = 0; // ELFOSABI_NONE
    ehdr.e_type = 1;         // ET_REL
    ehdr.e_machine = m_machine;
    ehdr.e_version = 1;
    ehdr.e_entry = 0;
    ehdr.e_phoff = 0;
    ehdr.e_shoff = shoff;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(Elf64_Ehdr);
    ehdr.e_phentsize = 0;
    ehdr.e_phnum = 0;
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = static_cast<uint16_t>(sectionHeaders.size());
    ehdr.e_shstrndx = static_cast<uint16_t>(shstrtabIndex);

    std::memcpy(fileBuf.data(), &ehdr, sizeof(Elf64_Ehdr));

    // Write Section Contents
    for (size_t i = 0; i < sectionDescs.size(); ++i)
    {
        const auto &desc = sectionDescs[i];
        const auto &shdr = sectionHeaders[i];
        if (!desc.isNoBits && !desc.data.empty())
        {
            std::memcpy(fileBuf.data() + shdr.sh_offset, desc.data.data(), desc.data.size());
        }
    }

    // Write Section Header Table
    std::memcpy(fileBuf.data() + shoff, sectionHeaders.data(), sectionHeaders.size() * sizeof(Elf64_Shdr));

    return fileBuf;
}

} // namespace EzCodeEmitter::ObjectFormat
