#ifndef EZMIR_MIR_BUILDER_CONTEXT_H
#define EZMIR_MIR_BUILDER_CONTEXT_H

#include "EzMirCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"

/**
 * Central state and memory manager for MIR module construction.
 * Manages monotonic PMR arena resources, symbol and type lookup tables, ID generation,
 * and maintains collections of functions, global variables, basic blocks, and virtual registers.
 */
class MirBuilderContext
{
  public:
    /**
     * Constructs a MIR builder context with default calling convention, diagnostic collector, type table, and arena.
     */
    MirBuilderContext(class CallingConvDesc *defaultCallingConv,
                      class DiagnosticCollector *diagCollector,
                      class MirTypeTable *typeTable,
                      std::pmr::monotonic_buffer_resource *globalArena);

    // Disable copy/move constructors to preserve safety across the arena resource references
    MirBuilderContext(const MirBuilderContext &) = delete;
    MirBuilderContext &operator=(const MirBuilderContext &) = delete;

    /**
     * Appends and registers a basic block into the context's lookup map. Returns false if already registered.
     */
    bool appendBlock(class MirBlock *block);

    /**
     * Appends and registers a function into the context's intrusive function list and lookup map.
     */
    bool appendFunction(class MirFunction *func);

    /**
     * Appends and registers a global variable into the context's global variable list.
     */
    bool appendGlobalVar(class MirGlobalVar *globalVar);

    /**
     * Registers a virtual register into the context's ID lookup table. Skips physical registers.
     */
    bool appendRegister(class MirRegister *reg);

    /**
     * Returns the default calling convention descriptor.
     */
    class CallingConvDesc *getDefaultCallingConvention() const;

    /**
     * Returns the diagnostic collector.
     */
    class DiagnosticCollector *getDiagCollector();

    /**
     * Returns the mutable intrusive list of functions built within this context.
     */
    IntrusiveLinkedList<class MirFunction> &getFunctions();

    /**
     * Retrieves a basic block by its unique MirId. Returns nullptr if not found.
     */
    class MirBlock *getBlockById(MirId id) const;

    /**
     * Retrieves a function by its unique MirId. Returns nullptr if not found.
     */
    class MirFunction *getFuncById(MirId id) const;

    /**
     * Retrieves a global variable by its unique MirId. Returns nullptr if not found.
     */
    class MirGlobalVar *getGVarById(MirId id) const;

    /**
     * Allocates and returns a new monotonically increasing MirId.
     */
    MirId createId();

    /**
     * Retrieves a virtual/physical register by its numeric ID. Returns nullptr if not found.
     */
    class MirRegister *getRegisterById(size_t id) const;

    /**
     * Returns the MirTypeTable associated with this context.
     */
    class MirTypeTable *getTypeTable();

    /**
     * Sets the default target calling convention descriptor for functions built in this context.
     */
    void setDefaultCallingConvention(class CallingConvDesc *defaultCallingConv);

    /**
     * Returns the monotonic buffer memory resource for arena allocations.
     */
    std::pmr::monotonic_buffer_resource *getGlobalAllocator();

    /**
     * Returns the mutable list of global variables registered in this context.
     */
    std::pmr::list<class MirGlobalVar *> &getGlobalVars();

  private:
    class CallingConvDesc *m_defaultCallingConv; // Default calling convention used when building functions.
    class DiagnosticCollector *m_diagCollector;  // Sink for errors/traces raised while building MIR.
    MirId m_currentId{ 0 };                      // Next ID to hand out; advanced by createId().
    class MirTypeTable *m_typeTable;             // Deduplicating table used to create/query MIR types.

    // Pools.
    std::pmr::monotonic_buffer_resource *m_globalResource;

    IntrusiveLinkedList<class MirFunction> m_functions; // Used to quickly iterate over defined functions.
    std::pmr::list<class MirGlobalVar *> m_globalVars;  // Used to quickly iterate over defined global variables.

    std::pmr::map<MirId, class MirBlock *> m_blockIdToBlock;          // Used to search for blocks.
    std::pmr::map<MirId, class MirFunction *> m_functionIdToFunc;     // Used to search for functions.
    std::pmr::map<MirId, class MirGlobalVar *> m_globalVarIdToGVar;   // Used to search for global variables.
    std::pmr::map<MirId, class MirRegister *> m_registerIdToRegister; // Used to search for registers.
};

#endif // EZMIR_MIR_BUILDER_CONTEXT_H