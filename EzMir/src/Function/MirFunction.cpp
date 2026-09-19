#include "Block/MirBlock.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirOperands.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"

/**
 * Constructs a function, binding its calling convention, stack frame, types and all PMR-backed
 * containers to the supplied arena. The entry point is left unset until a block is added.
 */
MirFunction::MirFunction(CallingConvDesc *callingConv,
                         MirFunctionStackFrame *stackFrame,
                         MirType *returnType,
                         MirType *type,
                         MirId id,
                         SourceReference *sourceRef,
                         std::pmr::string name,
                         std::pmr::memory_resource *alloc) :
    m_callingConv(callingConv), m_entryPoint(nullptr), m_regInfo(alloc), m_stackFrame(stackFrame),
    m_returnType(returnType), m_type(type), m_id(id), m_sourceRef(sourceRef), m_parameters(alloc),
    m_blockIdToBlock(alloc), m_name(std::move(name)), m_usedCalleeSavedRegs(alloc)
{
}

/**
 * Returns the calling convention that governs parameter passing and register preservation.
 */
CallingConvDesc *MirFunction::getCallingConv() const { return m_callingConv; }

/**
 * Returns the function's basic blocks as a const intrusive list.
 */
const IntrusiveLinkedList<class MirBlock> &MirFunction::getBlocks() const { return m_blocks; }

/**
 * Returns a const iterator to the first basic block.
 */
IntrusiveLinkedList<class MirBlock>::const_iterator MirFunction::begin() const { return m_blocks.begin(); }

/**
 * Returns a const iterator past the last basic block.
 */
IntrusiveLinkedList<class MirBlock>::const_iterator MirFunction::end() const { return m_blocks.end(); }

/**
 * Looks up a basic block by its MIR ID, returning nullptr when the block is not part of this function.
 */
MirBlock *MirFunction::getBlock(MirId id) const
{
    auto it = m_blockIdToBlock.find(id);
    if (it != m_blockIdToBlock.end())
        return it->second;

    return nullptr;
}

/**
 * Returns the designated entry block, which may be null before construction completes.
 */
MirBlock *MirFunction::getEntryPoint() const { return m_entryPoint; }

/**
 * Returns the previous function in the owning intrusive list.
 */
MirFunction *MirFunction::getPrev() const { return m_prev; }

/**
 * Returns the next function in the owning intrusive list.
 */
MirFunction *MirFunction::getNext() const { return m_next; }

/**
 * Returns mutable per-function analysis storage used by analysis passes.
 */
MirFunctionAnalysisData *MirFunction::getAnalysisData() { return &m_analysisData; }

/**
 * Returns the register usage/definition information gathered for this function.
 */
MirFunctionRegisterInfo *MirFunction::getRegisterInfo() { return &m_regInfo; }

/**
 * Returns the stack frame describing this function's stack-allocated objects.
 */
MirFunctionStackFrame *MirFunction::getStackFrame() const { return m_stackFrame; }

/**
 * Returns the unique MIR identifier of this function.
 */
MirId MirFunction::getId() const { return m_id; }

/**
 * Returns the declared return type.
 */
MirType *MirFunction::getReturnType() const { return m_returnType; }

/**
 * Returns the function type describing the full signature.
 */
MirType *MirFunction::getType() const { return m_type; }

/**
 * Returns the number of basic blocks currently owned by this function.
 */
size_t MirFunction::getBlockCount() const { return m_blocks.size(); }

/**
 * Returns the number of formal parameters.
 */
size_t MirFunction::getParamCount() const { return m_parameters.size(); }

/**
 * Returns the source location where the function was defined.
 */
SourceReference *MirFunction::getSourceRef() const { return m_sourceRef; }

/**
 * Returns the ordered list of formal parameter registers.
 */
const std::pmr::list<MirRegister *> &MirFunction::getParameters() const { return m_parameters; }

/**
 * Returns the function's symbol name.
 */
const std::pmr::string &MirFunction::getName() const { return m_name; }

/**
 * Returns the callee-saved registers referenced by this function, as collected during lowering.
 */
const std::pmr::vector<MirRegisterRef> &MirFunction::getUsedCalleeSavedRegs() const { return m_usedCalleeSavedRegs; }

/**
 * Adds a block to both the ID map and the intrusive list. Rejects blocks whose ID is already
 * present, returning false in that case.
 */
bool MirFunction::appendBlock(MirBlock *block)
{
    if (m_blockIdToBlock.contains(block->getId()))
        return false;

    m_blockIdToBlock.insert({ block->getId(), block });
    m_blocks.push_back(block);

    return true;
}

/**
 * Records that the given callee-saved register is used by this function.
 */
void MirFunction::addCalleeSavedRegUse(const MirRegisterRef &reg) { m_usedCalleeSavedRegs.push_back(reg); }

/**
 * Sets the block that control enters first when the function is called.
 */
void MirFunction::setEntryPoint(MirBlock *entryPoint) { m_entryPoint = entryPoint; }

/**
 * Links the next function in the intrusive list.
 */
void MirFunction::setNext(MirFunction *next) { m_next = next; }

/**
 * Links the previous function in the intrusive list.
 */
void MirFunction::setPrev(MirFunction *prev) { m_prev = prev; }
