#ifndef EZPACKER_CODEEMITERCONTEXT_H
#define EZPACKER_CODEEMITERCONTEXT_H

#include "EzCodeEmitterCommon.h"

/**
 * Structure used to contain the bare minimum information about an emitted label.
 */
struct CodeLabel
{
    MirId m_id;
    std::string_view m_name;
};

/**
 * This struct contains information about a relocation.
 */
struct CodeRelocation
{
    MirReference *m_srcRef{ nullptr }; // Reference that caused the relocation to appear.
    uint64_t m_address{ 0 };           // Address or section offset where the relocation fixup applies.
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