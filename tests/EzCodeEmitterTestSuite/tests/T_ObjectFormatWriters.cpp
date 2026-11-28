#include "EzCodeEmitterTestSuite.h"
#include "Debug/IDebugInfoEmitter.h"
#include "Helpers.h"
#include "ObjectFormat/Elf64Writer.h"
#include "ObjectFormat/CoffWriter.h"

#include <stdexcept>

using namespace EzCodeEmitter;
using namespace EzCodeEmitter::ObjectFormat;

// Serializes text/rodata sections plus symbols and relocations into an ELF64 object and validates the header fields.
TEST_F(EzCodeEmitterTestSuite, TestElf64ObjectWriter)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeSection *textSec = sections[SectionType::Text];
    CodeSection *rodataSec = sections[SectionType::ReadOnly];

    // Emit machine code: mov eax, 42; ret -> B8 2A 00 00 00 C3
    textSec->emit8(0xB8);
    textSec->emit32(42);
    textSec->emit8(0xC3);

    // Emit rodata: "Hello EzPacker\0"
    const char *helloStr = "Hello EzPacker";
    rodataSec->emitBytes(reinterpret_cast<const uint8_t *>(helloStr), std::strlen(helloStr) + 1);

    textSec->finalize();
    rodataSec->finalize();

    Elf64Writer elfWriter;
    ObjectSymbol symMain{ .m_name = "main",
                          .m_section = SectionType::Text,
                          .m_offset = 0,
                          .m_size = 6,
                          .m_isGlobal = true,
                          .m_isFunction = true };
    elfWriter.addSymbol(symMain);

    ObjectSymbol symStr{ .m_name = "hello_str",
                         .m_section = SectionType::ReadOnly,
                         .m_offset = 0,
                         .m_size = std::strlen(helloStr) + 1,
                         .m_isGlobal = false,
                         .m_isFunction = false };
    elfWriter.addSymbol(symStr);

    ObjectRelocEntry reloc{ .m_section = SectionType::Text,
                            .m_offset = 1, // Offset of imm32 in mov
                            .m_symbolName = "hello_str",
                            .m_type = TargetCodeRelocationType::PCRel32,
                            .m_addend = -4 };
    elfWriter.addRelocation(reloc);

    std::vector<uint8_t> elfBytes = elfWriter.write(sections);

    // Assert minimum ELF header size (64 bytes)
    ASSERT_GE(elfBytes.size(), 64u);

    // 1. Magic bytes "\x7fELF"
    EXPECT_EQ(elfBytes[0], 0x7F);
    EXPECT_EQ(elfBytes[1], 'E');
    EXPECT_EQ(elfBytes[2], 'L');
    EXPECT_EQ(elfBytes[3], 'F');

    // 2. Class: 2 (ELFCLASS64)
    EXPECT_EQ(elfBytes[4], 2);

    // 3. Data: 1 (ELFDATA2LSB / little-endian)
    EXPECT_EQ(elfBytes[5], 1);

    // 4. Version: 1 (EV_CURRENT)
    EXPECT_EQ(elfBytes[6], 1);

    // 5. Type: 1 (ET_REL)
    uint16_t e_type = 0;
    std::memcpy(&e_type, &elfBytes[16], 2);
    EXPECT_EQ(e_type, 1);

    // 6. Machine: 62 (EM_X86_64)
    uint16_t e_machine = 0;
    std::memcpy(&e_machine, &elfBytes[18], 2);
    EXPECT_EQ(e_machine, 62);

    // 7. Section header offset and count
    uint64_t e_shoff = 0;
    std::memcpy(&e_shoff, &elfBytes[40], 8);
    EXPECT_GT(e_shoff, 0u);

    uint16_t e_shnum = 0;
    std::memcpy(&e_shnum, &elfBytes[60], 2);
    EXPECT_GE(e_shnum, 6u); // NULL, .text, .rodata, .symtab, .strtab, .shstrtab, .rela.text
}

// Serializes text/data sections plus symbols and relocations into a COFF object and validates the header fields.
TEST_F(EzCodeEmitterTestSuite, TestCoffObjectWriter)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateCoffSections(sections, getAllocator());

    CodeSection *textSec = sections[SectionType::Text];
    CodeSection *dataSec = sections[SectionType::Data];

    // Emit machine code: mov eax, 42; ret
    textSec->emit8(0xB8);
    textSec->emit32(42);
    textSec->emit8(0xC3);

    // Emit data: 4-byte int 100
    dataSec->emit32(100);

    textSec->finalize();
    dataSec->finalize();

    CoffWriter coffWriter;
    ObjectSymbol symFunc{ .m_name = "my_function_with_long_name",
                          .m_section = SectionType::Text,
                          .m_offset = 0,
                          .m_size = 6,
                          .m_isGlobal = true,
                          .m_isFunction = true };
    coffWriter.addSymbol(symFunc);

    ObjectSymbol symData{ .m_name = "global_var",
                          .m_section = SectionType::Data,
                          .m_offset = 0,
                          .m_size = 4,
                          .m_isGlobal = true,
                          .m_isFunction = false };
    coffWriter.addSymbol(symData);

    ObjectRelocEntry reloc{ .m_section = SectionType::Text,
                            .m_offset = 1,
                            .m_symbolName = "global_var",
                            .m_type = TargetCodeRelocationType::PCRel32,
                            .m_addend = 0 };
    coffWriter.addRelocation(reloc);

    std::vector<uint8_t> coffBytes = coffWriter.write(sections);

    // Assert minimum COFF file header size (20 bytes)
    ASSERT_GE(coffBytes.size(), 20u);

    // 1. Machine: 0x8664 (IMAGE_FILE_MACHINE_AMD64)
    uint16_t machine = 0;
    std::memcpy(&machine, &coffBytes[0], 2);
    EXPECT_EQ(machine, 0x8664);

    // 2. Number of sections >= 2 (.text, .data, .rdata, .bss)
    uint16_t numSections = 0;
    std::memcpy(&numSections, &coffBytes[2], 2);
    EXPECT_GE(numSections, 2u);

    // 3. Pointer to Symbol Table
    uint32_t symTablePtr = 0;
    std::memcpy(&symTablePtr, &coffBytes[8], 4);
    EXPECT_GT(symTablePtr, 0u);

    // 4. Number of symbols
    uint32_t numSymbols = 0;
    std::memcpy(&numSymbols, &coffBytes[12], 4);
    EXPECT_EQ(numSymbols, 2u);
}

// Regression for the compiler WEI-01 lead: getCurrentOffset must include pending alignment padding
// so symbols recorded after alignTo() are not placed at the pre-padding offset.
TEST_F(EzCodeEmitterTestSuite, TestCurrentOffsetAccountsForAlignment)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeSection *dataSec = sections[SectionType::Data];
    ASSERT_NE(dataSec, nullptr);

    dataSec->emit8(0xAA);
    EXPECT_EQ(dataSec->getCurrentOffset(), 1u);

    // The next emitted byte must start at an 8-byte boundary.
    dataSec->alignTo(8);
    EXPECT_EQ(dataSec->getCurrentOffset(), 8u);

    dataSec->emit8(0xBB);
    EXPECT_EQ(dataSec->getCurrentOffset(), 9u);

    dataSec->finalize();
    ASSERT_EQ(dataSec->getData().size(), 9u);
    EXPECT_EQ(dataSec->getData()[0], 0xAA);
    EXPECT_EQ(dataSec->getData()[8], 0xBB);
}

// LEG-09: a defined symbol in a section the writer does not serialize must fail loudly instead of
// silently becoming an undefined (section 0) symbol.
TEST_F(EzCodeEmitterTestSuite, TestWritersRejectUnsupportedSections)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());
    sections[SectionType::Text]->finalize();

    ObjectSymbol customSym{ .m_name = "custom_data",
                            .m_section = SectionType::Custom,
                            .m_offset = 0,
                            .m_size = 4,
                            .m_isGlobal = true,
                            .m_isFunction = false };

    Elf64Writer elfWriter;
    elfWriter.addSymbol(customSym);
    EXPECT_THROW(elfWriter.write(sections), std::runtime_error);

    // The same policy applies to COFF.
    std::pmr::unordered_map<SectionType, CodeSection *> coffSections(getAllocator());
    Helpers::ObjectFormat::CreateCoffSections(coffSections, getAllocator());
    coffSections[SectionType::Text]->finalize();

    CoffWriter coffWriter;
    coffWriter.addSymbol(customSym);
    EXPECT_THROW(coffWriter.write(coffSections), std::runtime_error);
}

// Verifies ELF64 symbol table emission for global, local, weak, and undefined symbols.
TEST_F(EzCodeEmitterTestSuite, TestElf64SymbolLinkageAndWeak)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());

    CodeSection *textSec = sections[SectionType::Text];
    textSec->emit8(0xC3); // ret
    textSec->finalize();

    Elf64Writer elfWriter;

    ObjectSymbol symGlobal{ .m_name = "sym_global",
                            .m_section = SectionType::Text,
                            .m_offset = 0,
                            .m_size = 1,
                            .m_isGlobal = true,
                            .m_isWeak = false,
                            .m_isFunction = true };
    elfWriter.addSymbol(symGlobal);

    ObjectSymbol symLocal{ .m_name = "sym_local",
                           .m_section = SectionType::Text,
                           .m_offset = 0,
                           .m_size = 1,
                           .m_isGlobal = false,
                           .m_isWeak = false,
                           .m_isFunction = true };
    elfWriter.addSymbol(symLocal);

    ObjectSymbol symWeak{ .m_name = "sym_weak",
                          .m_section = SectionType::Text,
                          .m_offset = 0,
                          .m_size = 1,
                          .m_isGlobal = true,
                          .m_isWeak = true,
                          .m_isFunction = true };
    elfWriter.addSymbol(symWeak);

    ObjectSymbol symExtern{ .m_name = "sym_extern",
                            .m_section = SectionType::Undefined,
                            .m_offset = 0,
                            .m_size = 0,
                            .m_isGlobal = true,
                            .m_isWeak = false,
                            .m_isFunction = true };
    elfWriter.addSymbol(symExtern);

    ObjectSymbol symWeakExtern{ .m_name = "sym_weak_extern",
                                .m_section = SectionType::Undefined,
                                .m_offset = 0,
                                .m_size = 0,
                                .m_isGlobal = true,
                                .m_isWeak = true,
                                .m_isFunction = true };
    elfWriter.addSymbol(symWeakExtern);

    std::vector<uint8_t> elfBytes = elfWriter.write(sections);
    ASSERT_GE(elfBytes.size(), 64u);

    // Locate section headers
    uint64_t e_shoff = 0;
    std::memcpy(&e_shoff, &elfBytes[40], 8);
    uint16_t e_shentsize = 0;
    std::memcpy(&e_shentsize, &elfBytes[58], 2);
    uint16_t e_shnum = 0;
    std::memcpy(&e_shnum, &elfBytes[60], 2);

    ASSERT_GT(e_shoff, 0u);
    ASSERT_GT(e_shnum, 0u);

    // Find .symtab and .strtab sections
    uint64_t symtabOffset = 0;
    uint64_t symtabSize = 0;
    uint32_t strtabSecIdx = 0;

    for (uint16_t i = 0; i < e_shnum; ++i)
    {
        const uint8_t *shdr = &elfBytes[e_shoff + i * e_shentsize];
        uint32_t sh_type = 0;
        std::memcpy(&sh_type, shdr + 4, 4);

        if (sh_type == 2) // SHT_SYMTAB
        {
            std::memcpy(&symtabOffset, shdr + 24, 8);
            std::memcpy(&symtabSize, shdr + 32, 8);
            std::memcpy(&strtabSecIdx, shdr + 40, 4);
            break;
        }
    }

    ASSERT_GT(symtabOffset, 0u);
    ASSERT_GT(symtabSize, 0u);

    // Read .strtab offset
    const uint8_t *strtabHdr = &elfBytes[e_shoff + strtabSecIdx * e_shentsize];
    uint64_t strtabOffset = 0;
    std::memcpy(&strtabOffset, strtabHdr + 24, 8);

    struct ParsedSym
    {
        std::string name;
        uint8_t bind;
        uint16_t shndx;
    };
    std::vector<ParsedSym> symbols;

    constexpr size_t ELF_SYM_SIZE = 24;
    size_t numSyms = symtabSize / ELF_SYM_SIZE;
    for (size_t i = 0; i < numSyms; ++i)
    {
        const uint8_t *symData = &elfBytes[symtabOffset + i * ELF_SYM_SIZE];
        uint32_t st_name = 0;
        std::memcpy(&st_name, symData, 4);
        uint8_t st_info = symData[4];
        uint16_t st_shndx = 0;
        std::memcpy(&st_shndx, symData + 6, 2);

        const char *namePtr = reinterpret_cast<const char *>(&elfBytes[strtabOffset + st_name]);
        symbols.push_back({ std::string(namePtr), static_cast<uint8_t>(st_info >> 4), st_shndx });
    }

    auto findSym = [&](std::string_view name) -> const ParsedSym * {
        for (const auto &s : symbols)
        {
            if (s.name == name) return &s;
        }
        return nullptr;
    };

    // STB_LOCAL = 0, STB_GLOBAL = 1, STB_WEAK = 2
    const ParsedSym *psGlobal = findSym("sym_global");
    ASSERT_NE(psGlobal, nullptr);
    EXPECT_EQ(psGlobal->bind, 1); // STB_GLOBAL
    EXPECT_NE(psGlobal->shndx, 0);

    const ParsedSym *psLocal = findSym("sym_local");
    ASSERT_NE(psLocal, nullptr);
    EXPECT_EQ(psLocal->bind, 0); // STB_LOCAL
    EXPECT_NE(psLocal->shndx, 0);

    const ParsedSym *psWeak = findSym("sym_weak");
    ASSERT_NE(psWeak, nullptr);
    EXPECT_EQ(psWeak->bind, 2); // STB_WEAK
    EXPECT_NE(psWeak->shndx, 0);

    const ParsedSym *psExtern = findSym("sym_extern");
    ASSERT_NE(psExtern, nullptr);
    EXPECT_EQ(psExtern->bind, 1); // STB_GLOBAL
    EXPECT_EQ(psExtern->shndx, 0); // SHN_UNDEF

    const ParsedSym *psWeakExtern = findSym("sym_weak_extern");
    ASSERT_NE(psWeakExtern, nullptr);
    EXPECT_EQ(psWeakExtern->bind, 2); // STB_WEAK
    EXPECT_EQ(psWeakExtern->shndx, 0); // SHN_UNDEF
}

// Verifies ELF64 extra sections, flags, STT_SECTION symbols, .rela<sec> emission, and clear().
TEST_F(EzCodeEmitterTestSuite, TestElf64ExtraSectionsAndSectionRelocs)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());
    CodeSection *textSec = sections[SectionType::Text];
    textSec->emit8(0xC3);
    textSec->finalize();

    Elf64Writer writer;
    writer.addSymbol(ObjectSymbol{ .m_name = "local_fn",
                                   .m_section = SectionType::Text,
                                   .m_offset = 0,
                                   .m_size = 1,
                                   .m_isGlobal = false,
                                   .m_isFunction = true });
    writer.addSymbol(ObjectSymbol{ .m_name = "main",
                                   .m_section = SectionType::Text,
                                   .m_offset = 0,
                                   .m_size = 1,
                                   .m_isGlobal = true,
                                   .m_isFunction = true });

    ExtraObjectSection debugInfo;
    debugInfo.m_name = ".debug_info";
    debugInfo.m_data = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C };
    debugInfo.m_alignment = 1;
    debugInfo.m_isDebug = true;
    debugInfo.m_isAlloc = false;
    debugInfo.m_isMergeStrings = false;
    debugInfo.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                    .m_targetName = ".text",
                                                    .m_targetIsSection = true,
                                                    .m_type = TargetCodeRelocationType::Absolute64,
                                                    .m_addend = 0 });
    debugInfo.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 8,
                                                    .m_targetName = ".debug_str",
                                                    .m_targetIsSection = true,
                                                    .m_type = TargetCodeRelocationType::Absolute32,
                                                    .m_addend = 4 });
    writer.addExtraSection(debugInfo);

    ExtraObjectSection debugStr;
    debugStr.m_name = ".debug_str";
    debugStr.m_data = { 'f', 'o', 'o', '\0', 'b', 'a', 'r', '\0' };
    debugStr.m_alignment = 1;
    debugStr.m_isDebug = true;
    debugStr.m_isAlloc = false;
    debugStr.m_isMergeStrings = true;
    writer.addExtraSection(debugStr);

    ExtraObjectSection allocExtra;
    allocExtra.m_name = ".eh_frame_custom";
    allocExtra.m_data = { 0xAA, 0xBB };
    allocExtra.m_alignment = 8;
    allocExtra.m_isDebug = false;
    allocExtra.m_isAlloc = true;
    writer.addExtraSection(allocExtra);

    std::vector<uint8_t> elfBytes = writer.write(sections);
    ASSERT_GE(elfBytes.size(), 64u);

    uint64_t e_shoff = 0;
    std::memcpy(&e_shoff, &elfBytes[40], 8);
    uint16_t e_shentsize = 0;
    std::memcpy(&e_shentsize, &elfBytes[58], 2);
    uint16_t e_shnum = 0;
    std::memcpy(&e_shnum, &elfBytes[60], 2);
    uint16_t e_shstrndx = 0;
    std::memcpy(&e_shstrndx, &elfBytes[62], 2);

    const uint8_t *shstrHdr = &elfBytes[e_shoff + e_shstrndx * e_shentsize];
    uint64_t shstrOff = 0;
    std::memcpy(&shstrOff, shstrHdr + 24, 8);

    struct SecHeaderInfo
    {
        uint16_t idx;
        std::string name;
        uint32_t type;
        uint64_t flags;
        uint64_t offset;
        uint64_t size;
        uint32_t link;
        uint32_t info;
        uint64_t entsize;
    };
    std::vector<SecHeaderInfo> secHdrs;
    for (uint16_t i = 0; i < e_shnum; ++i)
    {
        const uint8_t *sh = &elfBytes[e_shoff + i * e_shentsize];
        SecHeaderInfo s{};
        s.idx = i;
        uint32_t sh_name = 0;
        std::memcpy(&sh_name, sh, 4);
        s.name = reinterpret_cast<const char *>(&elfBytes[shstrOff + sh_name]);
        std::memcpy(&s.type, sh + 4, 4);
        std::memcpy(&s.flags, sh + 8, 8);
        std::memcpy(&s.offset, sh + 24, 8);
        std::memcpy(&s.size, sh + 32, 8);
        std::memcpy(&s.link, sh + 40, 4);
        std::memcpy(&s.info, sh + 44, 4);
        std::memcpy(&s.entsize, sh + 56, 8);
        secHdrs.push_back(s);
    }

    auto findSec = [&](std::string_view name) -> const SecHeaderInfo * {
        for (const auto &s : secHdrs)
        {
            if (s.name == name)
                return &s;
        }
        return nullptr;
    };

    const SecHeaderInfo *shDebugInfo = findSec(".debug_info");
    const SecHeaderInfo *shDebugStr = findSec(".debug_str");
    const SecHeaderInfo *shAlloc = findSec(".eh_frame_custom");
    const SecHeaderInfo *shRelaDebug = findSec(".rela.debug_info");
    const SecHeaderInfo *shSymtab = findSec(".symtab");
    const SecHeaderInfo *shText = findSec(".text");

    ASSERT_NE(shDebugInfo, nullptr);
    EXPECT_EQ(shDebugInfo->type, 1u);  // SHT_PROGBITS
    EXPECT_EQ(shDebugInfo->flags, 0u); // non-alloc debug
    EXPECT_EQ(shDebugInfo->size, 12u);

    ASSERT_NE(shDebugStr, nullptr);
    EXPECT_EQ(shDebugStr->type, 1u);
    EXPECT_EQ(shDebugStr->flags, 0x30u); // SHF_MERGE | SHF_STRINGS
    EXPECT_EQ(shDebugStr->entsize, 1u);

    ASSERT_NE(shAlloc, nullptr);
    EXPECT_EQ(shAlloc->flags, 0x2u); // SHF_ALLOC

    ASSERT_NE(shSymtab, nullptr);
    ASSERT_NE(shRelaDebug, nullptr);
    EXPECT_EQ(shRelaDebug->type, 4u);    // SHT_RELA
    EXPECT_EQ(shRelaDebug->flags, 0x40u); // SHF_INFO_LINK
    EXPECT_EQ(shRelaDebug->link, shSymtab->idx);
    EXPECT_EQ(shRelaDebug->info, shDebugInfo->idx);
    EXPECT_EQ(shRelaDebug->size, 2u * 24u);

    // Verify STT_SECTION symbols precede local_fn and main, and sh_info == index of main.
    constexpr size_t ELF_SYM_SIZE = 24;
    size_t numSyms = shSymtab->size / ELF_SYM_SIZE;
    // NULL + 7 section symbols (.text, .rodata, .data, .bss, .debug_info, .debug_str, .eh_frame_custom) + local_fn + main = 10
    ASSERT_EQ(numSyms, 10u);
    EXPECT_EQ(shSymtab->info, 9u); // index of 'main' (1 NULL + 7 STT_SECTION + 1 local_fn)

    // Verify section symbols at indices 1..7
    uint16_t textSecSymIdx = 0;
    uint16_t debugStrSecSymIdx = 0;
    for (size_t i = 1; i <= 7; ++i)
    {
        const uint8_t *symData = &elfBytes[shSymtab->offset + i * ELF_SYM_SIZE];
        uint8_t st_info = symData[4];
        uint16_t st_shndx = 0;
        std::memcpy(&st_shndx, symData + 6, 2);
        EXPECT_EQ(st_info >> 4, 0u);   // STB_LOCAL
        EXPECT_EQ(st_info & 0xF, 3u); // STT_SECTION
        if (st_shndx == shText->idx)
            textSecSymIdx = static_cast<uint16_t>(i);
        if (st_shndx == shDebugStr->idx)
            debugStrSecSymIdx = static_cast<uint16_t>(i);
    }
    EXPECT_NE(textSecSymIdx, 0u);
    EXPECT_NE(debugStrSecSymIdx, 0u);

    // Verify .rela.debug_info entries
    uint64_t r0_info = 0;
    std::memcpy(&r0_info, &elfBytes[shRelaDebug->offset + 8], 8);
    EXPECT_EQ(static_cast<uint32_t>(r0_info >> 32), textSecSymIdx);
    EXPECT_EQ(static_cast<uint32_t>(r0_info & 0xFFFFFFFFULL), 1u); // R_X86_64_64

    uint64_t r1_info = 0;
    int64_t r1_addend = 0;
    std::memcpy(&r1_info, &elfBytes[shRelaDebug->offset + 24 + 8], 8);
    std::memcpy(&r1_addend, &elfBytes[shRelaDebug->offset + 24 + 16], 8);
    EXPECT_EQ(static_cast<uint32_t>(r1_info >> 32), debugStrSecSymIdx);
    EXPECT_EQ(static_cast<uint32_t>(r1_info & 0xFFFFFFFFULL), 10u); // R_X86_64_32
    EXPECT_EQ(r1_addend, 4);

    // Verify clear() removes extra sections
    writer.clear();
    std::vector<uint8_t> clearedBytes = writer.write(sections);
    uint16_t clearedShnum = 0;
    std::memcpy(&clearedShnum, &clearedBytes[60], 2);
    EXPECT_EQ(clearedShnum, 8u); // NULL, .text, .rodata, .data, .bss, .symtab, .strtab, .shstrtab
}

// Verifies COFF extra sections, long section names, SECREL/SECTION relocations, and section symbols.
TEST_F(EzCodeEmitterTestSuite, TestCoffExtraSectionsLongNamesAndSecRel)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateCoffSections(sections, getAllocator());
    CodeSection *textSec = sections[SectionType::Text];
    textSec->emit8(0xC3);
    textSec->finalize();

    CoffWriter writer;
    writer.addSymbol(ObjectSymbol{ .m_name = "main",
                                   .m_section = SectionType::Text,
                                   .m_offset = 0,
                                   .m_size = 1,
                                   .m_isGlobal = true,
                                   .m_isFunction = true });

    // Short name <= 8 bytes: ".debug$S"
    ExtraObjectSection debugS;
    debugS.m_name = ".debug$S";
    debugS.m_data = { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    debugS.m_alignment = 4;
    debugS.m_isDebug = true;
    debugS.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 4,
                                                 .m_targetName = "main",
                                                 .m_targetIsSection = false,
                                                 .m_type = TargetCodeRelocationType::SecRel32 });
    debugS.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 8,
                                                 .m_targetName = ".text",
                                                 .m_targetIsSection = true,
                                                 .m_type = TargetCodeRelocationType::SectionIdx16 });
    writer.addExtraSection(debugS);

    // Long name > 8 bytes: ".debug_info"
    ExtraObjectSection debugInfo;
    debugInfo.m_name = ".debug_info";
    debugInfo.m_data = { 0x11, 0x22, 0x33, 0x44 };
    debugInfo.m_alignment = 1;
    debugInfo.m_isDebug = true;
    debugInfo.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                    .m_targetName = ".text",
                                                    .m_targetIsSection = true,
                                                    .m_type = TargetCodeRelocationType::SecRel32 });
    writer.addExtraSection(debugInfo);

    std::vector<uint8_t> coffBytes = writer.write(sections);
    ASSERT_GE(coffBytes.size(), 20u);

    uint16_t numSections = 0;
    uint32_t symTablePtr = 0;
    uint32_t numSymbols = 0;
    std::memcpy(&numSections, &coffBytes[2], 2);
    std::memcpy(&symTablePtr, &coffBytes[8], 4);
    std::memcpy(&numSymbols, &coffBytes[12], 4);

    ASSERT_EQ(numSections, 6u); // .text, .rdata, .data, .bss, .debug$S, .debug_info
    // Symbols: main + .text section symbol (deduplicated across two relocs!) = 2
    EXPECT_EQ(numSymbols, 2u);

    uint32_t strTableOffset = symTablePtr + numSymbols * 18u;
    uint32_t strTableSize = 0;
    std::memcpy(&strTableSize, &coffBytes[strTableOffset], 4);
    EXPECT_GT(strTableSize, 4u);

    // Check section headers (starting at offset 20, each 40 bytes; extra sections start at index 4)
    const uint8_t *shDebugS = &coffBytes[20 + 4 * 40];
    EXPECT_EQ(std::string_view(reinterpret_cast<const char *>(shDebugS), 8), ".debug$S");
    uint32_t debugSChars = 0;
    std::memcpy(&debugSChars, shDebugS + 36, 4);
    EXPECT_EQ(debugSChars, 0x42300040u); // align 4 debug

    uint32_t debugSRelocPtr = 0;
    uint16_t debugSNumRelocs = 0;
    std::memcpy(&debugSRelocPtr, shDebugS + 24, 4);
    std::memcpy(&debugSNumRelocs, shDebugS + 32, 2);
    ASSERT_EQ(debugSNumRelocs, 2u);

    uint16_t r0Type = 0;
    std::memcpy(&r0Type, &coffBytes[debugSRelocPtr + 8], 2);
    EXPECT_EQ(r0Type, 0x000Bu); // IMAGE_REL_AMD64_SECREL

    uint16_t r1Type = 0;
    std::memcpy(&r1Type, &coffBytes[debugSRelocPtr + 10 + 8], 2);
    EXPECT_EQ(r1Type, 0x000Au); // IMAGE_REL_AMD64_SECTION

    const uint8_t *shDebugInfo = &coffBytes[20 + 5 * 40];
    EXPECT_EQ(shDebugInfo[0], '/');
    uint32_t nameOff = static_cast<uint32_t>(std::stoul(reinterpret_cast<const char *>(shDebugInfo + 1)));
    ASSERT_LT(nameOff, strTableSize);
    EXPECT_STREQ(reinterpret_cast<const char *>(&coffBytes[strTableOffset + nameOff]), ".debug_info");

    uint32_t debugInfoChars = 0;
    std::memcpy(&debugInfoChars, shDebugInfo + 36, 4);
    EXPECT_EQ(debugInfoChars, 0x42100040u); // align 1 debug

    // Verify clear() removes extra sections
    writer.clear();
    std::vector<uint8_t> clearedBytes = writer.write(sections);
    uint16_t clearedSections = 0;
    std::memcpy(&clearedSections, &clearedBytes[2], 2);
    EXPECT_EQ(clearedSections, 4u); // .text, .rdata, .data, .bss only
}

// Verifies ELF64 and COFF fail loudly when an extra section relocation targets a missing section or symbol.
TEST_F(EzCodeEmitterTestSuite, TestExtraSectionRelocsFailLoudOnMissingTarget)
{
    std::pmr::unordered_map<SectionType, CodeSection *> elfSections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(elfSections, getAllocator());
    elfSections[SectionType::Text]->finalize();

    {
        Elf64Writer elfWriter;
        ExtraObjectSection sec;
        sec.m_name = ".debug_info";
        sec.m_data = { 0, 0, 0, 0 };
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                  .m_targetName = ".nonexistent",
                                                  .m_targetIsSection = true,
                                                  .m_type = TargetCodeRelocationType::Absolute32 });
        elfWriter.addExtraSection(sec);
        EXPECT_THROW(elfWriter.write(elfSections), std::runtime_error);
    }

    {
        Elf64Writer elfWriter;
        ExtraObjectSection sec;
        sec.m_name = ".debug_info";
        sec.m_data = { 0, 0, 0, 0 };
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                  .m_targetName = "missing_sym",
                                                  .m_targetIsSection = false,
                                                  .m_type = TargetCodeRelocationType::Absolute64 });
        elfWriter.addExtraSection(sec);
        EXPECT_THROW(elfWriter.write(elfSections), std::runtime_error);
    }

    std::pmr::unordered_map<SectionType, CodeSection *> coffSections(getAllocator());
    Helpers::ObjectFormat::CreateCoffSections(coffSections, getAllocator());
    coffSections[SectionType::Text]->finalize();

    {
        CoffWriter coffWriter;
        ExtraObjectSection sec;
        sec.m_name = ".debug$S";
        sec.m_data = { 0, 0, 0, 0 };
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                  .m_targetName = ".nonexistent",
                                                  .m_targetIsSection = true,
                                                  .m_type = TargetCodeRelocationType::SecRel32 });
        coffWriter.addExtraSection(sec);
        EXPECT_THROW(coffWriter.write(coffSections), std::runtime_error);
    }

    {
        CoffWriter coffWriter;
        ExtraObjectSection sec;
        sec.m_name = ".debug$S";
        sec.m_data = { 0, 0, 0, 0 };
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                  .m_targetName = "missing_sym",
                                                  .m_targetIsSection = false,
                                                  .m_type = TargetCodeRelocationType::SecRel32 });
        coffWriter.addExtraSection(sec);
        EXPECT_THROW(coffWriter.write(coffSections), std::runtime_error);
    }
}

// Verifies ELF64 standard relocations in .data emit .rela.data and unknown m_relocs symbols fail loud.
TEST_F(EzCodeEmitterTestSuite, TestElf64DataRelocsAndUnknownSymbolFailLoud)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateElfSections(sections, getAllocator());
    sections[SectionType::Text]->emit8(0xC3);
    sections[SectionType::Text]->finalize();

    CodeSection *dataSec = sections[SectionType::Data];
    dataSec->emit64(0);
    dataSec->finalize();

    {
        Elf64Writer elfWriter;
        elfWriter.addSymbol(ObjectSymbol{ .m_name = "target_fn",
                                          .m_section = SectionType::Text,
                                          .m_offset = 0,
                                          .m_size = 1,
                                          .m_isGlobal = true,
                                          .m_isFunction = true });
        elfWriter.addRelocation(ObjectRelocEntry{ .m_section = SectionType::Data,
                                                  .m_offset = 0,
                                                  .m_symbolName = "target_fn",
                                                  .m_type = TargetCodeRelocationType::Absolute64,
                                                  .m_addend = 0 });

        std::vector<uint8_t> elfBytes = elfWriter.write(sections);
        uint64_t e_shoff = 0;
        std::memcpy(&e_shoff, &elfBytes[40], 8);
        uint16_t e_shentsize = 0;
        std::memcpy(&e_shentsize, &elfBytes[58], 2);
        uint16_t e_shnum = 0;
        std::memcpy(&e_shnum, &elfBytes[60], 2);
        uint16_t e_shstrndx = 0;
        std::memcpy(&e_shstrndx, &elfBytes[62], 2);

        const uint8_t *shstrHdr = &elfBytes[e_shoff + e_shstrndx * e_shentsize];
        uint64_t shstrOff = 0;
        std::memcpy(&shstrOff, shstrHdr + 24, 8);

        uint16_t dataIdx = 0;
        uint32_t relaDataInfo = 0;
        bool foundRelaData = false;
        bool foundRelaText = false;
        for (uint16_t i = 0; i < e_shnum; ++i)
        {
            const uint8_t *sh = &elfBytes[e_shoff + i * e_shentsize];
            uint32_t sh_name = 0;
            std::memcpy(&sh_name, sh, 4);
            std::string name(reinterpret_cast<const char *>(&elfBytes[shstrOff + sh_name]));
            if (name == ".data")
                dataIdx = i;
            if (name == ".rela.data")
            {
                foundRelaData = true;
                std::memcpy(&relaDataInfo, sh + 44, 4);
            }
            if (name == ".rela.text")
                foundRelaText = true;
        }
        EXPECT_TRUE(foundRelaData);
        EXPECT_FALSE(foundRelaText);
        EXPECT_EQ(relaDataInfo, dataIdx);
    }

    // Fail-loud on unknown symbol in standard m_relocs (ELF64 & COFF)
    {
        Elf64Writer elfWriter;
        elfWriter.addRelocation(ObjectRelocEntry{ .m_section = SectionType::Text,
                                                  .m_offset = 0,
                                                  .m_symbolName = "does_not_exist",
                                                  .m_type = TargetCodeRelocationType::PCRel32 });
        EXPECT_THROW(elfWriter.write(sections), std::runtime_error);
    }

    {
        std::pmr::unordered_map<SectionType, CodeSection *> coffSections(getAllocator());
        Helpers::ObjectFormat::CreateCoffSections(coffSections, getAllocator());
        coffSections[SectionType::Text]->emit8(0xC3);
        coffSections[SectionType::Text]->finalize();

        CoffWriter coffWriter;
        coffWriter.addRelocation(ObjectRelocEntry{ .m_section = SectionType::Text,
                                                   .m_offset = 0,
                                                   .m_symbolName = "does_not_exist",
                                                   .m_type = TargetCodeRelocationType::PCRel32 });
        EXPECT_THROW(coffWriter.write(coffSections), std::runtime_error);
    }
}

// Verifies COFF ExtraSectionReloc::m_addend patches section bytes and rejects out-of-bounds offsets.
TEST_F(EzCodeEmitterTestSuite, TestCoffExtraSectionAddendPatching)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateCoffSections(sections, getAllocator());
    sections[SectionType::Text]->emit8(0xC3);
    sections[SectionType::Text]->finalize();

    {
        CoffWriter coffWriter;
        coffWriter.addSymbol(ObjectSymbol{ .m_name = "main",
                                           .m_section = SectionType::Text,
                                           .m_offset = 0,
                                           .m_size = 1,
                                           .m_isGlobal = true,
                                           .m_isFunction = true });

        ExtraObjectSection sec;
        sec.m_name = ".debug$S";
        sec.m_data.resize(12, 0);
        sec.m_alignment = 4;
        sec.m_isDebug = true;
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                  .m_targetName = "main",
                                                  .m_targetIsSection = false,
                                                  .m_type = TargetCodeRelocationType::SecRel32,
                                                  .m_addend = 0x1234 });
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 4,
                                                  .m_targetName = ".text",
                                                  .m_targetIsSection = true,
                                                  .m_type = TargetCodeRelocationType::Absolute64,
                                                  .m_addend = 0x1122334455667788LL });
        coffWriter.addExtraSection(sec);

        std::vector<uint8_t> coffBytes = coffWriter.write(sections);
        const uint8_t *shDebugS = &coffBytes[20 + 4 * 40];
        uint32_t ptrRaw = 0;
        std::memcpy(&ptrRaw, shDebugS + 20, 4);
        ASSERT_GT(ptrRaw, 0u);

        int32_t patched32 = 0;
        int64_t patched64 = 0;
        std::memcpy(&patched32, &coffBytes[ptrRaw], 4);
        std::memcpy(&patched64, &coffBytes[ptrRaw + 4], 8);
        EXPECT_EQ(patched32, 0x1234);
        EXPECT_EQ(patched64, 0x1122334455667788LL);
    }

    // Out-of-bounds addend must throw
    {
        CoffWriter coffWriter;
        ExtraObjectSection sec;
        sec.m_name = ".debug$S";
        sec.m_data.resize(4, 0);
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 2,
                                                  .m_targetName = ".text",
                                                  .m_targetIsSection = true,
                                                  .m_type = TargetCodeRelocationType::SecRel32,
                                                  .m_addend = 16 });
        coffWriter.addExtraSection(sec);
        EXPECT_THROW(coffWriter.write(sections), std::runtime_error);
    }
}

// Verifies COFF IMAGE_SCN_LNK_NRELOC_OVFL when a section has > 65535 relocations.
TEST_F(EzCodeEmitterTestSuite, TestCoffRelocationOverflowNrelocOvfl)
{
    std::pmr::unordered_map<SectionType, CodeSection *> sections(getAllocator());
    Helpers::ObjectFormat::CreateCoffSections(sections, getAllocator());
    sections[SectionType::Text]->emit8(0xC3);
    sections[SectionType::Text]->finalize();

    CoffWriter coffWriter;
    ExtraObjectSection sec;
    sec.m_name = ".debug$S";
    sec.m_data.resize(4, 0);
    constexpr uint32_t kNumRelocs = 65536;
    sec.m_relocs.reserve(kNumRelocs);
    for (uint32_t i = 0; i < kNumRelocs; ++i)
    {
        sec.m_relocs.push_back(ExtraSectionReloc{ .m_offset = 0,
                                                  .m_targetName = ".text",
                                                  .m_targetIsSection = true,
                                                  .m_type = TargetCodeRelocationType::SecRel32 });
    }
    coffWriter.addExtraSection(sec);

    std::vector<uint8_t> coffBytes = coffWriter.write(sections);
    const uint8_t *shDebugS = &coffBytes[20 + 4 * 40];
    uint32_t ptrRelocs = 0;
    uint16_t numRelocs = 0;
    uint32_t chars = 0;
    std::memcpy(&ptrRelocs, shDebugS + 24, 4);
    std::memcpy(&numRelocs, shDebugS + 32, 2);
    std::memcpy(&chars, shDebugS + 36, 4);

    EXPECT_NE(chars & 0x01000000u, 0u); // IMAGE_SCN_LNK_NRELOC_OVFL
    EXPECT_EQ(numRelocs, 0xFFFFu);
    ASSERT_GT(ptrRelocs, 0u);

    uint32_t ovflCount = 0;
    uint32_t ovflSym = 1;
    uint16_t ovflType = 1;
    std::memcpy(&ovflCount, &coffBytes[ptrRelocs], 4);
    std::memcpy(&ovflSym, &coffBytes[ptrRelocs + 4], 4);
    std::memcpy(&ovflType, &coffBytes[ptrRelocs + 8], 2);

    EXPECT_EQ(ovflCount, kNumRelocs + 1);
    EXPECT_EQ(ovflSym, 0u);
    EXPECT_EQ(ovflType, 0u);
}
