#include "Debug/Dwarf5Emitter.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace EzCodeEmitter::Debug
{

namespace
{

// DWARF v5 Unit Types
constexpr uint8_t DW_UT_compile = 0x01;

// DWARF Tags
constexpr uint64_t DW_TAG_array_type = 0x01;
constexpr uint64_t DW_TAG_enumeration_type = 0x04;
constexpr uint64_t DW_TAG_formal_parameter = 0x05;
constexpr uint64_t DW_TAG_member = 0x0d;
constexpr uint64_t DW_TAG_pointer_type = 0x0f;
constexpr uint64_t DW_TAG_compile_unit = 0x11;
constexpr uint64_t DW_TAG_structure_type = 0x13;
constexpr uint64_t DW_TAG_subroutine_type = 0x15;
constexpr uint64_t DW_TAG_typedef = 0x16;
constexpr uint64_t DW_TAG_union_type = 0x17;
constexpr uint64_t DW_TAG_unspecified_parameters = 0x18;
constexpr uint64_t DW_TAG_subrange_type = 0x21;
constexpr uint64_t DW_TAG_base_type = 0x24;
constexpr uint64_t DW_TAG_enumerator = 0x28;
constexpr uint64_t DW_TAG_subprogram = 0x2e;
constexpr uint64_t DW_TAG_variable = 0x34;

// DWARF Children flags
constexpr uint8_t DW_CHILDREN_no = 0;
constexpr uint8_t DW_CHILDREN_yes = 1;

// DWARF Attributes
constexpr uint64_t DW_AT_location = 0x02;
constexpr uint64_t DW_AT_name = 0x03;
constexpr uint64_t DW_AT_byte_size = 0x0b;
constexpr uint64_t DW_AT_bit_size = 0x0d;
constexpr uint64_t DW_AT_stmt_list = 0x10;
constexpr uint64_t DW_AT_low_pc = 0x11;
constexpr uint64_t DW_AT_high_pc = 0x12;
constexpr uint64_t DW_AT_language = 0x13;
constexpr uint64_t DW_AT_comp_dir = 0x1b;
constexpr uint64_t DW_AT_const_value = 0x1c;
constexpr uint64_t DW_AT_producer = 0x25;
constexpr uint64_t DW_AT_upper_bound = 0x2f;
constexpr uint64_t DW_AT_data_member_location = 0x38;
constexpr uint64_t DW_AT_decl_file = 0x3a;
constexpr uint64_t DW_AT_decl_line = 0x3b;
constexpr uint64_t DW_AT_encoding = 0x3e;
constexpr uint64_t DW_AT_external = 0x3f;
constexpr uint64_t DW_AT_frame_base = 0x40;
constexpr uint64_t DW_AT_type = 0x49;
constexpr uint64_t DW_AT_data_bit_offset = 0x6b;
constexpr uint64_t DW_AT_linkage_name = 0x6e;

// DWARF Forms
constexpr uint64_t DW_FORM_addr = 0x01;
constexpr uint64_t DW_FORM_data2 = 0x05;
constexpr uint64_t DW_FORM_data4 = 0x06;
constexpr uint64_t DW_FORM_data8 = 0x07;
constexpr uint64_t DW_FORM_data1 = 0x0b;
constexpr uint64_t DW_FORM_sdata = 0x0d;
constexpr uint64_t DW_FORM_strp = 0x0e;
constexpr uint64_t DW_FORM_udata = 0x0f;
constexpr uint64_t DW_FORM_ref4 = 0x13;
constexpr uint64_t DW_FORM_sec_offset = 0x17;
constexpr uint64_t DW_FORM_exprloc = 0x18;
constexpr uint64_t DW_FORM_flag_present = 0x19;
constexpr uint64_t DW_FORM_line_strp = 0x1f;

// DWARF Base Type Encodings
constexpr uint8_t DW_ATE_address = 0x01;
constexpr uint8_t DW_ATE_boolean = 0x02;
constexpr uint8_t DW_ATE_float = 0x04;
constexpr uint8_t DW_ATE_signed = 0x05;
constexpr uint8_t DW_ATE_signed_char = 0x06;
constexpr uint8_t DW_ATE_unsigned = 0x07;
constexpr uint8_t DW_ATE_unsigned_char = 0x08;

// DWARF Expression Opcodes
constexpr uint8_t DW_OP_addr = 0x03;
constexpr uint8_t DW_OP_reg0 = 0x50;
constexpr uint8_t DW_OP_breg0 = 0x70;
constexpr uint8_t DW_OP_regx = 0x90;
constexpr uint8_t DW_OP_fbreg = 0x91;

// DWARF Line Number Content Types (v5)
constexpr uint64_t DW_LNCT_path = 0x01;
constexpr uint64_t DW_LNCT_directory_index = 0x02;

// DWARF Line Number Standard Opcodes
constexpr uint8_t DW_LNS_copy = 0x01;
constexpr uint8_t DW_LNS_advance_pc = 0x02;
constexpr uint8_t DW_LNS_advance_line = 0x03;
constexpr uint8_t DW_LNS_set_file = 0x04;
constexpr uint8_t DW_LNS_set_column = 0x05;
constexpr uint8_t DW_LNS_negate_stmt = 0x06;
constexpr uint8_t DW_LNS_set_prologue_end = 0x0a;
constexpr uint8_t DW_LNS_set_epilogue_begin = 0x0b;

// DWARF Line Number Extended Opcodes
constexpr uint8_t DW_LNE_end_sequence = 0x01;
constexpr uint8_t DW_LNE_set_address = 0x02;

// DWARF CFA Opcodes
constexpr uint8_t DW_CFA_nop = 0x00;
constexpr uint8_t DW_CFA_advance_loc1 = 0x02;
constexpr uint8_t DW_CFA_offset_extended = 0x05;
constexpr uint8_t DW_CFA_def_cfa = 0x0c;
constexpr uint8_t DW_CFA_def_cfa_register = 0x0d;
constexpr uint8_t DW_CFA_def_cfa_offset = 0x0e;
constexpr uint8_t DW_CFA_offset = 0x80;

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

void patchU32(std::vector<uint8_t> &out, size_t offset, uint32_t v)
{
    for (int i = 0; i < 4; ++i)
    {
        out[offset + i] = static_cast<uint8_t>((v >> (i * 8)) & 0xFFU);
    }
}

class StringTableBuilder
{
  public:
    StringTableBuilder() { m_data.push_back(0); }

    uint32_t add(std::string_view str)
    {
        if (str.empty())
        {
            return 0;
        }
        std::string key(str);
        auto it = m_offsets.find(key);
        if (it != m_offsets.end())
        {
            return it->second;
        }
        const uint32_t offset = static_cast<uint32_t>(m_data.size());
        m_data.insert(m_data.end(), str.begin(), str.end());
        m_data.push_back(0);
        m_offsets.emplace(std::move(key), offset);
        return offset;
    }

    std::vector<uint8_t> takeData() { return std::move(m_data); }

  private:
    std::vector<uint8_t> m_data;
    std::unordered_map<std::string, uint32_t> m_offsets;
};

struct AbbrevSpec
{
    uint64_t tag{ 0 };
    bool hasChildren{ false };
    std::vector<std::pair<uint64_t, uint64_t>> attrs;

    bool operator==(const AbbrevSpec &other) const
    {
        return tag == other.tag && hasChildren == other.hasChildren && attrs == other.attrs;
    }
};

class AbbrevTableBuilder
{
  public:
    uint32_t getOrAdd(const AbbrevSpec &spec)
    {
        for (size_t i = 0; i < m_specs.size(); ++i)
        {
            if (m_specs[i] == spec)
            {
                return static_cast<uint32_t>(i + 1);
            }
        }
        m_specs.push_back(spec);
        return static_cast<uint32_t>(m_specs.size());
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> out;
        for (size_t i = 0; i < m_specs.size(); ++i)
        {
            const uint32_t code = static_cast<uint32_t>(i + 1);
            const auto &spec = m_specs[i];
            Dwarf5Emitter::appendUleb128(out, code);
            Dwarf5Emitter::appendUleb128(out, spec.tag);
            appendU8(out, spec.hasChildren ? DW_CHILDREN_yes : DW_CHILDREN_no);
            for (const auto &[attr, form] : spec.attrs)
            {
                Dwarf5Emitter::appendUleb128(out, attr);
                Dwarf5Emitter::appendUleb128(out, form);
            }
            appendU8(out, 0);
            appendU8(out, 0);
        }
        appendU8(out, 0); // Table terminator
        return out;
    }

  private:
    std::vector<AbbrevSpec> m_specs;
};

uint8_t mapBaseEncoding(EzCore::Debug::DebugBaseEncoding enc, EzCore::RttiTypeKind kind)
{
    switch (enc)
    {
        case EzCore::Debug::DebugBaseEncoding::Address:
            return DW_ATE_address;
        case EzCore::Debug::DebugBaseEncoding::Boolean:
            return DW_ATE_boolean;
        case EzCore::Debug::DebugBaseEncoding::Float:
            return DW_ATE_float;
        case EzCore::Debug::DebugBaseEncoding::Signed:
            return DW_ATE_signed;
        case EzCore::Debug::DebugBaseEncoding::SignedChar:
            return DW_ATE_signed_char;
        case EzCore::Debug::DebugBaseEncoding::Unsigned:
            return DW_ATE_unsigned;
        case EzCore::Debug::DebugBaseEncoding::UnsignedChar:
            return DW_ATE_unsigned_char;
        case EzCore::Debug::DebugBaseEncoding::None:
        default:
            break;
    }

    switch (kind)
    {
        case EzCore::RttiTypeKind::Boolean:
            return DW_ATE_boolean;
        case EzCore::RttiTypeKind::Float:
            return DW_ATE_float;
        case EzCore::RttiTypeKind::UnsignedInt:
            return DW_ATE_unsigned;
        case EzCore::RttiTypeKind::SignedInt:
        default:
            return DW_ATE_signed;
    }
}

void emitStrp(std::vector<uint8_t> &infoData,
              std::vector<ObjectFormat::ExtraSectionReloc> &infoRelocs,
              StringTableBuilder &strTab,
              std::string_view str)
{
    const uint32_t strOff = strTab.add(str);
    ObjectFormat::ExtraSectionReloc rel{};
    rel.m_offset = infoData.size();
    rel.m_targetName = ".debug_str";
    rel.m_targetIsSection = true;
    rel.m_type = TargetCodeRelocationType::Absolute32;
    rel.m_addend = static_cast<int64_t>(strOff);
    infoRelocs.push_back(std::move(rel));
    appendU32(infoData, 0);
}

void emitLineStrp(std::vector<uint8_t> &lineData,
                  std::vector<ObjectFormat::ExtraSectionReloc> &lineRelocs,
                  StringTableBuilder &lineStrTab,
                  std::string_view str)
{
    const uint32_t strOff = lineStrTab.add(str);
    ObjectFormat::ExtraSectionReloc rel{};
    rel.m_offset = lineData.size();
    rel.m_targetName = ".debug_line_str";
    rel.m_targetIsSection = true;
    rel.m_type = TargetCodeRelocationType::Absolute32;
    rel.m_addend = static_cast<int64_t>(strOff);
    lineRelocs.push_back(std::move(rel));
    appendU32(lineData, 0);
}

void emitTextAddr64(std::vector<uint8_t> &secData,
                    std::vector<ObjectFormat::ExtraSectionReloc> &secRelocs,
                    uint64_t textOffset)
{
    ObjectFormat::ExtraSectionReloc rel{};
    rel.m_offset = secData.size();
    rel.m_targetName = ".text";
    rel.m_targetIsSection = true;
    rel.m_type = TargetCodeRelocationType::Absolute64;
    rel.m_addend = static_cast<int64_t>(textOffset);
    secRelocs.push_back(std::move(rel));
    appendU64(secData, 0);
}

std::vector<uint8_t> buildLocationExpr(const EzCore::Debug::DebugVarLocation &loc,
                                       EzCore::Debug::DebugCompileUnit::TargetArch arch,
                                       std::string &outGlobalSym)
{
    outGlobalSym.clear();
    std::vector<uint8_t> expr;
    switch (loc.kind)
    {
        case EzCore::Debug::DebugLocationKind::FrameBaseOffset:
            appendU8(expr, DW_OP_fbreg);
            Dwarf5Emitter::appendSleb128(expr, loc.offset);
            break;
        case EzCore::Debug::DebugLocationKind::StackPointerOffset:
        {
            const uint8_t spReg = (arch == EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64) ? 2 : 7;
            appendU8(expr, static_cast<uint8_t>(DW_OP_breg0 + spReg));
            Dwarf5Emitter::appendSleb128(expr, loc.offset);
            break;
        }
        case EzCore::Debug::DebugLocationKind::Register:
        {
            const uint16_t dwarfReg =
                    (arch == EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64)
                            ? EzCore::Debug::mapRiscV64ToDwarfReg(loc.regId, loc.isFpr)
                            : EzCore::Debug::mapX86_64ToDwarfReg(loc.regId, loc.isFpr);
            if (dwarfReg != 0xFFFF)
            {
                if (dwarfReg < 32)
                {
                    appendU8(expr, static_cast<uint8_t>(DW_OP_reg0 + dwarfReg));
                }
                else
                {
                    appendU8(expr, DW_OP_regx);
                    Dwarf5Emitter::appendUleb128(expr, dwarfReg);
                }
            }
            break;
        }
        case EzCore::Debug::DebugLocationKind::GlobalSymbol:
            if (!loc.symbolName.empty())
            {
                appendU8(expr, DW_OP_addr);
                outGlobalSym = loc.symbolName;
            }
            break;
        case EzCore::Debug::DebugLocationKind::None:
        default:
            break;
    }
    return expr;
}

// Maps a 1-based DebugCompileUnit fileId to its 0-based DWARF v5 .debug_line file index.
struct DwarfFileMap
{
    std::unordered_map<uint32_t, uint32_t> fileIdToDwarfIdx;
    bool hasMainFileInFiles{ false };
    uint32_t mainFileId{ 0 };

    uint32_t map(uint32_t fileId) const
    {
        auto it = fileIdToDwarfIdx.find(fileId);
        return (it != fileIdToDwarfIdx.end()) ? it->second : 0;
    }
};

DwarfFileMap buildDwarfFileMap(const EzCore::Debug::DebugCompileUnit &cu)
{
    DwarfFileMap fm;
    fm.fileIdToDwarfIdx[0] = 0;

    std::string normMain = cu.mainFile;
    std::replace(normMain.begin(), normMain.end(), '\\', '/');

    uint32_t nextIdx = 1;
    for (const auto &f : cu.files)
    {
        if (!normMain.empty() && f.fullPath == normMain && !fm.hasMainFileInFiles)
        {
            fm.hasMainFileInFiles = true;
            fm.mainFileId = f.fileId;
            fm.fileIdToDwarfIdx[f.fileId] = 0;
        }
        else
        {
            fm.fileIdToDwarfIdx[f.fileId] = nextIdx++;
        }
    }
    return fm;
}

ObjectFormat::ExtraObjectSection buildDebugLineSection(const EzCore::Debug::DebugCompileUnit &cu,
                                                       const DwarfFileMap &fileMap,
                                                       StringTableBuilder &lineStrTab)
{
    ObjectFormat::ExtraObjectSection sec;
    sec.m_name = ".debug_line";
    sec.m_alignment = 1;
    sec.m_isDebug = true;

    std::vector<uint8_t> &out = sec.m_data;
    const uint8_t minInstLen = (cu.arch == EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64) ? 2 : 1;

    // Collect directories: entry 0 is compDir
    std::vector<std::string> dirs;
    std::unordered_map<std::string, uint32_t> dirIndexMap;
    auto addDir = [&](const std::string &dir) -> uint32_t
    {
        auto it = dirIndexMap.find(dir);
        if (it != dirIndexMap.end())
        {
            return it->second;
        }
        const uint32_t idx = static_cast<uint32_t>(dirs.size());
        dirs.push_back(dir);
        dirIndexMap.emplace(dir, idx);
        return idx;
    };
    addDir(cu.compDir);
    for (const auto &f : cu.files)
    {
        addDir(f.directory);
    }

    // DWARF v5 Line Number Program Header
    appendU32(out, 0); // unit_length placeholder (offset 0)
    appendU16(out, 5); // version = 5
    appendU8(out, cu.pointerSize);
    appendU8(out, 0); // segment_selector_size

    const size_t headerLenPos = out.size();
    appendU32(out, 0); // header_length placeholder
    const size_t headerStart = out.size();

    appendU8(out, minInstLen);
    appendU8(out, 1);                          // max_ops_per_instruction
    appendU8(out, 1);                          // default_is_stmt
    appendU8(out, static_cast<uint8_t>(-5));   // line_base = -5
    appendU8(out, 14);                         // line_range = 14
    appendU8(out, 13);                         // opcode_base = 13

    static constexpr uint8_t kStdOpcodeLengths[12] = { 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1 };
    out.insert(out.end(), std::begin(kStdOpcodeLengths), std::end(kStdOpcodeLengths));

    // directory_entry_format
    appendU8(out, 1);
    Dwarf5Emitter::appendUleb128(out, DW_LNCT_path);
    Dwarf5Emitter::appendUleb128(out, DW_FORM_line_strp);

    Dwarf5Emitter::appendUleb128(out, dirs.size());
    for (const auto &d : dirs)
    {
        emitLineStrp(out, sec.m_relocs, lineStrTab, d);
    }

    // file_name_entry_format
    appendU8(out, 2);
    Dwarf5Emitter::appendUleb128(out, DW_LNCT_path);
    Dwarf5Emitter::appendUleb128(out, DW_FORM_line_strp);
    Dwarf5Emitter::appendUleb128(out, DW_LNCT_directory_index);
    Dwarf5Emitter::appendUleb128(out, DW_FORM_udata);

    // Entry 0 = mainFile, Entries 1..N = remaining cu.files (avoiding duplicate mainFile)
    std::string mainDir;
    std::string mainFilename;
    EzCore::Debug::splitDirectoryAndFilename(cu.mainFile, mainDir, mainFilename);
    if (mainFilename.empty())
    {
        mainFilename = cu.mainFile;
    }
    const uint32_t mainDirIdx = addDir(mainDir);

    const size_t totalFiles = 1 + cu.files.size() - (fileMap.hasMainFileInFiles ? 1 : 0);
    Dwarf5Emitter::appendUleb128(out, totalFiles);
    emitLineStrp(out, sec.m_relocs, lineStrTab, mainFilename);
    Dwarf5Emitter::appendUleb128(out, mainDirIdx);

    for (const auto &f : cu.files)
    {
        if (fileMap.hasMainFileInFiles && f.fileId == fileMap.mainFileId)
        {
            continue;
        }
        emitLineStrp(out, sec.m_relocs, lineStrTab, f.filename);
        Dwarf5Emitter::appendUleb128(out, dirIndexMap[f.directory]);
    }

    patchU32(out, headerLenPos, static_cast<uint32_t>(out.size() - headerStart));

    // Line number program per function
    for (const auto &fn : cu.functions)
    {
        if (fn.lineEntries.empty() && fn.codeSize == 0)
        {
            continue;
        }

        // DW_LNE_set_address
        appendU8(out, 0x00);
        Dwarf5Emitter::appendUleb128(out, 1 + cu.pointerSize);
        appendU8(out, DW_LNE_set_address);
        emitTextAddr64(out, sec.m_relocs, fn.sectionOffset);

        uint64_t curAddr = fn.sectionOffset;
        uint32_t curFile = 1;
        uint32_t curLine = 1;
        uint32_t curCol = 0;
        bool curIsStmt = true;

        for (const auto &entry : fn.lineEntries)
        {
            const uint64_t targetAddr = fn.sectionOffset + entry.codeOffset;
            if (targetAddr > curAddr)
            {
                const uint64_t adv = (targetAddr - curAddr) / minInstLen;
                if (adv > 0)
                {
                    appendU8(out, DW_LNS_advance_pc);
                    Dwarf5Emitter::appendUleb128(out, adv);
                }
                curAddr = targetAddr;
            }

            const uint32_t mappedFile = fileMap.map(entry.fileId);
            if (mappedFile != curFile)
            {
                appendU8(out, DW_LNS_set_file);
                Dwarf5Emitter::appendUleb128(out, mappedFile);
                curFile = mappedFile;
            }

            if (entry.column != curCol)
            {
                appendU8(out, DW_LNS_set_column);
                Dwarf5Emitter::appendUleb128(out, entry.column);
                curCol = entry.column;
            }

            if (entry.isStmt != curIsStmt)
            {
                appendU8(out, DW_LNS_negate_stmt);
                curIsStmt = entry.isStmt;
            }

            if (entry.prologueEnd)
            {
                appendU8(out, DW_LNS_set_prologue_end);
            }

            if (entry.epilogueBegin)
            {
                appendU8(out, DW_LNS_set_epilogue_begin);
            }

            const int64_t lineDelta = static_cast<int64_t>(entry.line) - static_cast<int64_t>(curLine);
            if (lineDelta != 0)
            {
                appendU8(out, DW_LNS_advance_line);
                Dwarf5Emitter::appendSleb128(out, lineDelta);
                curLine = entry.line;
            }

            appendU8(out, DW_LNS_copy);
        }

        const uint64_t endAddr = fn.sectionOffset + fn.codeSize;
        if (endAddr > curAddr)
        {
            const uint64_t adv = (endAddr - curAddr) / minInstLen;
            if (adv > 0)
            {
                appendU8(out, DW_LNS_advance_pc);
                Dwarf5Emitter::appendUleb128(out, adv);
            }
        }

        // DW_LNE_end_sequence
        appendU8(out, 0x00);
        appendU8(out, 1);
        appendU8(out, DW_LNE_end_sequence);
    }

    patchU32(out, 0, static_cast<uint32_t>(out.size() - 4));
    return sec;
}

ObjectFormat::ExtraObjectSection buildDebugFrameSection(const EzCore::Debug::DebugCompileUnit &cu)
{
    ObjectFormat::ExtraObjectSection sec;
    sec.m_name = ".debug_frame";
    sec.m_alignment = 8;
    sec.m_isDebug = true;

    std::vector<uint8_t> &out = sec.m_data;
    const bool isRiscV = (cu.arch == EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64);

    // Emit CIE (DWARF v4 .debug_frame CIE)
    const size_t cieStart = out.size();
    appendU32(out, 0);          // length placeholder
    appendU32(out, 0xFFFFFFFF); // CIE_id
    appendU8(out, 4);           // version = 4
    appendU8(out, 0);           // augmentation = ""
    appendU8(out, cu.pointerSize);
    appendU8(out, 0);           // segment_selector_size

    if (!isRiscV)
    {
        // x86-64: code_align=1, data_align=-8, ret_addr_reg=16 (RIP)
        Dwarf5Emitter::appendUleb128(out, 1);
        Dwarf5Emitter::appendSleb128(out, -8);
        Dwarf5Emitter::appendUleb128(out, 16);

        // Initial instructions: DW_CFA_def_cfa rsp(7), 8; DW_CFA_offset rip(16), 1
        appendU8(out, DW_CFA_def_cfa);
        Dwarf5Emitter::appendUleb128(out, 7);
        Dwarf5Emitter::appendUleb128(out, 8);
        appendU8(out, static_cast<uint8_t>(DW_CFA_offset | 16));
        Dwarf5Emitter::appendUleb128(out, 1);
    }
    else
    {
        // RISC-V 64: code_align=2, data_align=-8, ret_addr_reg=1 (ra)
        Dwarf5Emitter::appendUleb128(out, 2);
        Dwarf5Emitter::appendSleb128(out, -8);
        Dwarf5Emitter::appendUleb128(out, 1);

        // Initial instructions: DW_CFA_def_cfa sp(2), 0
        appendU8(out, DW_CFA_def_cfa);
        Dwarf5Emitter::appendUleb128(out, 2);
        Dwarf5Emitter::appendUleb128(out, 0);
    }

    while ((out.size() - cieStart) % 8 != 0)
    {
        appendU8(out, DW_CFA_nop);
    }
    patchU32(out, cieStart, static_cast<uint32_t>(out.size() - cieStart - 4));

    // Emit one FDE per function
    for (const auto &fn : cu.functions)
    {
        const size_t fdeStart = out.size();
        appendU32(out, 0); // length placeholder

        // CIE_pointer with Absolute32 reloc to .debug_frame + 0
        ObjectFormat::ExtraSectionReloc cieRel{};
        cieRel.m_offset = out.size();
        cieRel.m_targetName = ".debug_frame";
        cieRel.m_targetIsSection = true;
        cieRel.m_type = TargetCodeRelocationType::Absolute32;
        cieRel.m_addend = 0;
        sec.m_relocs.push_back(std::move(cieRel));
        appendU32(out, 0);

        emitTextAddr64(out, sec.m_relocs, fn.sectionOffset);
        appendU64(out, fn.codeSize);

        auto emitSavedRegs = [&](bool skipRbp, bool skipRaAndS0)
        {
            for (const auto &saved : fn.savedRegs)
            {
                const uint16_t dreg =
                        isRiscV ? EzCore::Debug::mapRiscV64ToDwarfReg(saved.regId, saved.isFpr)
                                : EzCore::Debug::mapX86_64ToDwarfReg(saved.regId, saved.isFpr);
                if (dreg == 0xFFFF || saved.cfaOffset >= 0)
                {
                    continue;
                }
                if (skipRbp && dreg == 6)
                {
                    continue;
                }
                if (skipRaAndS0 && (dreg == 1 || dreg == 8))
                {
                    continue;
                }
                if (dreg < 64)
                {
                    appendU8(out, static_cast<uint8_t>(DW_CFA_offset | dreg));
                }
                else
                {
                    appendU8(out, DW_CFA_offset_extended);
                    Dwarf5Emitter::appendUleb128(out, dreg);
                }
                Dwarf5Emitter::appendUleb128(out, static_cast<uint64_t>((-saved.cfaOffset) / 8));
            }
        };

        if (!isRiscV)
        {
            if (fn.hasFramePointer)
            {
                if (fn.prologueSize > 0)
                {
                    appendU8(out, DW_CFA_advance_loc1);
                    appendU8(out, 1);
                    appendU8(out, DW_CFA_def_cfa_offset);
                    Dwarf5Emitter::appendUleb128(out, 16);
                    appendU8(out, static_cast<uint8_t>(DW_CFA_offset | 6)); // RBP
                    Dwarf5Emitter::appendUleb128(out, 2);
                    if (fn.prologueSize >= 4)
                    {
                        appendU8(out, DW_CFA_advance_loc1);
                        appendU8(out, 3);
                        appendU8(out, DW_CFA_def_cfa_register);
                        Dwarf5Emitter::appendUleb128(out, 6); // RBP
                    }
                }
                emitSavedRegs(true, false);
            }
            else if (fn.totalFrameSize > 0 || !fn.savedRegs.empty())
            {
                if (fn.prologueSize > 0)
                {
                    appendU8(out, DW_CFA_advance_loc1);
                    appendU8(out, static_cast<uint8_t>(std::min<uint32_t>(fn.prologueSize, 255)));
                }
                if (fn.totalFrameSize > 0)
                {
                    appendU8(out, DW_CFA_def_cfa_offset);
                    Dwarf5Emitter::appendUleb128(out, static_cast<uint64_t>(fn.totalFrameSize + 8));
                }
                emitSavedRegs(false, false);
            }
        }
        else
        {
            if (fn.hasFramePointer)
            {
                if (fn.prologueSize > 0)
                {
                    const uint8_t adv = static_cast<uint8_t>(std::min<uint32_t>(fn.prologueSize, 16) / 2);
                    if (adv > 0)
                    {
                        appendU8(out, DW_CFA_advance_loc1);
                        appendU8(out, adv);
                    }
                }
                // s0 = sp + 16 = CFA => CFA = s0 + 0; ra at CFA-8 (1), s0 at CFA-16 (2)
                appendU8(out, DW_CFA_def_cfa);
                Dwarf5Emitter::appendUleb128(out, 8);
                Dwarf5Emitter::appendUleb128(out, 0);
                appendU8(out, static_cast<uint8_t>(DW_CFA_offset | 1));
                Dwarf5Emitter::appendUleb128(out, 1);
                appendU8(out, static_cast<uint8_t>(DW_CFA_offset | 8));
                Dwarf5Emitter::appendUleb128(out, 2);
                emitSavedRegs(false, true);
            }
            else if (fn.totalFrameSize > 0 || !fn.savedRegs.empty())
            {
                if (fn.prologueSize > 0)
                {
                    const uint8_t adv = static_cast<uint8_t>(std::min<uint32_t>(fn.prologueSize, 255) / 2);
                    if (adv > 0)
                    {
                        appendU8(out, DW_CFA_advance_loc1);
                        appendU8(out, adv);
                    }
                }
                if (fn.totalFrameSize > 0)
                {
                    appendU8(out, DW_CFA_def_cfa_offset);
                    Dwarf5Emitter::appendUleb128(out, static_cast<uint64_t>(fn.totalFrameSize));
                }
                emitSavedRegs(false, false);
            }
        }

        while ((out.size() - fdeStart) % 8 != 0)
        {
            appendU8(out, DW_CFA_nop);
        }
        patchU32(out, fdeStart, static_cast<uint32_t>(out.size() - fdeStart - 4));
    }

    return sec;
}

} // namespace

void Dwarf5Emitter::appendUleb128(std::vector<uint8_t> &out, uint64_t value)
{
    do
    {
        uint8_t byte = static_cast<uint8_t>(value & 0x7FU);
        value >>= 7;
        if (value != 0)
        {
            byte |= 0x80U;
        }
        out.push_back(byte);
    } while (value != 0);
}

void Dwarf5Emitter::appendSleb128(std::vector<uint8_t> &out, int64_t value)
{
    bool more = true;
    while (more)
    {
        uint8_t byte = static_cast<uint8_t>(value & 0x7F);
        value >>= 7;
        const bool signBit = (byte & 0x40U) != 0;
        if ((value == 0 && !signBit) || (value == -1 && signBit))
        {
            more = false;
        }
        else
        {
            byte |= 0x80U;
        }
        out.push_back(byte);
    }
}

std::vector<ObjectFormat::ExtraObjectSection> Dwarf5Emitter::emit(const EzCore::Debug::DebugCompileUnit &cu)
{
    if (cu.level == EzCore::Debug::DebugLevel::None)
    {
        return {};
    }

    StringTableBuilder strTab;
    StringTableBuilder lineStrTab;
    AbbrevTableBuilder abbrevTab;
    const DwarfFileMap fileMap = buildDwarfFileMap(cu);

    ObjectFormat::ExtraObjectSection lineSec = buildDebugLineSection(cu, fileMap, lineStrTab);
    ObjectFormat::ExtraObjectSection frameSec = buildDebugFrameSection(cu);

    ObjectFormat::ExtraObjectSection infoSec;
    infoSec.m_name = ".debug_info";
    infoSec.m_alignment = 1;
    infoSec.m_isDebug = true;
    std::vector<uint8_t> &info = infoSec.m_data;

    // DWARF v5 Compile Unit Header (12 bytes)
    appendU32(info, 0);                    // unit_length placeholder
    appendU16(info, 5);                    // version = 5
    appendU8(info, DW_UT_compile);         // unit_type = DW_UT_compile
    appendU8(info, cu.pointerSize);        // address_size = 8

    ObjectFormat::ExtraSectionReloc abbrevRel{};
    abbrevRel.m_offset = info.size();
    abbrevRel.m_targetName = ".debug_abbrev";
    abbrevRel.m_targetIsSection = true;
    abbrevRel.m_type = TargetCodeRelocationType::Absolute32;
    abbrevRel.m_addend = 0;
    infoSec.m_relocs.push_back(std::move(abbrevRel));
    appendU32(info, 0); // debug_abbrev_offset

    const bool hasCuChildren = !cu.functions.empty() ||
                               (cu.level == EzCore::Debug::DebugLevel::Full && (!cu.types.empty() || !cu.globals.empty()));
    const bool hasFunctions = !cu.functions.empty();

    AbbrevSpec cuSpec;
    cuSpec.tag = DW_TAG_compile_unit;
    cuSpec.hasChildren = hasCuChildren;
    cuSpec.attrs = {
        { DW_AT_producer, DW_FORM_strp },
        { DW_AT_language, DW_FORM_data2 },
        { DW_AT_name, DW_FORM_strp },
        { DW_AT_comp_dir, DW_FORM_strp },
        { DW_AT_stmt_list, DW_FORM_sec_offset }
    };
    if (hasFunctions)
    {
        cuSpec.attrs.emplace_back(DW_AT_low_pc, DW_FORM_addr);
        cuSpec.attrs.emplace_back(DW_AT_high_pc, DW_FORM_data8);
    }

    appendUleb128(info, abbrevTab.getOrAdd(cuSpec));
    emitStrp(info, infoSec.m_relocs, strTab, cu.producer);
    appendU16(info, static_cast<uint16_t>(cu.language));
    emitStrp(info, infoSec.m_relocs, strTab, cu.mainFile);
    emitStrp(info, infoSec.m_relocs, strTab, cu.compDir);

    ObjectFormat::ExtraSectionReloc stmtRel{};
    stmtRel.m_offset = info.size();
    stmtRel.m_targetName = ".debug_line";
    stmtRel.m_targetIsSection = true;
    stmtRel.m_type = TargetCodeRelocationType::Absolute32;
    stmtRel.m_addend = 0;
    infoSec.m_relocs.push_back(std::move(stmtRel));
    appendU32(info, 0); // DW_AT_stmt_list

    if (hasFunctions)
    {
        uint64_t cuLow = cu.functions.front().sectionOffset;
        uint64_t cuHigh = cu.functions.front().sectionOffset + cu.functions.front().codeSize;
        for (const auto &fn : cu.functions)
        {
            cuLow = std::min(cuLow, fn.sectionOffset);
            cuHigh = std::max(cuHigh, fn.sectionOffset + fn.codeSize);
        }
        emitTextAddr64(info, infoSec.m_relocs, cuLow);
        appendU64(info, cuHigh - cuLow);
    }

    // Track CU-relative DIE offsets for DW_FORM_ref4 patching
    std::unordered_map<uint64_t, uint32_t> typeDieOffsets;
    struct Ref4Patch
    {
        size_t patchOffset;
        uint64_t targetTypeId;
    };
    std::vector<Ref4Patch> refPatches;

    uint32_t fallbackTypeDieOffset = 0;
    auto ensureFallbackType = [&]() -> uint32_t
    {
        if (fallbackTypeDieOffset != 0)
        {
            return fallbackTypeDieOffset;
        }
        AbbrevSpec baseSpec;
        baseSpec.tag = DW_TAG_base_type;
        baseSpec.hasChildren = false;
        baseSpec.attrs = {
            { DW_AT_name, DW_FORM_strp },
            { DW_AT_encoding, DW_FORM_data1 },
            { DW_AT_byte_size, DW_FORM_udata }
        };
        fallbackTypeDieOffset = static_cast<uint32_t>(info.size());
        appendUleb128(info, abbrevTab.getOrAdd(baseSpec));
        emitStrp(info, infoSec.m_relocs, strTab, "__unknown_type");
        appendU8(info, DW_ATE_unsigned);
        appendUleb128(info, 1);
        return fallbackTypeDieOffset;
    };

    auto isResolvableNonVoidType = [&](uint64_t typeId) -> bool
    {
        if (typeId == 0)
        {
            return false;
        }
        const auto *found = cu.findType(typeId);
        return found != nullptr && found->kind != EzCore::RttiTypeKind::Void;
    };

    auto emitTypeRef4 = [&](uint64_t targetTypeId)
    {
        if (!isResolvableNonVoidType(targetTypeId))
        {
            appendU32(info, ensureFallbackType());
            return;
        }
        refPatches.push_back({ info.size(), targetTypeId });
        appendU32(info, 0);
    };

    uint32_t arrayIndexTypeDieOffset = 0;
    auto ensureArrayIndexType = [&]() -> uint32_t
    {
        if (arrayIndexTypeDieOffset != 0)
        {
            return arrayIndexTypeDieOffset;
        }
        AbbrevSpec baseSpec;
        baseSpec.tag = DW_TAG_base_type;
        baseSpec.hasChildren = false;
        baseSpec.attrs = {
            { DW_AT_name, DW_FORM_strp },
            { DW_AT_encoding, DW_FORM_data1 },
            { DW_AT_byte_size, DW_FORM_udata }
        };
        arrayIndexTypeDieOffset = static_cast<uint32_t>(info.size());
        appendUleb128(info, abbrevTab.getOrAdd(baseSpec));
        emitStrp(info, infoSec.m_relocs, strTab, "__ARRAY_SIZE_TYPE__");
        appendU8(info, DW_ATE_unsigned);
        appendUleb128(info, cu.pointerSize);
        return arrayIndexTypeDieOffset;
    };

    if (cu.level == EzCore::Debug::DebugLevel::Full)
    {
        // Emit all types
        for (const auto &ty : cu.types)
        {
            if (ty.kind == EzCore::RttiTypeKind::Void)
            {
                continue;
            }

            typeDieOffsets[ty.typeId] = static_cast<uint32_t>(info.size());

            switch (ty.kind)
            {
                case EzCore::RttiTypeKind::Boolean:
                case EzCore::RttiTypeKind::SignedInt:
                case EzCore::RttiTypeKind::UnsignedInt:
                case EzCore::RttiTypeKind::Float:
                {
                    AbbrevSpec spec;
                    spec.tag = DW_TAG_base_type;
                    spec.hasChildren = false;
                    spec.attrs = {
                        { DW_AT_name, DW_FORM_strp },
                        { DW_AT_encoding, DW_FORM_data1 },
                        { DW_AT_byte_size, DW_FORM_udata }
                    };
                    appendUleb128(info, abbrevTab.getOrAdd(spec));
                    emitStrp(info, infoSec.m_relocs, strTab, ty.name.empty() ? "int" : ty.name);
                    appendU8(info, mapBaseEncoding(ty.encoding, ty.kind));
                    appendUleb128(info, ty.sizeInBytes);
                    break;
                }
                case EzCore::RttiTypeKind::Pointer:
                {
                    const bool hasPointee = (ty.referencedTypeId != 0 && cu.findType(ty.referencedTypeId) != nullptr &&
                                             cu.findType(ty.referencedTypeId)->kind != EzCore::RttiTypeKind::Void);
                    AbbrevSpec spec;
                    spec.tag = DW_TAG_pointer_type;
                    spec.hasChildren = false;
                    spec.attrs.emplace_back(DW_AT_byte_size, DW_FORM_udata);
                    if (hasPointee)
                    {
                        spec.attrs.emplace_back(DW_AT_type, DW_FORM_ref4);
                    }
                    appendUleb128(info, abbrevTab.getOrAdd(spec));
                    appendUleb128(info, ty.sizeInBytes > 0 ? ty.sizeInBytes : cu.pointerSize);
                    if (hasPointee)
                    {
                        emitTypeRef4(ty.referencedTypeId);
                    }
                    break;
                }
                case EzCore::RttiTypeKind::Array:
                case EzCore::RttiTypeKind::Vector:
                {
                    const uint32_t idxTypeOff = ensureArrayIndexType();
                    // Re-record this array DIE offset after potentially emitting index type
                    typeDieOffsets[ty.typeId] = static_cast<uint32_t>(info.size());

                    AbbrevSpec arrSpec;
                    arrSpec.tag = DW_TAG_array_type;
                    arrSpec.hasChildren = true;
                    arrSpec.attrs = { { DW_AT_type, DW_FORM_ref4 } };
                    appendUleb128(info, abbrevTab.getOrAdd(arrSpec));
                    emitTypeRef4(ty.referencedTypeId);

                    AbbrevSpec subSpec;
                    subSpec.tag = DW_TAG_subrange_type;
                    subSpec.hasChildren = false;
                    subSpec.attrs = { { DW_AT_type, DW_FORM_ref4 } };
                    if (ty.arrayLength > 0)
                    {
                        subSpec.attrs.emplace_back(DW_AT_upper_bound, DW_FORM_udata);
                    }
                    appendUleb128(info, abbrevTab.getOrAdd(subSpec));
                    appendU32(info, idxTypeOff);
                    if (ty.arrayLength > 0)
                    {
                        appendUleb128(info, ty.arrayLength - 1);
                    }

                    appendU8(info, 0); // End array children
                    break;
                }
                case EzCore::RttiTypeKind::Struct:
                case EzCore::RttiTypeKind::Union:
                {
                    const bool hasMembers = !ty.members.empty();
                    AbbrevSpec aggSpec;
                    aggSpec.tag = (ty.kind == EzCore::RttiTypeKind::Union) ? DW_TAG_union_type : DW_TAG_structure_type;
                    aggSpec.hasChildren = hasMembers;
                    aggSpec.attrs = {
                        { DW_AT_name, DW_FORM_strp },
                        { DW_AT_byte_size, DW_FORM_udata },
                        { DW_AT_decl_file, DW_FORM_udata },
                        { DW_AT_decl_line, DW_FORM_udata }
                    };
                    appendUleb128(info, abbrevTab.getOrAdd(aggSpec));
                    emitStrp(info, infoSec.m_relocs, strTab, ty.name);
                    appendUleb128(info, ty.sizeInBytes);
                    appendUleb128(info, fileMap.map(ty.declLoc.fileId));
                    appendUleb128(info, ty.declLoc.line);

                    for (const auto &member : ty.members)
                    {
                        AbbrevSpec memSpec;
                        memSpec.tag = DW_TAG_member;
                        memSpec.hasChildren = false;
                        memSpec.attrs = {
                            { DW_AT_name, DW_FORM_strp },
                            { DW_AT_type, DW_FORM_ref4 }
                        };
                        if (member.isBitfield)
                        {
                            memSpec.attrs.emplace_back(DW_AT_bit_size, DW_FORM_udata);
                            memSpec.attrs.emplace_back(DW_AT_data_bit_offset, DW_FORM_udata);
                        }
                        else
                        {
                            memSpec.attrs.emplace_back(DW_AT_data_member_location, DW_FORM_udata);
                        }
                        appendUleb128(info, abbrevTab.getOrAdd(memSpec));
                        emitStrp(info, infoSec.m_relocs, strTab, member.name);
                        emitTypeRef4(member.typeId);
                        if (member.isBitfield)
                        {
                            appendUleb128(info, member.bitWidth);
                            appendUleb128(info, static_cast<uint64_t>(member.byteOffset) * 8ULL + member.bitOffset);
                        }
                        else
                        {
                            appendUleb128(info, member.byteOffset);
                        }
                    }

                    if (hasMembers)
                    {
                        appendU8(info, 0);
                    }
                    break;
                }
                case EzCore::RttiTypeKind::Enum:
                {
                    const bool hasEnums = !ty.enumerators.empty();
                    AbbrevSpec enumSpec;
                    enumSpec.tag = DW_TAG_enumeration_type;
                    enumSpec.hasChildren = hasEnums;
                    enumSpec.attrs = {
                        { DW_AT_name, DW_FORM_strp },
                        { DW_AT_encoding, DW_FORM_data1 },
                        { DW_AT_byte_size, DW_FORM_udata },
                        { DW_AT_decl_file, DW_FORM_udata },
                        { DW_AT_decl_line, DW_FORM_udata }
                    };
                    appendUleb128(info, abbrevTab.getOrAdd(enumSpec));
                    emitStrp(info, infoSec.m_relocs, strTab, ty.name);
                    appendU8(info, DW_ATE_signed);
                    appendUleb128(info, ty.sizeInBytes > 0 ? ty.sizeInBytes : 4);
                    appendUleb128(info, fileMap.map(ty.declLoc.fileId));
                    appendUleb128(info, ty.declLoc.line);

                    AbbrevSpec etorSpec;
                    etorSpec.tag = DW_TAG_enumerator;
                    etorSpec.hasChildren = false;
                    etorSpec.attrs = {
                        { DW_AT_name, DW_FORM_strp },
                        { DW_AT_const_value, DW_FORM_sdata }
                    };
                    const uint32_t etorCode = abbrevTab.getOrAdd(etorSpec);
                    for (const auto &etor : ty.enumerators)
                    {
                        appendUleb128(info, etorCode);
                        emitStrp(info, infoSec.m_relocs, strTab, etor.name);
                        appendSleb128(info, etor.value);
                    }

                    if (hasEnums)
                    {
                        appendU8(info, 0);
                    }
                    break;
                }
                case EzCore::RttiTypeKind::Typedef:
                {
                    const bool hasAliased = (ty.referencedTypeId != 0 && cu.findType(ty.referencedTypeId) != nullptr);
                    AbbrevSpec tdSpec;
                    tdSpec.tag = DW_TAG_typedef;
                    tdSpec.hasChildren = false;
                    tdSpec.attrs.emplace_back(DW_AT_name, DW_FORM_strp);
                    if (hasAliased)
                    {
                        tdSpec.attrs.emplace_back(DW_AT_type, DW_FORM_ref4);
                    }
                    appendUleb128(info, abbrevTab.getOrAdd(tdSpec));
                    emitStrp(info, infoSec.m_relocs, strTab, ty.name);
                    if (hasAliased)
                    {
                        emitTypeRef4(ty.referencedTypeId);
                    }
                    break;
                }
                case EzCore::RttiTypeKind::Function:
                {
                    const bool hasRet = isResolvableNonVoidType(ty.returnTypeId);
                    const bool hasChildren = (!ty.paramTypeIds.empty() || ty.isVariadic);
                    AbbrevSpec subSpec;
                    subSpec.tag = DW_TAG_subroutine_type;
                    subSpec.hasChildren = hasChildren;
                    if (hasRet)
                    {
                        subSpec.attrs.emplace_back(DW_AT_type, DW_FORM_ref4);
                    }
                    appendUleb128(info, abbrevTab.getOrAdd(subSpec));
                    if (hasRet)
                    {
                        emitTypeRef4(ty.returnTypeId);
                    }
                    if (!ty.paramTypeIds.empty())
                    {
                        AbbrevSpec paramSpec;
                        paramSpec.tag = DW_TAG_formal_parameter;
                        paramSpec.hasChildren = false;
                        paramSpec.attrs = { { DW_AT_type, DW_FORM_ref4 } };
                        const uint32_t paramCode = abbrevTab.getOrAdd(paramSpec);
                        for (uint64_t pTypeId : ty.paramTypeIds)
                        {
                            appendUleb128(info, paramCode);
                            emitTypeRef4(pTypeId);
                        }
                    }
                    if (ty.isVariadic)
                    {
                        AbbrevSpec varSpec;
                        varSpec.tag = DW_TAG_unspecified_parameters;
                        varSpec.hasChildren = false;
                        appendUleb128(info, abbrevTab.getOrAdd(varSpec));
                    }
                    if (hasChildren)
                    {
                        appendU8(info, 0);
                    }
                    break;
                }
                default:
                    // Fallback base_type for Unknown if referenced
                    AbbrevSpec spec;
                    spec.tag = DW_TAG_base_type;
                    spec.hasChildren = false;
                    spec.attrs = {
                        { DW_AT_name, DW_FORM_strp },
                        { DW_AT_encoding, DW_FORM_data1 },
                        { DW_AT_byte_size, DW_FORM_udata }
                    };
                    appendUleb128(info, abbrevTab.getOrAdd(spec));
                    emitStrp(info, infoSec.m_relocs, strTab, ty.name.empty() ? "unknown" : ty.name);
                    appendU8(info, DW_ATE_unsigned);
                    appendUleb128(info, ty.sizeInBytes);
                    break;
            }
        }

        // Emit global variables
        for (const auto &global : cu.globals)
        {
            const bool hasType = (global.typeId != 0 && typeDieOffsets.contains(global.typeId));
            AbbrevSpec gSpec;
            gSpec.tag = DW_TAG_variable;
            gSpec.hasChildren = false;
            gSpec.attrs = { { DW_AT_name, DW_FORM_strp } };
            if (hasType)
            {
                gSpec.attrs.emplace_back(DW_AT_type, DW_FORM_ref4);
            }
            if (global.isExternal)
            {
                gSpec.attrs.emplace_back(DW_AT_external, DW_FORM_flag_present);
            }
            gSpec.attrs.emplace_back(DW_AT_decl_file, DW_FORM_udata);
            gSpec.attrs.emplace_back(DW_AT_decl_line, DW_FORM_udata);
            gSpec.attrs.emplace_back(DW_AT_location, DW_FORM_exprloc);

            appendUleb128(info, abbrevTab.getOrAdd(gSpec));
            emitStrp(info, infoSec.m_relocs, strTab, global.name);
            if (hasType)
            {
                emitTypeRef4(global.typeId);
            }
            appendUleb128(info, fileMap.map(global.declLoc.fileId));
            appendUleb128(info, global.declLoc.line);

            // DW_OP_addr + 8-byte reloc to section+offset
            appendUleb128(info, 1 + cu.pointerSize);
            appendU8(info, DW_OP_addr);
            ObjectFormat::ExtraSectionReloc gRel{};
            gRel.m_offset = info.size();
            gRel.m_targetName = global.sectionName.empty() ? ".data" : global.sectionName;
            gRel.m_targetIsSection = true;
            gRel.m_type = TargetCodeRelocationType::Absolute64;
            gRel.m_addend = static_cast<int64_t>(global.sectionOffset);
            infoSec.m_relocs.push_back(std::move(gRel));
            appendU64(info, 0);
        }
    }

    // Emit subprograms
    for (const auto &fn : cu.functions)
    {
        const bool hasRetType = (cu.level == EzCore::Debug::DebugLevel::Full &&
                                 fn.returnTypeId != 0 && typeDieOffsets.contains(fn.returnTypeId));
        const bool hasLinkage = (!fn.linkageName.empty() && fn.linkageName != fn.name);
        const bool hasChildren = (cu.level == EzCore::Debug::DebugLevel::Full &&
                                  (!fn.parameters.empty() || !fn.locals.empty()));

        AbbrevSpec fnSpec;
        fnSpec.tag = DW_TAG_subprogram;
        fnSpec.hasChildren = hasChildren;
        fnSpec.attrs = { { DW_AT_low_pc, DW_FORM_addr }, { DW_AT_high_pc, DW_FORM_data8 } };
        if (cu.level == EzCore::Debug::DebugLevel::Full)
        {
            fnSpec.attrs.emplace_back(DW_AT_frame_base, DW_FORM_exprloc);
        }
        fnSpec.attrs.emplace_back(DW_AT_name, DW_FORM_strp);
        if (hasLinkage)
        {
            fnSpec.attrs.emplace_back(DW_AT_linkage_name, DW_FORM_strp);
        }
        fnSpec.attrs.emplace_back(DW_AT_decl_file, DW_FORM_udata);
        fnSpec.attrs.emplace_back(DW_AT_decl_line, DW_FORM_udata);
        if (hasRetType)
        {
            fnSpec.attrs.emplace_back(DW_AT_type, DW_FORM_ref4);
        }
        if (fn.isExternal)
        {
            fnSpec.attrs.emplace_back(DW_AT_external, DW_FORM_flag_present);
        }

        appendUleb128(info, abbrevTab.getOrAdd(fnSpec));
        emitTextAddr64(info, infoSec.m_relocs, fn.sectionOffset);
        appendU64(info, fn.codeSize);

        if (cu.level == EzCore::Debug::DebugLevel::Full)
        {
            uint8_t fbReg = 6; // x86-64 RBP
            if (cu.arch == EzCore::Debug::DebugCompileUnit::TargetArch::RiscV64)
            {
                fbReg = fn.hasFramePointer ? 8 : 2; // s0 or sp
            }
            else
            {
                fbReg = fn.hasFramePointer ? 6 : 7; // rbp or rsp
            }
            appendUleb128(info, 1);
            appendU8(info, static_cast<uint8_t>(DW_OP_reg0 + fbReg));
        }

        emitStrp(info, infoSec.m_relocs, strTab, fn.name);
        if (hasLinkage)
        {
            emitStrp(info, infoSec.m_relocs, strTab, fn.linkageName);
        }
        appendUleb128(info, fileMap.map(fn.declLoc.fileId));
        appendUleb128(info, fn.declLoc.line);
        if (hasRetType)
        {
            emitTypeRef4(fn.returnTypeId);
        }

        if (hasChildren)
        {
            auto emitVar = [&](const EzCore::Debug::DebugVariable &var, uint64_t tag)
            {
                std::string globalSym;
                std::vector<uint8_t> expr = buildLocationExpr(var.location, cu.arch, globalSym);
                const bool hasLoc = !expr.empty();
                const bool hasVarType = (var.typeId != 0 && typeDieOffsets.contains(var.typeId));

                AbbrevSpec vSpec;
                vSpec.tag = tag;
                vSpec.hasChildren = false;
                if (hasLoc)
                {
                    vSpec.attrs.emplace_back(DW_AT_location, DW_FORM_exprloc);
                }
                vSpec.attrs.emplace_back(DW_AT_name, DW_FORM_strp);
                vSpec.attrs.emplace_back(DW_AT_decl_file, DW_FORM_udata);
                vSpec.attrs.emplace_back(DW_AT_decl_line, DW_FORM_udata);
                if (hasVarType)
                {
                    vSpec.attrs.emplace_back(DW_AT_type, DW_FORM_ref4);
                }

                appendUleb128(info, abbrevTab.getOrAdd(vSpec));
                if (hasLoc)
                {
                    if (!globalSym.empty())
                    {
                        appendUleb128(info, 1 + cu.pointerSize);
                        appendU8(info, DW_OP_addr);
                        ObjectFormat::ExtraSectionReloc vRel{};
                        vRel.m_offset = info.size();
                        vRel.m_targetName = globalSym;
                        vRel.m_targetIsSection = false;
                        vRel.m_type = TargetCodeRelocationType::Absolute64;
                        vRel.m_addend = 0;
                        infoSec.m_relocs.push_back(std::move(vRel));
                        appendU64(info, 0);
                    }
                    else
                    {
                        appendUleb128(info, expr.size());
                        info.insert(info.end(), expr.begin(), expr.end());
                    }
                }
                emitStrp(info, infoSec.m_relocs, strTab, var.name);
                appendUleb128(info, fileMap.map(var.declLoc.fileId));
                appendUleb128(info, var.declLoc.line);
                if (hasVarType)
                {
                    emitTypeRef4(var.typeId);
                }
            };

            for (const auto &param : fn.parameters)
            {
                emitVar(param, DW_TAG_formal_parameter);
            }
            for (const auto &local : fn.locals)
            {
                emitVar(local, DW_TAG_variable);
            }

            appendU8(info, 0); // End subprogram children
        }
    }

    if (hasCuChildren)
    {
        appendU8(info, 0); // End CU children
    }

    // Patch all DW_FORM_ref4 offsets (never leaving 0 pointing at CU header)
    for (const auto &patch : refPatches)
    {
        auto it = typeDieOffsets.find(patch.targetTypeId);
        const uint32_t targetOff = (it != typeDieOffsets.end()) ? it->second : ensureFallbackType();
        patchU32(info, patch.patchOffset, targetOff);
    }

    // Patch CU unit_length
    patchU32(info, 0, static_cast<uint32_t>(info.size() - 4));

    // Finalize .debug_str, .debug_line_str, .debug_abbrev
    ObjectFormat::ExtraObjectSection strSec;
    strSec.m_name = ".debug_str";
    strSec.m_data = strTab.takeData();
    strSec.m_alignment = 1;
    strSec.m_isDebug = true;
    strSec.m_isMergeStrings = true;

    ObjectFormat::ExtraObjectSection lineStrSec;
    lineStrSec.m_name = ".debug_line_str";
    lineStrSec.m_data = lineStrTab.takeData();
    lineStrSec.m_alignment = 1;
    lineStrSec.m_isDebug = true;
    lineStrSec.m_isMergeStrings = true;

    ObjectFormat::ExtraObjectSection abbrevSec;
    abbrevSec.m_name = ".debug_abbrev";
    abbrevSec.m_data = abbrevTab.serialize();
    abbrevSec.m_alignment = 1;
    abbrevSec.m_isDebug = true;

    std::vector<ObjectFormat::ExtraObjectSection> result;
    result.reserve(6);
    result.push_back(std::move(strSec));
    result.push_back(std::move(lineStrSec));
    result.push_back(std::move(abbrevSec));
    result.push_back(std::move(infoSec));
    result.push_back(std::move(lineSec));
    result.push_back(std::move(frameSec));
    return result;
}

} // namespace EzCodeEmitter::Debug
