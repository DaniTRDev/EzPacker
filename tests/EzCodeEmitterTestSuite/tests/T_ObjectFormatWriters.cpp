#include "EzCodeEmitterTestSuite.h"
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
