#include "Helpers.h"

namespace Helpers::ObjectFormat
{

namespace
{
// Helper to allocate and construct a CodeSection using the PMR resource
CodeSection *AllocateSection(SectionFlags flags,
                             SectionType type,
                             size_t alignment,
                             uint8_t padByte,
                             std::string_view name,
                             TargetEndianness targetEndianness,
                             std::pmr::polymorphic_allocator<> &allocator)
{
    return allocator
            .new_object<CodeSection>(flags, type, alignment, targetEndianness, padByte, name, allocator.resource());
}
} // namespace

// =========================================================================
// 1. Windows PE / COFF Sections
// =========================================================================
void CreateCoffSections(std::pmr::unordered_map<SectionType, CodeSection *> &sectionMap,
                        std::pmr::memory_resource *alloc)
{
    std::pmr::polymorphic_allocator<> allocator(alloc);

    // .text: Executable code
    auto *textSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = true },
                                    SectionType::Text,
                                    16,
                                    0x90, // NOP padding
                                    ".text",
                                    TargetEndianness::Little,
                                    allocator);

    // .rdata: Read-only data, string literals, and constants
    auto *rdataSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                     SectionType::ReadOnly,
                                     16,
                                     0x00,
                                     ".rdata",
                                     TargetEndianness::Little,
                                     allocator);

    // .data: Mutable initialized data
    auto *dataSec = AllocateSection({ .m_readable = true, .m_writable = true, .m_executable = false },
                                    SectionType::Data,
                                    8,
                                    0x00,
                                    ".data",
                                    TargetEndianness::Little,
                                    allocator);

    // .bss: Uninitialized zero-fill memory
    auto *bssSec = AllocateSection({ .m_readable = true, .m_writable = true, .m_executable = false },
                                   SectionType::NonInitialized,
                                   8,
                                   0x00,
                                   ".bss",
                                   TargetEndianness::Little,
                                   allocator);

    // .pdata: Exception handling table
    auto *pdataSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                     SectionType::Custom,
                                     4,
                                     0x00,
                                     ".pdata",
                                     TargetEndianness::Little,
                                     allocator);

    // Primary Mappings
    sectionMap[SectionType::Text] = textSec;
    sectionMap[SectionType::Data] = dataSec;
    sectionMap[SectionType::DataWithRel] = dataSec;
    sectionMap[SectionType::NonInitialized] = bssSec;
    sectionMap[SectionType::Custom] = pdataSec;

    // Read-only aliases collapsing into .rdata
    sectionMap[SectionType::ReadOnly] = rdataSec;
    sectionMap[SectionType::ReadOnlyWithRel] = rdataSec;
    sectionMap[SectionType::CString] = rdataSec;
    sectionMap[SectionType::Const4] = rdataSec;
    sectionMap[SectionType::Const8] = rdataSec;
    sectionMap[SectionType::Const16AndBigger] = rdataSec;
}

// =========================================================================
// 2. Linux / BSD ELF Sections
// =========================================================================
void CreateElfSections(std::pmr::unordered_map<SectionType, CodeSection *> &sectionMap,
                       std::pmr::memory_resource *alloc)
{
    std::pmr::polymorphic_allocator<> allocator(alloc);

    // .text: Executable code
    auto *textSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = true },
                                    SectionType::Text,
                                    16,
                                    0x90, // NOP padding
                                    ".text",
                                    TargetEndianness::Little,
                                    allocator);

    // .rodata: General read-only data & merged literals
    auto *rodataSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                      SectionType::ReadOnly,
                                      16,
                                      0x00,
                                      ".rodata",
                                      TargetEndianness::Little,
                                      allocator);

    // .data.rel.ro: Read-only data containing relocation targets
    auto *dataRelRoSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                         SectionType::ReadOnlyWithRel,
                                         8,
                                         0x00,
                                         ".data.rel.ro",
                                         TargetEndianness::Little,
                                         allocator);

    // .data: Mutable initialized data
    auto *dataSec = AllocateSection({ .m_readable = true, .m_writable = true, .m_executable = false },
                                    SectionType::Data,
                                    8,
                                    0x00,
                                    ".data",
                                    TargetEndianness::Little,
                                    allocator);

    // .bss: Uninitialized zero-fill memory (SHT_NOBITS)
    auto *bssSec = AllocateSection({ .m_readable = true, .m_writable = true, .m_executable = false },
                                   SectionType::NonInitialized,
                                   8,
                                   0x00,
                                   ".bss",
                                   TargetEndianness::Little,
                                   allocator);

    // .custom / .eh_frame
    auto *customSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                      SectionType::Custom,
                                      8,
                                      0x00,
                                      ".custom",
                                      TargetEndianness::Little,
                                      allocator);

    // Primary Mappings
    sectionMap[SectionType::Text] = textSec;
    sectionMap[SectionType::ReadOnlyWithRel] = dataRelRoSec;
    sectionMap[SectionType::Data] = dataSec;
    sectionMap[SectionType::DataWithRel] = dataSec;
    sectionMap[SectionType::NonInitialized] = bssSec;
    sectionMap[SectionType::Custom] = customSec;

    // Read-only aliases collapsing into .rodata
    sectionMap[SectionType::ReadOnly] = rodataSec;
    sectionMap[SectionType::CString] = rodataSec;
    sectionMap[SectionType::Const4] = rodataSec;
    sectionMap[SectionType::Const8] = rodataSec;
    sectionMap[SectionType::Const16AndBigger] = rodataSec;
}

// =========================================================================
// 3. Apple Mach-O Sections
// =========================================================================
void CreateMachoSections(std::pmr::unordered_map<SectionType, CodeSection *> &sectionMap,
                         std::pmr::memory_resource *alloc)
{
    std::pmr::polymorphic_allocator<> allocator(alloc);

    // __TEXT,__text: Executable code
    auto *textSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = true },
                                    SectionType::Text,
                                    16,
                                    0x90, // NOP padding
                                    "__text",
                                    TargetEndianness::Little,
                                    allocator);

    // __TEXT,__const: General read-only constants without relocations
    auto *constSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                     SectionType::ReadOnly,
                                     16,
                                     0x00,
                                     "__const",
                                     TargetEndianness::Little,
                                     allocator);

    // __DATA,__const: Read-only constants with pointer relocations (relocated by dyld at load-time)
    auto *constDataSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                         SectionType::ReadOnlyWithRel,
                                         8,
                                         0x00,
                                         "__const_data",
                                         TargetEndianness::Little,
                                         allocator);

    // __TEXT,__cstring: Literal C-strings
    auto *cstringSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                       SectionType::CString,
                                       1,
                                       0x00,
                                       "__cstring",
                                       TargetEndianness::Little,
                                       allocator);

    // __TEXT,__literal4: 4-byte scalar constants
    auto *literal4Sec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                        SectionType::Const4,
                                        4,
                                        0x00,
                                        "__literal4",
                                        TargetEndianness::Little,
                                        allocator);

    // __TEXT,__literal8: 8-byte scalar constants
    auto *literal8Sec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                        SectionType::Const8,
                                        8,
                                        0x00,
                                        "__literal8",
                                        TargetEndianness::Little,
                                        allocator);

    // __TEXT,__literal16: 16-byte SIMD / vector constants
    auto *literal16Sec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                         SectionType::Const16AndBigger,
                                         16,
                                         0x00,
                                         "__literal16",
                                         TargetEndianness::Little,
                                         allocator);

    // __DATA,__data: Mutable initialized data
    auto *dataSec = AllocateSection({ .m_readable = true, .m_writable = true, .m_executable = false },
                                    SectionType::Data,
                                    8,
                                    0x00,
                                    "__data",
                                    TargetEndianness::Little,
                                    allocator);

    // __DATA,__bss: Uninitialized zero-fill memory
    auto *bssSec = AllocateSection({ .m_readable = true, .m_writable = true, .m_executable = false },
                                   SectionType::NonInitialized,
                                   8,
                                   0x00,
                                   "__bss",
                                   TargetEndianness::Little,
                                   allocator);

    // Custom metadata / unwind info
    auto *customSec = AllocateSection({ .m_readable = true, .m_writable = false, .m_executable = false },
                                      SectionType::Custom,
                                      8,
                                      0x00,
                                      "__custom",
                                      TargetEndianness::Little,
                                      allocator);

    sectionMap[SectionType::Text] = textSec;
    sectionMap[SectionType::ReadOnly] = constSec;
    sectionMap[SectionType::ReadOnlyWithRel] = constDataSec;
    sectionMap[SectionType::CString] = cstringSec;
    sectionMap[SectionType::Const4] = literal4Sec;
    sectionMap[SectionType::Const8] = literal8Sec;
    sectionMap[SectionType::Const16AndBigger] = literal16Sec;
    sectionMap[SectionType::Data] = dataSec;
    sectionMap[SectionType::DataWithRel] = dataSec;
    sectionMap[SectionType::NonInitialized] = bssSec;
    sectionMap[SectionType::Custom] = customSec;
}

} // namespace Helpers::ObjectFormat