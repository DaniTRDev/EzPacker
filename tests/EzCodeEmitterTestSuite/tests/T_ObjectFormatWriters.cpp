#include "EzCodeEmitterTestSuite.h"
#include "Helpers.h"
#include "ObjectFormat/Elf64Writer.h"
#include "ObjectFormat/CoffWriter.h"

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
