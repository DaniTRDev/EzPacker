#ifndef EZMIR_MIR_FUNCTION_H
#define EZMIR_MIR_FUNCTION_H

#include "EzMirCommon.h"
#include "MirFunctionRegisterInfo.h"
#include "HelperClasses/IntrusiveLinkedList.h"

/**
 * Compiler pass analysis metrics collected during lowering and optimization pipelines.
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
 * Mid-level intermediate representation for a function definition.
 * Owns the CFG of MirBlock nodes (intrusively linked), incoming virtual register parameters,
 * stack frame layout (MirFunctionStackFrame), and target calling convention descriptor.
 */
class MirFunction
{
  public:
    friend class IntrusiveLinkedList<MirFunction>;
    friend class MirBlockBuilder;
    friend class MirFunctionBuilder;

    /**
     * Constructs a MIR function instance with arena memory resource and initializes its CFG entry block.
     */
    MirFunction(class CallingConvDesc *callingConv,
                class MirFunctionStackFrame *stackFrame,
                class MirType *returnType,
                class MirType *type,
                MirId id,
                class SourceReference *sourceRef,
                std::pmr::string name,
                std::pmr::memory_resource *alloc,
                MirLinkage linkage = MirLinkage::External);

    /**
     * Returns the symbol linkage and visibility specification for this function.
     */
    MirLinkage getLinkage() const;

    /**
     * Sets the symbol linkage and visibility specification for this function.
     */
    void setLinkage(MirLinkage linkage);

    /**
     * Checks if this function is an external declaration without a body (has 0 basic blocks).
     */
    bool isDeclaration() const;

    /**
     * Checks if this function is a definition with a body (has 1 or more basic blocks).
     */
    bool isDefinition() const;

    /**
     * Returns the target calling convention descriptor for this function.
     */
    class CallingConvDesc *getCallingConv() const;

    /**
     * Returns an inmutable reference to the intrusive linked list of basic blocks comprising this function.
     */
    const IntrusiveLinkedList<class MirBlock> &getBlocks() const;

    /**
     * Returns an iterator to the entry block of the function.
     */
    IntrusiveLinkedList<class MirBlock>::const_iterator begin() const;

    /**
     * Returns an end iterator past the last block of the function.
     */
    IntrusiveLinkedList<class MirBlock>::const_iterator end() const;

    /**
     * Retrieves a basic block by its numeric MirId. Returns nullptr if not found in this function.
     */
    class MirBlock *getBlock(size_t id) const;

    /**
     * Returns the designated CFG entry basic block.
     */
    class MirBlock *getEntryPoint() const;

    /**
     * Returns the preceding function in the module function list.
     */
    MirFunction *getPrev() const;

    /**
     * Returns the subsequent function in the module function list.
     */
    MirFunction *getNext() const;

    /**
     * Returns a pointer to mutable pass analysis metadata.
     */
    MirFunctionAnalysisData *getAnalysisData();

    /**
     * Returns the register info holder for this function.
     */
    MirFunctionRegisterInfo *getRegisterInfo();

    /**
     * Returns the stack frame layout descriptor for this function.
     */
    class MirFunctionStackFrame *getStackFrame() const;

    /**
     * Returns the return MirType of this function.
     */
    class MirType *getReturnType() const;

    /**
     * Returns the composite function signature MirType.
     */
    class MirType *getType() const;

    /**
     * Returns the unique MIR ID assigned to this function.
     */
    MirId getId() const;

    /**
     * Returns the total number of basic blocks in this function.
     */
    size_t getBlockCount() const;

    /**
     * Returns the number of formal parameters accepted by this function.
     */
    size_t getParamCount() const;

    /**
     * Returns the SourceReference span representing the function definition in source code.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Returns the inmutable list of incoming virtual/physical parameter registers.
     */
    const std::pmr::list<class MirRegister *> &getParameters() const;

    /**
     * Returns the collection of callee-saved registers modified in this function body.
     */
    const std::pmr::vector<class MirRegisterRef> &getUsedCalleeSavedRegs() const;

    /**
     * Returns the symbol name of this function.
     */
    const std::pmr::string &getName() const;

  private:
    /**
     * Appends a basic block to the end of the function's block chain. Returns false if already inserted.
     */
    bool appendBlock(MirBlock *block);

    /**
     * Updates the entry basic block pointer without inserting it into the block list.
     */
    void setEntryPoint(MirBlock *entryPoint);

    /**
     * Links the subsequent function in the intrusive module chain.
     */
    void setNext(MirFunction *next);

    /**
     * Links the preceding function in the intrusive module chain.
     */
    void setPrev(MirFunction *prev);

  private:
    class CallingConvDesc *m_callingConv;      // ABI rules for this function.
    class MirBlock *m_entryPoint;              // First block executed on entry, or nullptr before building.
    MirFunction *m_next{ nullptr };            // Next function in the owning intrusive module list.
    MirFunction *m_prev{ nullptr };            // Previous function in the owning intrusive module list.
    MirFunctionAnalysisData m_analysisData;    // Pass-produced metrics (calls, frame sizes, etc.).
    MirFunctionRegisterInfo m_regInfo;         // SSA def/use tracking for this function's registers.
    class MirFunctionStackFrame *m_stackFrame; // Stack frame layout owned by this function.
    class MirType *m_returnType;               // Declared return type.
    class MirType *m_type;                     // Composite signature type of the function.
    MirId m_id;                                // Unique MIR identifier.
    class SourceReference *m_sourceRef;        // Source location of the definition.

    IntrusiveLinkedList<class MirBlock> m_blocks;            // CFG blocks in layout order.
    std::pmr::list<class MirRegister *> m_parameters;        // Incoming parameter registers, in declaration order.
    std::pmr::map<MirId, class MirBlock *> m_blockIdToBlock; // Block lookup by ID.

    std::pmr::string m_name; // Function symbol name.
    MirLinkage m_linkage{ MirLinkage::External }; // Symbol linkage visibility.

    // Set filled by MirRegisterAllocatorPass that contains which callee-saved registers were consume by this function.
    std::pmr::vector<class MirRegisterRef> m_usedCalleeSavedRegs;
};

#endif // EZMIR_MIR_FUNCTION_H
