#ifndef EZPACKER_HELPERS_H
#define EZPACKER_HELPERS_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include <map>

namespace Helpers::ObjectFormat
{
/**
 * Instantiates and registers the default binary sections compliant with the Windows PE/COFF specification.
 *
 * Populates sectionMap with the following canonical section configurations:
 *  - SectionType::Text:             ".text"   (Executable, Read-Only, 16-byte alignment, 0x90 NOP padding)
 *  - SectionType::ReadOnly:         ".rdata"  (Read-Only, 16-byte alignment for SIMD/vectors, 0x00 padding)
 *  - SectionType::ReadOnlyWithRel:  ".rdata"  (Aliased to .rdata)
 *  - SectionType::CString:          ".rdata"  (Aliased to .rdata)
 *  - SectionType::Const4:           ".rdata"  (Aliased to .rdata)
 *  - SectionType::Const8:           ".rdata"  (Aliased to .rdata)
 *  - SectionType::Const16AndBigger: ".rdata"  (Aliased to .rdata)
 *  - SectionType::Data:             ".data"   (Readable, Writable, 8-byte alignment, 0x00 padding)
 *  - SectionType::DataWithRel:      ".data"   (Aliased to .data)
 *  - SectionType::NonInitialized:   ".bss"    (Readable, Writable, 8-byte alignment, uninitialized storage)
 *  - SectionType::Custom:           ".pdata"  (Read-Only, 4-byte alignment, exception tables / unwinding)
 *
 * All scalar encodings are configured as Little-Endian.
 *
 */
extern void CreateCoffSections(std::pmr::map<SectionType, CodeSection *> &sectionMap, std::pmr::memory_resource *alloc);

/**
 * Instantiates and registers the standard binary sections compliant with the System V / Linux ELF specification.
 *
 * Populates sectionMap with the following canonical section configurations:
 *  - SectionType::Text:             ".text"         (Executable, Read-Only, 16-byte alignment, 0x90 NOP padding)
 *  - SectionType::ReadOnly:         ".rodata"       (Read-Only, 16-byte alignment for SIMD/constants, 0x00 padding)
 *  - SectionType::ReadOnlyWithRel:  ".data.rel.ro"  (Read-Only, 8-byte alignment, relocatable constants, 0x00 padding)
 *  - SectionType::CString:          ".rodata"       (Aliased to .rodata)
 *  - SectionType::Const4:           ".rodata"       (Aliased to .rodata)
 *  - SectionType::Const8:           ".rodata"       (Aliased to .rodata)
 *  - SectionType::Const16AndBigger: ".rodata"       (Aliased to .rodata)
 *  - SectionType::Data:             ".data"         (Readable, Writable, 8-byte alignment, 0x00 padding)
 *  - SectionType::DataWithRel:      ".data"         (Aliased to .data)
 *  - SectionType::NonInitialized:   ".bss"          (Readable, Writable, 8-byte alignment, uninitialized SHT_NOBITS)
 *  - SectionType::Custom:           ".custom"       (Read-Only, 8-byte alignment, container for metadata / .eh_frame)
 *
 * All scalar encodings are configured as Little-Endian.
 *
 */
extern void CreateElfSections(std::pmr::map<SectionType, CodeSection *> &sectionMap, std::pmr::memory_resource *alloc);

/**
 * Instantiates and registers the standard binary sections compliant with the Apple Mach-O specification.
 *
 * Populates sectionMap with fine-grained sections mapped across the __TEXT and __DATA segments:
 *  - SectionType::Text:             "__text"        (__TEXT segment, Executable, Read-Only, 16-byte alignment, 0x90 NOP
 * padding)
 *  - SectionType::ReadOnly:         "__const"       (__TEXT segment, Read-Only, 16-byte alignment, 0x00 padding)
 *  - SectionType::ReadOnlyWithRel:  "__const_data"  (__DATA segment, Read-Only with relocations, 8-byte alignment, 0x00
 * padding)
 *  - SectionType::CString:          "__cstring"     (__TEXT segment, Read-Only, 1-byte alignment, null-terminated
 * strings)
 *  - SectionType::Const4:           "__literal4"    (__TEXT segment, Read-Only, 4-byte alignment, float/int literals)
 *  - SectionType::Const8:           "__literal8"    (__TEXT segment, Read-Only, 8-byte alignment, double/int literals)
 *  - SectionType::Const16AndBigger: "__literal16"   (__TEXT segment, Read-Only, 16-byte alignment, 128-bit/SIMD
 * literals)
 *  - SectionType::Data:             "__data"        (__DATA segment, Readable, Writable, 8-byte alignment, 0x00
 * padding)
 *  - SectionType::DataWithRel:      "__data"        (Aliased to __data)
 *  - SectionType::NonInitialized:   "__bss"         (__DATA segment, Readable, Writable, 8-byte alignment, zero-fill)
 *  - SectionType::Custom:           "__custom"      (Read-Only, 8-byte alignment, custom metadata / unwind info)
 *
 * All scalar encodings are configured as Little-Endian.
 *
 */
extern void CreateMachoSections(std::pmr::map<SectionType, CodeSection *> &sectionMap,
                                std::pmr::memory_resource *alloc);
}; // namespace Helpers::ObjectFormat

#endif // EZPACKER_HELPERS_H