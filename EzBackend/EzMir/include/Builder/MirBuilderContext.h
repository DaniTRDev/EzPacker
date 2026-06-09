/**
 * @file MirBuilderContext.h
 * @brief Central bookkeeping for MIR allocation, IDs, active bindings, and arena pools.
 */
#ifndef EZPACKER_MIRBUILDERCONTEXT_H
#define EZPACKER_MIRBUILDERCONTEXT_H

#include "EzMirCommon.h"
#include "Block/MirBlock.h"
#include "Function/MirFunction.h"
#include "Type/MirTypeTable.h"

/**
 * A struct that contains information about global data.
 */
struct MirGlobalDataEntry
{
    bool m_isReadOnly;
    bool m_uninitialized;
    std::pmr::vector<char> m_data;
    MirType *m_dataType;
    size_t m_entryId;
};

class MirBuilderContext
{
  public:
    /**
     * Builds the context with the given type table.
     * @param globalArena Used to store general data, names, ...
     * @param funcArena Used to store functions, blocks, instructions, operands, ...
     * @param diagCollector
     * @param typeTable
     */
    MirBuilderContext(std::pmr::monotonic_buffer_resource *globalArena,
                      const std::shared_ptr<DiagnosticCollector> &diagCollector,
                      const std::shared_ptr<MirTypeTable> &typeTable);

    // Disable copy/move constructors to preserve safety across the arena resource references
    MirBuilderContext(const MirBuilderContext &) = delete;
    MirBuilderContext &operator=(const MirBuilderContext &) = delete;

    /**
     * Appends the block to the context. Returns true if succeeded.
     * @param block
     * @return
     */
    bool appendBlock(MirBlock *block);

    /**
     * Appends the function to the context. Returns true if succeeded.
     * @param func
     * @return
     */
    bool appendFunction(MirFunction *func);

    /**
     * Appends a register to the context. Returns true if succeeded.
     * @param reg
     * @return
     */
    bool appendRegister(MirRegister *reg);

    /**
     * Searches in the context for the given block ID and returns a pointer to it, if exists. Returns nullptr is the
     * ID is not found.
     * @param id
     * @return
     */
    MirBlock *getBlockById(size_t id) const;

    /**
     * Searches in the context for the given function ID and returns a pointer to the function, if exists. Returns
     * nullptr if the ID is not found.
     * @param id
     * @return
     */
    MirFunction *getFuncById(size_t id) const;

    /**
     * Creates an ID within this context. Returns 0 (MIRID_INVALID) if failed.
     * @return
     */
    MirId createId();

    /**
     * Returns the register that has the given ID. If no match is found, nullptr is returned.
     * @param id
     * @return
     */
    MirRegister *getRegisterById(size_t id) const;

    /**
     * Returns the MUTABLE list of functions that have been built in this context.
     * @return
     */
    std::pmr::list<MirFunction *> &getFunctions();

    /**
     * Returns an allocator used to allocate complementary resources (global data, types, names, maps...).
     * @return
     */
    std::pmr::monotonic_buffer_resource *getGlobalAllocator();

    /**
     * Returns an allocator related to functions (blocks mainly).
     * @return
     */
    std::pmr::monotonic_buffer_resource *getFuncAllocator();

    /**
     * Returns the diagnostic collector.
     * @return
     */
    const std::shared_ptr<DiagnosticCollector> &getDiagCollector();

    /**
     * Returns the type table attached to this context.
     * @return
     */
    const std::shared_ptr<MirTypeTable> &getTypeTable();

  private:
    MirId m_currentId{ 0 };

    // Pools.
    std::pmr::monotonic_buffer_resource *m_globalResource;
    std::pmr::monotonic_buffer_resource *m_functionResource;

    std::pmr::list<MirFunction *> m_functions;
    std::pmr::map<size_t, MirFunction *> m_functionIdToFunc; // Used to search for functions.
    std::pmr::map<size_t, MirBlock *> m_blockIdToBlock;      // Used to search for blocks.
    std::pmr::map<size_t, MirRegister *> m_registerIdToRegister;

    std::pmr::vector<MirGlobalDataEntry *> m_globalData;

    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<MirTypeTable> m_typeTable;
};

#endif // EZPACKER_MIRBUILDERCONTEXT_H