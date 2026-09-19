#ifndef EZPACKER_CODEEMITERCONTEXT_H
#define EZPACKER_CODEEMITERCONTEXT_H

#include "EzCodeEmitterCommon.h"
#include "CodeSection.h"
#include <unordered_map>
#include <vector>

class DiagnosticCollector;
class MirReference;
class MirFunction;

/**
 * Target-agnostic relocation types representing standard relocation fixups
 * required across various object formats (ELF, COFF/PE, Mach-O).
 */
enum class TargetCodeRelocationType : uint8_t
{
    /** No relocation needed or relocation is undefined. */
    None,

    /** Direct 32-bit absolute address fixup. */
    Absolute32,

    /** Direct 64-bit absolute address fixup. */
    Absolute64,

    /** 32-bit signed PC-relative (RIP-relative) data displacement. */
    PCRel32,

    /** 32-bit signed PC-relative branch or call offset. */
    BranchRel32,

    /** 32-bit PC-relative reference to a Global Offset Table (GOT) entry. */
    GOTPCREL,

    /** 32-bit PC-relative reference to a Procedure Linkage Table (PLT) entry. */
    PLTRel32
};

/**
 * Structure used to contain the bare minimum information about an emitted label.
 */
struct CodeLabel
{
    CodeSection *m_definingSection{ nullptr };
    SectionNode *m_node{ nullptr }; // Linked node representation inside CodeSection
    MirId m_id{ MIRID_INVALID };
    uint64_t m_currentOffset{ 0 }; // Current offset from the start of the code
    uint64_t m_labelAddress{ 0 };  // Offset from the start of the section
    std::string_view m_name{};

    uint64_t getAddress() const
    {
        return m_node ? m_node->m_calculatedOffset : m_labelAddress;
    }
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
 * during the code emission phase across modules.
 */
class CodeEmitterContext
{
  public:
    /**
     * Constructs a code emission context with the specified diagnostic collector, section map and allocator.
     */
    CodeEmitterContext(DiagnosticCollector *diagCollector,
                       const std::pmr::unordered_map<SectionType, CodeSection *> &sections,
                       std::pmr::memory_resource *alloc);

    /**
     * Frees ALL the resources allocated in this context (relocations and code labels).
     */
    ~CodeEmitterContext();

    /**
     * Looks up an existing label for the currently bound function by its MIR ID.
     * If the label does not exist, allocates a new CodeLabel via the context allocator,
     * registers it into the current function's label map, and returns a pointer to it.
     */
    CodeLabel *getOrCreateLabel(CodeSection *definingSection, MirId id, const std::string_view &name);

    /**
     * Returns the current code label bound to the context.
     */
    CodeLabel *getCurrentLabel() const;

    /**
     * Creates a relocation defined in the CURRENT LABEL'S SECTION at the CURRENT ADDRESS.
     */
    CodeRelocation *addReloc(MirReference *srcRef, TargetCodeRelocationType relocType);

    /**
     * Returns the current section of the current bound label in the context.
     */
    CodeSection *getCurrentSection() const;

    /**
     * Returns the section with the given type. This function ALWAYS RETURNS NON-NULLPTR.
     */
    CodeSection *getSection(SectionType type) const;

    /**
     * Switches the current bound label to the one given and updates the section node cursor.
     */
    void bindLabel(CodeLabel *label);

    /**
     * Flushes the current function's local labels and relocations into the module-level persistent tables
     * associated with the provided function, and clears function-local scratch maps for the next function emission.
     */
    void resetFuncState(MirFunction *currentFunc);

    /**
     * Looks up a label across the module by its MirId.
     */
    CodeLabel *findLabel(MirId id) const;

    /**
     * Returns module-wide relocations grouped by section.
     */
    const std::pmr::unordered_map<CodeSection *, std::pmr::vector<CodeRelocation *>> &getRelocations() const;

    /**
     * Returns the active function's relocations.
     */
    const std::pmr::vector<CodeRelocation *> &getCurrentFuncRelocs() const;

  private:
    CodeLabel *m_currentLabel{ nullptr };
    DiagnosticCollector *m_diagCollector{ nullptr };
    std::pmr::memory_resource *m_alloc{ nullptr };

    // Fast-lookup scratch data types active for the currently emitting function.
    std::pmr::unordered_map<MirId, CodeLabel *> m_currentFuncLabels;
    std::pmr::vector<CodeRelocation *> m_currentFuncRelocs;

    // Persistent storage of function labels indexed by MirFunction pointer.
    std::pmr::unordered_map<MirFunction *, std::pmr::unordered_map<MirId, CodeLabel *>> m_labels;

    // Persistent module-wide relocation table aggregated across all emitted code sections.
    std::pmr::unordered_map<CodeSection *, std::pmr::vector<CodeRelocation *>> m_relocations;

    // Global tracking of all allocated labels and relocations for safe destruction and lookup
    std::pmr::vector<CodeLabel *> m_allocatedLabels;
    std::pmr::vector<CodeRelocation *> m_allocatedRelocs;
    std::pmr::unordered_map<MirId, CodeLabel *> m_allLabels;

    const std::pmr::unordered_map<SectionType, CodeSection *> &m_sections;
};

#endif // EZPACKER_CODEEMITERCONTEXT_H