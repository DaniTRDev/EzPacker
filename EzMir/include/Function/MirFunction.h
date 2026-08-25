#ifndef EZMIR_MIR_FUNCTION_H
#define EZMIR_MIR_FUNCTION_H

#include "EzMirCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"

/**
 * Structure that contains information that is filled by passes as the function flows in the compilation process.
 */
struct MirFunctionAnalysisData
{
    bool m_hasCalls{ false };         // Set by AbiLowererPass.
    bool m_hasDynamicAllocs{ false }; // Set by RegisterAllocator OR FrameLowererPass.

    // Total size (in bytes) occupied by pushed callee-saved registers.
    size_t m_calleeSavedAreaSize = 0; // Set by FrameLowererPass.

    // Total aligned stack payload size allocated during the prologue.
    size_t m_totalFrameSize = 0; // Set by FrameLowererPass.
};

/**
 * Important: Parameters MUST BE VIRTUAL/PHYSICAL REGISTERS.
 */
class MirFunction
{
  public:
    /**
     * Creates a function wrapper over arena-managed MIR data structures.
     */
    MirFunction(class CallingConvDesc *callingConv,
                class MirBlock *entryPoint,
                class MirFunctionStackFrame *stackFrame,
                class MirType *returnType,
                class MirType *type,
                MirId id,
                class SourceReference *sourceRef,
                std::pmr::list<class MirRegister *> parameters,
                std::pmr::string name,
                std::pmr::memory_resource *alloc);

    /**
     * Appends the given block into the function. If it's already present, false is returned.
     */
    bool appendBlock(MirBlock *block);

    /**
     * Returns the calling convention of this function.
     */
    class CallingConvDesc *getCallingConv() const;

    /**
     * Returns the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirBuilderContext::createFunction()` succeeds.
     */
    IntrusiveLinkedList<class MirBlock> &getBlocks();

    /**
     * Returns a pointer to the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirBuilderContext::createFunction()` succeeds.
     */
    IntrusiveLinkedList<class MirBlock> *getBlocksPtr();

    /**
     * Returns an iterator pointing to the first element of the list.
     */
    IntrusiveLinkedList<class MirBlock>::iterator begin();

    /**
     * Returns an iterator pointing to the last element (INVALID) of the list.
     */
    IntrusiveLinkedList<class MirBlock>::iterator end();

    /**
     * Returns the class MirBlock owned by this function that matches the given ID, if no case is found nullptr is
     * returned.
     */
    class MirBlock *getBlock(size_t id) const;

    /**
     * Returns the function entry block.
     */
    class MirBlock *getEntryPoint() const;

    /**
     * Returns the previous function.
     */
    MirFunction *getPrev() const;

    /**
     * Returns the next function.
     */
    MirFunction *getNext() const;

    /**
     * Returns the analysis data of this function.
     */
    MirFunctionAnalysisData *getAnalysisData();

    /**
     * Returns the stack frame linked to this object.
     */
    class MirFunctionStackFrame *getStackFrame() const;

    /**
     * Returns the return type of the function.
     */
    class MirType *getReturnType() const;

    /**
     * Returns the type of this function.
     */
    class MirType *getType() const;

    /**
     * Returns the unique MIR ID assigned to this function.
     */
    MirId getId() const;

    /**
     * Returns the number of blocks defined in this function.
     */
    size_t getBlockCount() const;

    /**
     * Returns the number of parameters.
     */
    size_t getParamCount() const;

    /**
     * Returns the source reference that created this function.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Adds a callee-saved register that this function is using. If the register is already present, it will be
     * dupplicated.
     */
    void addCalleeSavedRegUse(const class MirRegisterRef &reg);

    /**
     * Sets the entry point of the function. This WON'T push the block to the list, it is up to the caller to push the
     * block in the FIRST position.
     */
    void setEntryPoint(MirBlock *entryPoint);

    /**
     * Sets the next function.
     */
    void setNext(MirFunction *next);

    /**
     * Sets the previous function.
     */
    void setPrev(MirFunction *prev);

    /**
     * Returns the MUTABLE parameter list for this function.
     *
     * Each element is a `MirRegister*` describing one incoming parameter. The
     * exact calling-convention meaning is defined by later lowering stages.
     */
    std::pmr::list<class MirRegister *> &getParameters();

    /**
     * Returns the list of callee-saved register consumed by this function. This information is available after
     * MirRegisterAllocatorPass.
     */
    const std::pmr::vector<class MirRegisterRef> &getUsedCalleeSavedRegs() const;

    /**
     * Returns the name of the function.
     */
    const std::pmr::string &getName();

  private:
    class CallingConvDesc *m_callingConv;
    class MirBlock *m_entryPoint;
    MirFunction *m_next{ nullptr };
    MirFunction *m_prev{ nullptr };

    MirFunctionAnalysisData m_analysisData;
    class MirFunctionStackFrame *m_stackFrame;
    class MirType *m_returnType;
    class MirType *m_type;
    MirId m_id;
    class SourceReference *m_sourceRef;

    IntrusiveLinkedList<class MirBlock> m_blocks;
    std::pmr::list<class MirRegister *> m_parameters;
    std::pmr::map<MirId, class MirBlock *> m_blockIdToBlock;

    std::pmr::string m_name;

    // Set filled by MirRegisterAllocatorPass that contains which callee-saved registers were consume by this function.
    std::pmr::vector<class MirRegisterRef> m_usedCalleeSavedRegs;
};

#endif // EZMIR_MIR_FUNCTION_H
