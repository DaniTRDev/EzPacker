#include "EzCodeEmitterTestSuite.h"
#include "Debug/CodeViewEmitter.h"
#include "Debug/Dwarf5Emitter.h"
#include "Debug/IDebugInfoEmitter.h"
#include "Helpers.h"
#include "ObjectFormat/CoffWriter.h"
#include "ObjectFormat/Elf64Writer.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace EzCodeEmitter;
using namespace EzCodeEmitter::Debug;
using namespace EzCodeEmitter::ObjectFormat;
using namespace EzCore::Debug;

namespace
{

uint16_t readU16(const std::vector<uint8_t> &buf, size_t off)
{
    uint16_t v = 0;
    std::memcpy(&v, &buf[off], 2);
    return v;
}

uint32_t readU32(const std::vector<uint8_t> &buf, size_t off)
{
    uint32_t v = 0;
    std::memcpy(&v, &buf[off], 4);
    return v;
}

std::string runCommandAndCapture(const std::string &cmd, int &exitCode)
{
    std::string output;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        exitCode = -1;
        return output;
    }
    std::array<char, 512> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr)
    {
        output += buffer.data();
    }
    const int status = pclose(pipe);
    exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return output;
}

DebugCompileUnit buildRichCompileUnit(DebugCompileUnit::TargetArch arch)
{
    DebugCompileUnit cu;
    cu.producer = "EzPacker ezc 1.0.0";
    cu.language = DebugLanguage::C11;
    cu.level = DebugLevel::Full;
    cu.compDir = "/workspace/src";
    cu.mainFile = "/workspace/src/main.c";
    cu.pointerSize = 8;
    cu.arch = arch;

    const uint32_t mainFileId = cu.addOrGetFile("/workspace/src/main.c");

    // Base types
    DebugType intTy;
    intTy.typeId = 100;
    intTy.name = "int";
    intTy.kind = EzCore::RttiTypeKind::SignedInt;
    intTy.encoding = DebugBaseEncoding::Signed;
    intTy.sizeInBytes = 4;
    cu.addType(intTy);

    DebugType voidPtrTy;
    voidPtrTy.typeId = 101;
    voidPtrTy.kind = EzCore::RttiTypeKind::Pointer;
    voidPtrTy.sizeInBytes = 8;
    voidPtrTy.referencedTypeId = 0;
    cu.addType(voidPtrTy);

    // Register Node* BEFORE Node to exercise forward-ref / two-pass ref4 patching
    DebugType nodePtrTy;
    nodePtrTy.typeId = 102;
    nodePtrTy.kind = EzCore::RttiTypeKind::Pointer;
    nodePtrTy.sizeInBytes = 8;
    nodePtrTy.referencedTypeId = 103;
    cu.addType(nodePtrTy);

    DebugType nodeStructTy;
    nodeStructTy.typeId = 103;
    nodeStructTy.name = "Node";
    nodeStructTy.kind = EzCore::RttiTypeKind::Struct;
    nodeStructTy.sizeInBytes = 16;
    nodeStructTy.declLoc = { mainFileId, 5, 1 };
    nodeStructTy.members = {
        { "value", 100, 0, 0, 0, false },
        { "next", 102, 8, 0, 0, false }
    };
    cu.addType(nodeStructTy);

    DebugType colorEnumTy;
    colorEnumTy.typeId = 104;
    colorEnumTy.name = "Color";
    colorEnumTy.kind = EzCore::RttiTypeKind::Enum;
    colorEnumTy.sizeInBytes = 4;
    colorEnumTy.declLoc = { mainFileId, 10, 1 };
    colorEnumTy.enumerators = { { "Red", 0 }, { "Green", 1 }, { "Blue", -1 } };
    cu.addType(colorEnumTy);

    DebugType valUnionTy;
    valUnionTy.typeId = 105;
    valUnionTy.name = "ValueUnion";
    valUnionTy.kind = EzCore::RttiTypeKind::Union;
    valUnionTy.sizeInBytes = 8;
    valUnionTy.declLoc = { mainFileId, 15, 1 };
    valUnionTy.members = {
        { "as_int", 100, 0, 0, 0, false },
        { "as_ptr", 101, 0, 0, 0, false }
    };
    cu.addType(valUnionTy);

    DebugType intArrTy;
    intArrTy.typeId = 106;
    intArrTy.kind = EzCore::RttiTypeKind::Array;
    intArrTy.sizeInBytes = 16;
    intArrTy.referencedTypeId = 100;
    intArrTy.arrayLength = 4;
    cu.addType(intArrTy);

    DebugType myIntTypedef;
    myIntTypedef.typeId = 107;
    myIntTypedef.name = "MyInt";
    myIntTypedef.kind = EzCore::RttiTypeKind::Typedef;
    myIntTypedef.referencedTypeId = 100;
    cu.addType(myIntTypedef);

    // Global variable
    DebugGlobalVar gVar;
    gVar.name = "g_counter";
    gVar.typeId = 100;
    gVar.declLoc = { mainFileId, 20, 1 };
    gVar.isExternal = true;
    gVar.sizeInBytes = 4;
    gVar.sectionName = ".data";
    gVar.sectionOffset = 0;
    cu.globals.push_back(gVar);

    // Function
    DebugFunction fn;
    fn.name = "compute";
    fn.returnTypeId = 100;
    fn.declLoc = { mainFileId, 25, 1 };
    fn.sectionOffset = 0;
    fn.codeSize = 16;
    fn.prologueSize = 4;
    fn.isExternal = true;
    fn.hasFramePointer = true;
    fn.totalFrameSize = 32;

    DebugVariable param;
    param.name = "head";
    param.typeId = 102;
    param.isParameter = true;
    param.paramIndex = 0;
    param.declLoc = { mainFileId, 25, 13 };
    param.location = { DebugLocationKind::Register, 7, 7, false, {} }; // RDI on x86-64 / x7 on riscv
    fn.parameters.push_back(param);

    DebugVariable local;
    local.name = "acc";
    local.typeId = 107;
    local.isParameter = false;
    local.declLoc = { mainFileId, 27, 9 };
    local.location = { DebugLocationKind::FrameBaseOffset, -8, 0, false, {} };
    fn.locals.push_back(local);

    fn.lineEntries = {
        { 0, mainFileId, 25, 1, true, false, false },
        { 4, mainFileId, 27, 5, true, true, false },
        { 12, mainFileId, 28, 5, true, false, true }
    };
    cu.functions.push_back(fn);

    return cu;
}

} // namespace

TEST_F(EzCodeEmitterTestSuite, TestDebugInfoEmitterFactory)
{
    auto autoElf = createDebugInfoEmitter(DebugFormat::Auto, false);
    EXPECT_NE(dynamic_cast<Dwarf5Emitter *>(autoElf.get()), nullptr);

    auto autoCoff = createDebugInfoEmitter(DebugFormat::Auto, true);
    EXPECT_NE(dynamic_cast<CodeViewEmitter *>(autoCoff.get()), nullptr);

    auto forceDwarf = createDebugInfoEmitter(DebugFormat::Dwarf5, true);
    EXPECT_NE(dynamic_cast<Dwarf5Emitter *>(forceDwarf.get()), nullptr);

    auto forceCv = createDebugInfoEmitter(DebugFormat::CodeView, false);
    EXPECT_NE(dynamic_cast<CodeViewEmitter *>(forceCv.get()), nullptr);

    DebugCompileUnit cu;
    cu.level = DebugLevel::None;
    EXPECT_TRUE(autoElf->emit(cu).empty());
    EXPECT_TRUE(autoCoff->emit(cu).empty());
}

TEST_F(EzCodeEmitterTestSuite, TestLeb128Encoding)
{
    auto encodeU = [](uint64_t v)
    {
        std::vector<uint8_t> out;
        Dwarf5Emitter::appendUleb128(out, v);
        return out;
    };
    auto encodeS = [](int64_t v)
    {
        std::vector<uint8_t> out;
        Dwarf5Emitter::appendSleb128(out, v);
        return out;
    };

    EXPECT_EQ(encodeU(0), (std::vector<uint8_t>{ 0x00 }));
    EXPECT_EQ(encodeU(63), (std::vector<uint8_t>{ 0x3F }));
    EXPECT_EQ(encodeU(64), (std::vector<uint8_t>{ 0x40 }));
    EXPECT_EQ(encodeU(127), (std::vector<uint8_t>{ 0x7F }));
    EXPECT_EQ(encodeU(128), (std::vector<uint8_t>{ 0x80, 0x01 }));
    EXPECT_EQ(encodeU(624485), (std::vector<uint8_t>{ 0xE5, 0x8E, 0x26 }));

    EXPECT_EQ(encodeS(0), (std::vector<uint8_t>{ 0x00 }));
    EXPECT_EQ(encodeS(63), (std::vector<uint8_t>{ 0x3F }));
    EXPECT_EQ(encodeS(64), (std::vector<uint8_t>{ 0xC0, 0x00 }));
    EXPECT_EQ(encodeS(-64), (std::vector<uint8_t>{ 0x40 }));
    EXPECT_EQ(encodeS(-65), (std::vector<uint8_t>{ 0xBF, 0x7F }));
    EXPECT_EQ(encodeS(std::numeric_limits<int64_t>::max()).size(), 10u);
    EXPECT_EQ(encodeS(std::numeric_limits<int64_t>::min()).size(), 10u);
    EXPECT_EQ(encodeS(std::numeric_limits<int64_t>::min()).back(), 0x7F);
}

TEST_F(EzCodeEmitterTestSuite, TestDwarf5X86_64WithReadelf)
{
    DebugCompileUnit cu = buildRichCompileUnit(DebugCompileUnit::TargetArch::X86_64);
    std::string err;
    ASSERT_TRUE(cu.validate(err)) << err;

    Dwarf5Emitter emitter;
    auto debugSections = emitter.emit(cu);
    ASSERT_EQ(debugSections.size(), 6u);

    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());
    for (uint32_t i = 0; i < 16; ++i)
    {
        sections[SectionType::Text]->emit8(0x90);
    }
    sections[SectionType::Data]->emit32(0);
    sections[SectionType::Text]->finalize();
    sections[SectionType::Data]->finalize();

    Elf64Writer writer;
    writer.setMachine(62); // EM_X86_64
    writer.addSymbol({ .m_name = "compute",
                       .m_section = SectionType::Text,
                       .m_offset = 0,
                       .m_size = 16,
                       .m_isGlobal = true,
                       .m_isFunction = true });
    writer.addSymbol({ .m_name = "g_counter",
                       .m_section = SectionType::Data,
                       .m_offset = 0,
                       .m_size = 4,
                       .m_isGlobal = true,
                       .m_isFunction = false });
    for (const auto &sec : debugSections)
    {
        writer.addExtraSection(sec);
    }

    const std::vector<uint8_t> elfBytes = writer.write(sections);
    const std::filesystem::path tmpObj =
            std::filesystem::temp_directory_path() / "ezpacker_test_dwarf5_x64.o";
    {
        std::ofstream ofs(tmpObj, std::ios::binary);
        ofs.write(reinterpret_cast<const char *>(elfBytes.data()), static_cast<std::streamsize>(elfBytes.size()));
    }

    int exitCode = -1;
    const std::string out = runCommandAndCapture(
            "LC_ALL=C readelf --debug-dump=info,abbrev,line,str,frames " + tmpObj.string() + " 2>&1", exitCode);
    std::filesystem::remove(tmpObj);

    EXPECT_EQ(exitCode, 0) << out;
    EXPECT_NE(out.find("Version:       5"), std::string::npos) << out;
    EXPECT_NE(out.find("DW_TAG_compile_unit"), std::string::npos) << out;
    EXPECT_NE(out.find("DW_TAG_structure_type"), std::string::npos) << out;
    EXPECT_NE(out.find("Node"), std::string::npos) << out;
    EXPECT_NE(out.find("Color"), std::string::npos) << out;
    EXPECT_NE(out.find("ValueUnion"), std::string::npos) << out;
    EXPECT_NE(out.find("DW_TAG_subprogram"), std::string::npos) << out;
    EXPECT_NE(out.find("compute"), std::string::npos) << out;
    EXPECT_NE(out.find("g_counter"), std::string::npos) << out;
    EXPECT_NE(out.find("head"), std::string::npos) << out;
    EXPECT_NE(out.find("acc"), std::string::npos) << out;
    EXPECT_NE(out.find("25"), std::string::npos) << out;
    EXPECT_NE(out.find("27"), std::string::npos) << out;
    EXPECT_NE(out.find("28"), std::string::npos) << out;
}

TEST_F(EzCodeEmitterTestSuite, TestDwarf5RiscV64WithReadelf)
{
    DebugCompileUnit cu = buildRichCompileUnit(DebugCompileUnit::TargetArch::RiscV64);
    cu.level = DebugLevel::LineTablesOnly;

    Dwarf5Emitter emitter;
    auto debugSections = emitter.emit(cu);
    ASSERT_EQ(debugSections.size(), 6u);

    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());
    for (uint32_t i = 0; i < 16; ++i)
    {
        sections[SectionType::Text]->emit8(0x13);
    }
    sections[SectionType::Text]->finalize();

    Elf64Writer writer;
    writer.setMachine(243); // EM_RISCV
    writer.addSymbol({ .m_name = "compute",
                       .m_section = SectionType::Text,
                       .m_offset = 0,
                       .m_size = 16,
                       .m_isGlobal = true,
                       .m_isFunction = true });
    for (const auto &sec : debugSections)
    {
        writer.addExtraSection(sec);
    }

    const std::vector<uint8_t> elfBytes = writer.write(sections);
    const std::filesystem::path tmpObj =
            std::filesystem::temp_directory_path() / "ezpacker_test_dwarf5_rv64.o";
    {
        std::ofstream ofs(tmpObj, std::ios::binary);
        ofs.write(reinterpret_cast<const char *>(elfBytes.data()), static_cast<std::streamsize>(elfBytes.size()));
    }

    int exitCode = -1;
    const std::string out = runCommandAndCapture(
            "LC_ALL=C readelf --debug-dump=info,abbrev,line,str,frames " + tmpObj.string() + " 2>&1", exitCode);
    std::filesystem::remove(tmpObj);

    EXPECT_EQ(exitCode, 0) << out;
    EXPECT_NE(out.find("DW_TAG_subprogram"), std::string::npos) << out;
    EXPECT_NE(out.find("compute"), std::string::npos) << out;
    // LineTablesOnly must omit type/variable DIEs
    EXPECT_EQ(out.find("DW_TAG_structure_type"), std::string::npos) << out;
    EXPECT_EQ(out.find("DW_TAG_formal_parameter"), std::string::npos) << out;
}

TEST_F(EzCodeEmitterTestSuite, TestCodeViewX86_64C13AndCoffWriter)
{
    DebugCompileUnit cu = buildRichCompileUnit(DebugCompileUnit::TargetArch::X86_64);
    CodeViewEmitter emitter;
    auto debugSections = emitter.emit(cu);
    ASSERT_EQ(debugSections.size(), 2u);

    const auto &debugT = debugSections[0];
    const auto &debugS = debugSections[1];
    EXPECT_EQ(debugT.m_name, ".debug$T");
    EXPECT_EQ(debugS.m_name, ".debug$S");
    ASSERT_GE(debugT.m_data.size(), 4u);
    ASSERT_GE(debugS.m_data.size(), 4u);
    EXPECT_EQ(readU32(debugT.m_data, 0), 0x00000004u);
    EXPECT_EQ(readU32(debugS.m_data, 0), 0x00000004u);

    // Parse .debug$T records: verify 4-byte alignment and forward-ref LF_STRUCTURE
    size_t posT = 4;
    bool sawFwdStruct = false;
    bool sawCompleteStruct = false;
    bool sawFieldList = false;
    bool sawFuncId = false;
    while (posT < debugT.m_data.size())
    {
        ASSERT_EQ(posT % 4, 0u);
        const uint16_t recLen = readU16(debugT.m_data, posT);
        const uint16_t leaf = readU16(debugT.m_data, posT + 2);
        ASSERT_GE(recLen, 2u);
        if (leaf == 0x1505) // LF_STRUCTURE
        {
            const uint16_t prop = readU16(debugT.m_data, posT + 6);
            if ((prop & 0x0080U) != 0)
            {
                sawFwdStruct = true;
            }
            else
            {
                sawCompleteStruct = true;
            }
        }
        else if (leaf == 0x1203)
        {
            sawFieldList = true;
        }
        else if (leaf == 0x1601)
        {
            sawFuncId = true;
        }
        posT += 2 + recLen;
    }
    EXPECT_EQ(posT, debugT.m_data.size());
    EXPECT_TRUE(sawFwdStruct);
    EXPECT_TRUE(sawCompleteStruct);
    EXPECT_TRUE(sawFieldList);
    EXPECT_TRUE(sawFuncId);

    // Parse .debug$S C13 subsections
    size_t posS = 4;
    bool sawSymbols = false;
    bool sawLines = false;
    bool sawFileChksms = false;
    bool sawStrTab = false;
    while (posS < debugS.m_data.size())
    {
        ASSERT_EQ(posS % 4, 0u);
        const uint32_t subType = readU32(debugS.m_data, posS);
        const uint32_t subLen = readU32(debugS.m_data, posS + 4);
        if (subType == 0xF1) sawSymbols = true;
        if (subType == 0xF2) sawLines = true;
        if (subType == 0xF3) sawStrTab = true;
        if (subType == 0xF4) sawFileChksms = true;
        posS += 8 + ((subLen + 3) & ~3u);
    }
    EXPECT_TRUE(sawSymbols);
    EXPECT_TRUE(sawLines);
    EXPECT_TRUE(sawFileChksms);
    EXPECT_TRUE(sawStrTab);

    // Verify SecRel32 and SectionIdx16 relocations exist
    bool hasSecRel = false;
    bool hasSecIdx = false;
    for (const auto &rel : debugS.m_relocs)
    {
        if (rel.m_type == TargetCodeRelocationType::SecRel32) hasSecRel = true;
        if (rel.m_type == TargetCodeRelocationType::SectionIdx16) hasSecIdx = true;
    }
    EXPECT_TRUE(hasSecRel);
    EXPECT_TRUE(hasSecIdx);

    // Serialize through CoffWriter to verify all relocations resolve cleanly
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateCoffSections(sections, getAllocator());
    for (uint32_t i = 0; i < 16; ++i)
    {
        sections[SectionType::Text]->emit8(0x90);
    }
    sections[SectionType::Data]->emit32(0);
    sections[SectionType::Text]->finalize();
    sections[SectionType::Data]->finalize();

    CoffWriter coffWriter;
    coffWriter.addSymbol({ .m_name = "compute",
                           .m_section = SectionType::Text,
                           .m_offset = 0,
                           .m_size = 16,
                           .m_isGlobal = true,
                           .m_isFunction = true });
    coffWriter.addSymbol({ .m_name = "g_counter",
                           .m_section = SectionType::Data,
                           .m_offset = 0,
                           .m_size = 4,
                           .m_isGlobal = true,
                           .m_isFunction = false });
    for (const auto &sec : debugSections)
    {
        coffWriter.addExtraSection(sec);
    }
    const std::vector<uint8_t> coffBytes = coffWriter.write(sections);
    EXPECT_GT(coffBytes.size(), 64u);
}

TEST_F(EzCodeEmitterTestSuite, TestFunctionPointerBitfieldLeafAndRiscVCfa)
{
    // 1. DWARF5 x86-64: function pointer type, bitfield struct, leaf function (!hasFramePointer)
    {
        DebugCompileUnit cu;
        cu.level = DebugLevel::Full;
        cu.compDir = "/workspace";
        cu.mainFile = "/workspace/test.c";
        cu.pointerSize = 8;
        cu.arch = DebugCompileUnit::TargetArch::X86_64;
        const uint32_t fId = cu.addOrGetFile("/workspace/test.c");

        DebugType intTy;
        intTy.typeId = 1;
        intTy.name = "int";
        intTy.kind = EzCore::RttiTypeKind::SignedInt;
        intTy.encoding = DebugBaseEncoding::Signed;
        intTy.sizeInBytes = 4;
        cu.addType(intTy);

        DebugType fnTy;
        fnTy.typeId = 2;
        fnTy.kind = EzCore::RttiTypeKind::Function;
        fnTy.returnTypeId = 1;
        fnTy.paramTypeIds = { 1 };
        fnTy.isVariadic = true;
        cu.addType(fnTy);

        DebugType fnPtrTy;
        fnPtrTy.typeId = 3;
        fnPtrTy.kind = EzCore::RttiTypeKind::Pointer;
        fnPtrTy.sizeInBytes = 8;
        fnPtrTy.referencedTypeId = 2;
        cu.addType(fnPtrTy);

        DebugType flagsTy;
        flagsTy.typeId = 4;
        flagsTy.name = "Flags";
        flagsTy.kind = EzCore::RttiTypeKind::Struct;
        flagsTy.sizeInBytes = 4;
        flagsTy.declLoc = { fId, 2, 1 };
        flagsTy.members = { { "active", 1, 0, 0, 1, true }, { "mode", 1, 0, 1, 3, true } };
        cu.addType(flagsTy);

        DebugFunction leafFn;
        leafFn.name = "leaf_fn";
        leafFn.returnTypeId = 1;
        leafFn.declLoc = { fId, 5, 1 };
        leafFn.sectionOffset = 0;
        leafFn.codeSize = 12;
        leafFn.prologueSize = 4;
        leafFn.hasFramePointer = false;
        leafFn.totalFrameSize = 16;
        leafFn.savedRegs = { { 3, false, -16 } }; // RBX saved at CFA-16
        leafFn.lineEntries = { { 0, fId, 5, 1, true, false, false } };
        cu.functions.push_back(leafFn);

        Dwarf5Emitter dwarf;
        auto dSecs = dwarf.emit(cu);
        std::pmr::unordered_map<SectionType, CodeSection *> secs(getAllocator());
        Helpers::ObjectFormat::CreateElfSections(secs, getAllocator());
        for (int i = 0; i < 12; ++i) secs[SectionType::Text]->emit8(0x90);
        secs[SectionType::Text]->finalize();

        Elf64Writer elf;
        elf.setMachine(62);
        elf.addSymbol({ .m_name = "leaf_fn", .m_section = SectionType::Text, .m_offset = 0, .m_size = 12, .m_isGlobal = true, .m_isFunction = true });
        for (const auto &s : dSecs) elf.addExtraSection(s);

        const auto bytes = elf.write(secs);
        const std::filesystem::path tmpObj = std::filesystem::temp_directory_path() / "ezpacker_test_leaf_x64.o";
        {
            std::ofstream ofs(tmpObj, std::ios::binary);
            ofs.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }
        int exitCode = -1;
        const std::string out = runCommandAndCapture(
                "LC_ALL=C readelf --debug-dump=info,abbrev,line,str,frames " + tmpObj.string() + " 2>&1", exitCode);
        std::filesystem::remove(tmpObj);

        EXPECT_EQ(exitCode, 0) << out;
        EXPECT_NE(out.find("DW_TAG_subroutine_type"), std::string::npos) << out;
        EXPECT_NE(out.find("DW_TAG_unspecified_parameters"), std::string::npos) << out;
        EXPECT_NE(out.find("DW_AT_data_bit_offset"), std::string::npos) << out;
        EXPECT_NE(out.find("DW_CFA_def_cfa_offset: 24"), std::string::npos) << out;
        EXPECT_NE(out.find("DW_CFA_offset: r3 (rbx) at cfa-16"), std::string::npos) << out;

        // Also check CodeView LF_BITFIELD (0x1205) & LF_PROCEDURE (0x1008)
        CodeViewEmitter cv;
        auto cvSecs = cv.emit(cu);
        ASSERT_EQ(cvSecs.size(), 2u);
        const auto &debugT = cvSecs[0].m_data;
        bool sawBitfield = false;
        bool sawProc = false;
        size_t posT = 4;
        while (posT < debugT.size())
        {
            const uint16_t recLen = readU16(debugT, posT);
            const uint16_t leaf = readU16(debugT, posT + 2);
            if (leaf == 0x1205) sawBitfield = true;
            if (leaf == 0x1008) sawProc = true;
            posT += 2 + recLen;
        }
        EXPECT_TRUE(sawBitfield);
        EXPECT_TRUE(sawProc);
    }

    // 2. DWARF5 RISC-V 64: s0 = CFA (DW_CFA_def_cfa: r8 (s0) ofs 0)
    {
        DebugCompileUnit cu;
        cu.level = DebugLevel::Full;
        cu.compDir = "/workspace";
        cu.mainFile = "/workspace/rv.c";
        cu.pointerSize = 8;
        cu.arch = DebugCompileUnit::TargetArch::RiscV64;
        const uint32_t fId = cu.addOrGetFile("/workspace/rv.c");

        DebugFunction fn;
        fn.name = "rv_fn";
        fn.declLoc = { fId, 1, 1 };
        fn.sectionOffset = 0;
        fn.codeSize = 24;
        fn.prologueSize = 16;
        fn.hasFramePointer = true;
        fn.totalFrameSize = 32;
        fn.lineEntries = { { 0, fId, 1, 1, true, false, false } };
        cu.functions.push_back(fn);

        Dwarf5Emitter dwarf;
        auto dSecs = dwarf.emit(cu);
        std::pmr::unordered_map<SectionType, CodeSection *> secs(getAllocator());
        Helpers::ObjectFormat::CreateElfSections(secs, getAllocator());
        for (int i = 0; i < 24; ++i) secs[SectionType::Text]->emit8(0x13);
        secs[SectionType::Text]->finalize();

        Elf64Writer elf;
        elf.setMachine(243);
        elf.addSymbol({ .m_name = "rv_fn", .m_section = SectionType::Text, .m_offset = 0, .m_size = 24, .m_isGlobal = true, .m_isFunction = true });
        for (const auto &s : dSecs) elf.addExtraSection(s);

        const auto bytes = elf.write(secs);
        const std::filesystem::path tmpObj = std::filesystem::temp_directory_path() / "ezpacker_test_cfa_rv64.o";
        {
            std::ofstream ofs(tmpObj, std::ios::binary);
            ofs.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        }
        int exitCode = -1;
        const std::string out = runCommandAndCapture(
                "LC_ALL=C readelf --debug-dump=frames " + tmpObj.string() + " 2>&1", exitCode);
        std::filesystem::remove(tmpObj);

        EXPECT_EQ(exitCode, 0) << out;
        EXPECT_NE(out.find("DW_CFA_def_cfa: r8 (s0) ofs 0"), std::string::npos) << out;
        EXPECT_NE(out.find("DW_CFA_offset: r1 (ra) at cfa-8"), std::string::npos) << out;
        EXPECT_NE(out.find("DW_CFA_offset: r8 (s0) at cfa-16"), std::string::npos) << out;
    }
}

