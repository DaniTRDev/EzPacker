#ifndef EZPACKER_CODEEMITERCONTEXT_H
#define EZPACKER_CODEEMITERCONTEXT_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"

/**
 * Target-agnostic relocation types representing standard relocation fixups
 * required across various object formats (ELF, COFF/PE, Mach-O).
 */
enum class TargetCodeRelocationType : uint8_t
{
    /** No relocation needed or relocation is undefined. */
    None,

    /**
     * Direct 32-bit absolute address fixup (e.g., ELF R_X86_64_32 / COFF IMAGE_REL_AMD64_ADDR32).
     * The linker writes the full 32-bit virtual address of the symbol directly into the field.
     */
    Absolute32,

    /**
     * Direct 64-bit absolute address fixup (e.g., ELF R_X86_64_64 / COFF IMAGE_REL_AMD64_ADDR64).
     * Used by instructions loading 64-bit immediate pointers (such as x86-64 `movabs reg, imm64`).
     */
    Absolute64,

    /**
     * 32-bit signed PC-relative (RIP-relative) data displacement (e.g., ELF R_X86_64_PC32 / COFF
     * IMAGE_REL_AMD64_REL32). Computes the signed offset between the next instruction address (PC/RIP) and the target
     * symbol (e.g., `mov reg, [rip + symbol]`).
     */
    PCRel32,

    /**
     * 32-bit signed PC-relative branch or call offset (e.g., ELF R_X86_64_PLT32 or direct branch relocations).
     * Used specifically for control flow instructions (`call target`, `jmp target`, conditional jumps).
     */
    BranchRel32,

    /**
     * 32-bit PC-relative reference to a Global Offset Table (GOT) entry (e.g., ELF R_X86_64_GOTPCREL).
     * Used in Position-Independent Code (PIC) to resolve external/global symbols via an indirect pointer in the GOT.
     */
    GOTPCREL,

    /**
     * 32-bit PC-relative reference to a Procedure Linkage Table (PLT) entry (e.g., ELF R_X86_64_PLT32).
     * Used in shared libraries and dynamic linking to invoke external functions via dynamic stubs.
     */
    PLTRel32
};

/**
 * Structure used to contain the bare minimum information about an emitted label.
 */
struct CodeLabel
{
    // Section in which this label was defined.
    CodeSection *m_definingSection{ nullptr };
    MirId m_id{ MIRID_INVALID };
    uint64_t m_labelAddress{ 0 }; // Offset from the start of the section.
    std::string_view m_name{};
};

/**
 * This struct contains information about a relocation.
 */
struct CodeRelocation
{
    TargetCodeRelocationType m_relocType{ TargetCodeRelocationType::None };
    CodeSection *m_definingSection{ nullptr };
    MirReference *m_srcRef{ nullptr }; // Reference that caused the relocation to appear.
    uint64_t m_address{ 0 };           // Offset from the start of the section.
};

/**
 * Context responsible for managing labels, relocations, diagnostics, and allocations
 * during the code emission phase across functions and modules.
 */
class CodeEmitterContext
{
  public:
    /**
     * Constructs a code emission context with the specified diagnostic collector and PMR allocator.
     */
    CodeEmitterContext(DiagnosticCollector *diagCollector, std::pmr::memory_resource *alloc);

    /**
     * Frees ALL the resources allocated in this context (relocations and code labels). Any further use is undefined
     * behaviour.
     */
    ~CodeEmitterContext();

    /**
     * Looks up an existing label for the currently bound function by its MIR ID.
     * If the label does not exist, allocates a new CodeLabel via the context allocator,
     * registers it into the current function's label map, and returns a pointer to it.
     */
    CodeLabel *getOrCreateLabel(MirId id, const std::string_view &name);

    /**
     * Creates and registers a relocation for the given symbolic reference at the specified address/offset
     * in the current function's relocation map. If a relocation at this address already exists, it is overwritten.
     * Returns a pointer to the allocated CodeRelocation.
     */
    CodeRelocation *addReloc(MirReference *srcRef, uint64_t address);

    /**
     * Searches the module-level global relocation table for an entry recorded at the given address.
     * Returns the matching CodeRelocation pointer if found, or nullptr if no relocation exists at that address.
     */
    CodeRelocation *getReloc(uint64_t address);

    /**
     * Flushes the current function's local labels and relocations into the module-level persistent tables
     * associated with the provided function, and clears function-local scratch maps for the next function emission.
     */
    void resetFuncState(MirFunction *currentFunc);

  private:
    DiagnosticCollector *m_diagCollector;
    std::pmr::memory_resource *m_alloc;

    // Fast-lookup scratch maps active for the currently emitting function.
    std::pmr::unordered_map<MirId, CodeLabel *> m_currentFuncLabels;
    std::pmr::unordered_map<uint64_t, CodeRelocation *> m_currentFuncRelocs;

    // Persistent storage of function labels indexed by MirFunction pointer.
    std::pmr::unordered_map<MirFunction *, std::pmr::unordered_map<MirId, CodeLabel *>> m_labels;

    // Persistent module-wide relocation table aggregated across all emitted code sections.
    std::pmr::unordered_map<uint64_t, CodeRelocation *> m_relocations;
};

#endif // EZPACKER_CODEEMITERCONTEXT_H