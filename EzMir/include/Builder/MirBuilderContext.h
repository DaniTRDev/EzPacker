#ifndef EZMIR_MIR_BUILDER_CONTEXT_H
#define EZMIR_MIR_BUILDER_CONTEXT_H

#include "EzMirCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"

class MirBuilderContext
{
  public:
    /**
     * Builds the context with the given type table.
     */
    MirBuilderContext(class CallingConvDesc *defaultCallingConv,
                      class DiagnosticCollector *diagCollector,
                      class MirTypeTable *typeTable,
                      std::pmr::monotonic_buffer_resource *globalArena);

    // Disable copy/move constructors to preserve safety across the arena resource references
    MirBuilderContext(const MirBuilderContext &) = delete;
    MirBuilderContext &operator=(const MirBuilderContext &) = delete;

    /**
     * Appends the block to the context. Returns true if succeeded.
     */
    bool appendBlock(class MirBlock *block);

    /**
     * Appends the function to the context. Returns true if succeeded.
     */
    bool appendFunction(class MirFunction *func);

    /**
     * Appends a global variable to the context. Returns true if succeeded.
     */
    bool appendGlobalVar(class MirGlobalVar *globalVar);

    /**
     * Appends a register to the context. Returns true if succeeded. This function will skip physical registers.
     */
    bool appendRegister(class MirRegister *reg);

    /**
     * Returns the default calling convention for a function.
     */
    class CallingConvDesc *getDefaultCallingConvention() const;

    /**
     * Returns the diagnostic collector.
     */
    class DiagnosticCollector *getDiagCollector();

    /**
     * Returns the MUTABLE function list that were built in this context. Used internally by the pass manager.
     */
    IntrusiveLinkedList<class MirFunction> &getFunctions();

    /**
     * Searches in the context for the given block ID and returns a pointer to it, if exists. Returns nullptr if the
     * ID is not found.
     */
    class MirBlock *getBlockById(MirId id) const;

    /**
     * Searches in the context for the given function ID and returns a pointer to the function, if exists. Returns
     * nullptr if the ID is not found.
     */
    class MirFunction *getFuncById(MirId id) const;

    /**
     * Searches in the context for a global variable with the given index. If it exists, it is returned. Returns nullptr
     * if not found.
     */
    class MirGlobalVar *getGVarById(MirId id) const;

    /**
     * Creates an ID within this context. Returns 0 (MIRID_INVALID) if failed.
     */
    MirId createId();

    /**
     * Returns the register that has the given ID. If no match is found, nullptr is returned.
     */
    class MirRegister *getRegisterById(size_t id) const;

    /**
     * Returns the type table attached to this context.
     */
    class MirTypeTable *getTypeTable();

    /**
     * Sets the default calling convention.
     */
    void setDefaultCallingConvention(class CallingConvDesc *defaultCallingConv);

    /**
     * Returns an allocator used to allocate complementary resources (global data, types, names, maps...).
     */
    std::pmr::monotonic_buffer_resource *getGlobalAllocator();

    /**
     * Returns the MUTABLE global variable list that were built using this context. Used internally by the pass manager.
     */
    std::pmr::list<class MirGlobalVar *> &getGlobalVars();

  private:
    class CallingConvDesc *m_defaultCallingConv; // Default calling convention used when building functions.
    class DiagnosticCollector *m_diagCollector;
    MirId m_currentId{ 0 };
    class MirTypeTable *m_typeTable;

    // Pools.
    std::pmr::monotonic_buffer_resource *m_globalResource;

    IntrusiveLinkedList<class MirFunction> m_functions; // Used to quickly iterate over defined functions.
    std::pmr::list<class MirGlobalVar *> m_globalVars;  // Used to quickly iterate over defined global variables.

    std::pmr::map<MirId, class MirBlock *> m_blockIdToBlock;          // Used to search for blocks.
    std::pmr::map<MirId, class MirClass *> m_typeIdToClass;           // Used to search for classes using their type.
    std::pmr::map<MirId, class MirFunction *> m_functionIdToFunc;     // Used to search for functions.
    std::pmr::map<MirId, class MirGlobalVar *> m_globalVarIdToGVar;   // Used to search for global variables.
    std::pmr::map<MirId, class MirRegister *> m_registerIdToRegister; // Used to search for registers.
};

#endif // EZMIR_MIR_BUILDER_CONTEXT_H