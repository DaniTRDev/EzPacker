#ifndef EZPACKER_EZTESTTRIPLEBINARYDESC_H
#define EZPACKER_EZTESTTRIPLEBINARYDESC_H

#include "EzTestTripleCommon.h"

/**
 * ====================================================================================================================
 * EZTESTTRIPLE BINARY DESCRIPTOR (EzTestTripleBinaryDesc)
 * ====================================================================================================================
 *
 * Implements TargetBinaryDesc to define the OS/ABI-level binary conventions and memory layout rules for the
 * EzTestTriple backend (x86-64 target architecture).
 *
 * 1. Byte Ordering & Data Layout:
 *    - Byte Order: Little-Endian (LSB first) for all immediate encodings, wide scalar values (i128/i256), and
 *      constant pool byte arrays.
 *    - Pointer Size: 8 bytes (64-bit native virtual address width).
 *
 * 2. Alignment Hierarchy:
 *    - Functions: 16-byte boundary alignment on entry points to ensure optimal instruction prefetch caching.
 *    - Loops: 16-byte alignment on loop header blocks and critical branch targets.
 *    - Section .text: 16 bytes (native code buffer alignment).
 *    - Section .rodata: 16 bytes (accommodates 128-bit/256-bit SIMD constants, float literals, and VTables).
 *    - Section .data / .bss: 8 bytes (standard scalar and pointer alignment).
 *
 * 3. Addressing & Relocation Behavior:
 *    - Small Code Model: Restricts symbol references to a signed 32-bit (±2 GB) RIP-relative displacement window.
 *    - Large Code Model: Allows unconstrained 64-bit virtual address ranges via full 64-bit immediate materialization.
 *    - PIC/PIE: Enforces relative relocations (PCRel32, GOTPCREL, PLTRel32) and avoids absolute address fixups.
 *    - Non-PIC (Static): Permits direct absolute address link-time fixups (Absolute32, Absolute64).
 *
 * 4. Object Format Integration:
 *    - Container Formats: Configurable for ELF, COFF/PE, or Mach-O output.
 *
 * ====================================================================================================================
 */
class EzTestTripleBinaryDesc : public TargetBinaryDesc
{
  public:
    /**
     * Configuration parameters for customizing the target binary representation (ABI/OS-level) for testing purposes
     * without creating multiple ABIs:
     *
     *   • m_codeModel (TargetCodeModel):
     *       Determines the virtual address space and displacement rules for code/data accesses:
     *         - TargetCodeModel::Small: Assumes all symbols fit within a signed 32-bit (2 GB) PC-relative offset.
     *                             Enables compact RIP-relative addressing and 32-bit direct jumps/calls.
     *         - TargetCodeModel::Large: Makes no proximity assumptions; symbols can span the full 64-bit address space.
     *                             Forces full 64-bit absolute address materialization (e.g., movabs) and indirect
     * calls.
     *
     *   • m_objectFormat (TargetObjectFormat):
     *       Specifies the target container/object file format (TargetObjectFormat::ELF, TargetObjectFormat::COFF,
     *       or TargetObjectFormat::MachO) used by downstream object encoders (e.g., LIEF) to generate
     *       platform-compliant section headers and relocation records.
     *
     *   • m_isPIC (bool):
     *       Flags whether to generate Position-Independent Code (PIC/PIE):
     *         - true:  Code can execute at arbitrary runtime base addresses; avoids fixed absolute relocations
     *                  in favor of RIP-relative displacements, GOT indirections, or PLT stubs.
     *         - false: Direct absolute/static relocations (Absolute32, Absolute64) are permitted.
     *
     * ====================================================================================================================
     */
    struct Options
    {
        TargetCodeModel m_codeModel{ TargetCodeModel::Small };
        TargetObjectFormat m_objectFormat{ TargetObjectFormat::ELF };
        bool m_isPIC{ false };
    };

    EzTestTripleBinaryDesc(const Options &options, std::pmr::memory_resource *alloc);

    /**
     * Returns true (EzTestTriple follows x86-64 little-endian byte ordering).
     */
    bool isLittleEndian() const override;

    /**
     * Returns true if configured to generate position-independent code (PIC / PIE).
     */
    bool isPositionIndependent() const override;

    /**
     * Returns the canonical name of this binary descriptor.
     */
    const char *getName() const override;

    /**
     * Returns the a NEW section for each type. All of them have the same alignment.
     */
    virtual CodeSection *getSection(SectionType type) override;

    /**
     * Returns the active code model (Small or Large).
     */
    TargetCodeModel getCodeModel() const override;

    /**
     * Returns the binary object container format (ELF, COFF, MachO).
     */
    TargetObjectFormat getObjectFormat() const override;

    /**
     * Returns the byte alignment required for function entry points (16 bytes on x86-64).
     */
    size_t getFunctionAlignment() const override;

    /**
     * Returns the byte alignment recommended for loop branch targets (16 bytes).
     */
    size_t getLoopAlignment() const override;

    /**
     * Initializes the descriptor. Sets the sections depending on the object format.
     */
    void initialize() override;

    // Modifiers for testing configurations
    void setCodeModel(TargetCodeModel model) { m_options.m_codeModel = model; }
    void setPositionIndependent(bool isPic) { m_options.m_isPIC = isPic; }
    void setObjectFormat(TargetObjectFormat format) { m_options.m_objectFormat = format; }

  private:
    Options m_options;
    std::pmr::memory_resource *m_alloc;
    std::pmr::unordered_map<SectionType, CodeSection> m_sections;
};

#endif // EZPACKER_EZTESTTRIPLEBINARYDESC_H