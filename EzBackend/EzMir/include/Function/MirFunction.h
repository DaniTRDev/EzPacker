#ifndef EZPACKER_MIRFUNCTION_H
#define EZPACKER_MIRFUNCTION_H

#include "EzMirCommon.h"
#include "Block/MirBlock.h"
#include "Type/MirType.h"
#include "MirFunctionStackFrame.h"
#include "CallingConvDesc.h"

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
     *
     * @param callingConv
     * @param entryPoint    First block executed when the function starts.
     * @param stackFrame    A stack frame object that describes this function's stack frame.
     * @param returnType    MIR type describing the function's return value.
     * @param type
     * @param id            Unique MIR ID for the function itself.
     * @param sourceRef     Source reference that originated this function.
     * @param blocks        Ordered block slice belonging to the function.
     * @param parameters    Slice of parameter operands in declaration order.
     * @param name
     */
    MirFunction(CallingConvDesc *callingConv,
                MirBlock *entryPoint,
                MirFunctionStackFrame *stackFrame,
                MirType *returnType,
                MirType *type,
                MirId id,
                SourceReference *sourceRef,
                std::pmr::list<MirBlock *> blocks,
                std::pmr::list<MirRegister *> parameters,
                std::pmr::string name);

    /**
     * Returns the calling convention of this function.
     */
    CallingConvDesc *getCallingConv() const;

    /**
     * Returns the MirBlock owned by this function that matches the given ID, if no case is found nullptr is returned.
     * @param id
     * @return
     */
    MirBlock *getBlock(size_t id) const;

    /**
     * Returns the function entry block.
     */
    MirBlock *getEntryPoint() const;

    /**
     * Returns the analysis data of this function.
     */
    MirFunctionAnalysisData *getAnalysisData();

    /**
     * Returns the stack frame linked to this object.
     * @return
     */
    MirFunctionStackFrame *getStackFrame() const;

    /**
     * Returns the return type of the function.
     * @return
     */
    MirType *getReturnType() const;

    /**
     * Returns the type of this function.
     */
    MirType *getType() const;

    /**
     * Returns the unique MIR ID assigned to this function.
     */
    MirId getId() const;

    /**
     * Returns the source reference that created this function.
     * @return
     */
    SourceReference *getSourceRef() const;

    /**
     * Adds a callee-saved register that this function is using. If the register is already present, it will be
     * dupplicated.
     */
    void addCalleeSavedRegUse(const RegisterRef &reg);

    /**
     * Returns the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirBuilderContext::createFunction()` succeeds.
     */
    std::pmr::list<MirBlock *> &getBlocks();

    /**
     * Returns a pointer to the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirBuilderContext::createFunction()` succeeds.
     */
    std::pmr::list<MirBlock *> *getBlocksPtr();

    /**
     * Returns the MUTABLE parameter list for this function.
     *
     * Each element is a `MirFuncParam*` describing one incoming parameter. The
     * exact calling-convention meaning is defined by later lowering stages.
     */
    std::pmr::list<MirRegister *> &getParameters();

    /**
     * Returns the list of callee-saved register consumed by this function.
     */
    const std::pmr::vector<RegisterRef> &getUsedCalleeSavedRegs() const;

    /**
     * Returns the name of the function.
     * @return
     */
    const std::pmr::string &getName();

  private:
    CallingConvDesc *m_callingConv;
    MirBlock *m_entryPoint;
    MirFunctionAnalysisData m_analysisData;
    MirFunctionStackFrame *m_stackFrame;
    MirType *m_returnType;
    MirType *m_type;
    MirId m_id;
    SourceReference *m_sourceRef;

    std::pmr::list<MirBlock *> m_blocks;
    std::pmr::list<MirRegister *> m_parameters;
    std::pmr::map<MirId, MirBlock *> m_blockIdToBlock;

    std::pmr::string m_name;

    // Set filled by RegisterAllocatorPass that contains which callee-saved registers were consume by this function.
    std::pmr::vector<RegisterRef> m_usedCalleeSavedRegs;
};

#endif // EZPACKER_MIRFUNCTION_H
