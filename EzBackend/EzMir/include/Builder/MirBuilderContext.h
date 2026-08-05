/**
 * @file MirBuilderContext.h
 * @brief Central bookkeeping for MIR allocation, IDs, active bindings, and arena pools.
 */
#ifndef EZPACKER_MIRBUILDERCONTEXT_H
#define EZPACKER_MIRBUILDERCONTEXT_H

#include "EzMirCommon.h"
#include "Block/MirBlock.h"
#include "Class/MirClass.h"
#include "Function/MirFunction.h"
#include "GlobalVar/MirGlobalVar.h"
#include "Type/IMirTargetTypeLayout.h"
#include "Type/MirTypeTable.h"

class MirBuilderContext
{
  public:
    /**
     * Builds the context with the given type table.
     * @param defaultCallingConv
     * @param globalArena Used to store general data, names, ...
     * @param funcArena Used to store functions, blocks, instructions, operands, ...
     * @param diagCollector
     * @param typeTable
     */
    MirBuilderContext(CallingConvDesc *defaultCallingConv,
                      std::pmr::monotonic_buffer_resource *globalArena,
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
     * Appends the class to the context. Returns true if succeeded.
     * @param block
     * @return
     */
    bool appendClass(MirClass *_class);

    /**
     * Appends the function to the context. Returns true if succeeded.
     * @param func
     * @return
     */
    bool appendFunction(MirFunction *func);

    /**
     * Appends a global variable to the context. Returns true if succeeded.
     * @param globalVar
     */
    bool appendGlobalVar(MirGlobalVar *globalVar);

    /**
     * Appends a register to the context. Returns true if succeeded. This function will skip physical registers.
     * @param reg
     * @return
     */
    bool appendRegister(MirRegister *reg);

    /**
     * Returns the default calling convention for a function.
     */
    CallingConvDesc *getDefaultCallingConvention() const;

    /**
     * Searches in the context for the given block ID and returns a pointer to it, if exists. Returns nullptr if the
     * ID is not found.
     * @param id
     * @return
     */
    MirBlock *getBlockById(MirId id) const;

    /**
     * Searches in the context for the given class ID and returns a pointer to it, if exists. Returns nullptr if the ID
     * is not found.
     */
    MirClass *getClassById(MirId id) const;

    /**
     * Searches in the context for the given type ID and returns the class linked to it, if exists. Returns nullptr if
     * the ID is not found.
     */
    MirClass *getClassByTypeId(MirId id) const;

    /**
     * Searches in the context for the given function ID and returns a pointer to the function, if exists. Returns
     * nullptr if the ID is not found.
     * @param id
     * @return
     */
    MirFunction *getFuncById(MirId id) const;

    /**
     * Searches in the context for a global variable with the given index. If it exists, it is returned. Returns nullptr
     * if not found.
     * @param id
     */
    MirGlobalVar *getGVarById(MirId id) const;

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
     * Sets the default calling convention.
     */
    void setDefaultCallingConvention(CallingConvDesc *defaultCallingConv);

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
     * Returns the MUTABLE classes list that were built using this context. Used internally by the pass manager.
     */
    std::pmr::list<MirClass *> &getClasses();

    /**
     * Returns the MUTABLE function list that were built in this context. Used internally by the pass manager.
     * @return
     */
    std::pmr::list<MirFunction *> &getFunctions();

    /**
     * Returns the MUTABLE global variable list that were built using this context. Used internally by the pass manager.
     */
    std::pmr::list<MirGlobalVar *> &getGlobalVars();

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
    CallingConvDesc *m_defaultCallingConv; // Default calling convention used when building functions.
    MirId m_currentId{ 0 };

    // Pools.
    std::pmr::monotonic_buffer_resource *m_globalResource;
    std::pmr::monotonic_buffer_resource *m_functionResource;

    std::pmr::list<MirClass *> m_classes;        // Used to quickly iterate over defined classes.
    std::pmr::list<MirFunction *> m_functions;   // Used to quickly iterate over defined functions.
    std::pmr::list<MirGlobalVar *> m_globalVars; // Used to quickly iterate over defined global variables.

    std::pmr::map<MirId, MirBlock *> m_blockIdToBlock;          // Used to search for blocks.
    std::pmr::map<MirId, MirClass *> m_classIdToClass;          // Used to search for classes.
    std::pmr::map<MirId, MirClass *> m_typeIdToClass;           // Used to search for classes using their type.
    std::pmr::map<MirId, MirFunction *> m_functionIdToFunc;     // Used to search for functions.
    std::pmr::map<MirId, MirGlobalVar *> m_globalVarIdToGVar;   // Used to search for global variables.
    std::pmr::map<MirId, MirRegister *> m_registerIdToRegister; // Used to search for registers.
    std::pmr::map<MirId, PhysicalRegId> m_virtualRegIdToPhysical;

    std::shared_ptr<DiagnosticCollector> m_diagCollector;
    std::shared_ptr<MirTypeTable> m_typeTable;
};

#endif // EZPACKER_MIRBUILDERCONTEXT_H