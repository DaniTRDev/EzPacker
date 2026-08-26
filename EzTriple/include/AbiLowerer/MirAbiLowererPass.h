#ifndef EZTRIPLE_FUCTION_ABI_LOWERER_PASS_H
#define EZTRIPLE_FUCTION_ABI_LOWERER_PASS_H

#include "EzTripleCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"
#include "MirPasses/IMirTransformPass.h"

/**
 * Tag classifying unlowered ABI calling sequences (Return, Call, or Function incoming arguments).
 */
enum class UnloweredBlockType
{
    Return = 0,
    Call,
    FunctionArgs
};

/**
 * A return block is formed by a set of PUSH_RET and a final RET instructions. These instructions are all bound to a
 * binding token. This design allows for non-ordered return pushes.
 *
 * A call block is formed by a set of PUSH_ARG and a final CALL instruction. These instructions are all bound to a
 * binding token. This design allows non-ordered argument pushes.
 *
 * A function signature block is a set of POP_ARG and a final END_ARG instructions. These instructions are all bound to
 * a binding token. This design allows non-ordered argument pops.
 */
struct UnloweredBlock
{
    bool m_terminated{ false }; // Is this block ended with a termination instruction? Used for error catching.
    class MirBlock *m_targetBlock{ nullptr }; // In which block was the final termination instruction.
    UnloweredBlockType m_type;
    IntrusiveLinkedList<class MirInstruction>::iterator m_termIt{}; // Iterator pointing to the terminator instruction.
    std::pmr::vector<class MirInstruction *> m_pushList{};       // List used to store all the PUSH_RET/PUSH_ARG.
    std::pmr::vector<class MirInstruction *> m_popList{};        // List used to store all the POP_RET/POP_ARG.

    explicit UnloweredBlock(UnloweredBlockType type, std::pmr::polymorphic_allocator<std::byte> alloc) :
        m_type(type), m_pushList(alloc), m_popList(alloc)
    {
    }
};

// Same fields internally, just made this alias not to confuse.
using LoweredBlock = UnloweredBlock;

/**
 * This pass lowers iterates over the blocks of a function, searches for RET blocks (PUSH_RET/RET + PUSH_ARG/CALL with
 * the same binding token) and aggrupates them. Then it will lower each block into a physical destination provided by
 * the CallingConvention (dictated by ABI).
 *
 * It uses a helper called MirAbiLowerer that implements methods to lower returns/calls.
 */
class MirAbiLowererPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    MirAbiLowererPass(class MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "FunctionAbiLowerer".
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given function and returns the result.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction> &funcList,
                      IntrusiveLinkedList<class MirFunction>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Prints the resulting block of of the lowered returns.
     */
    void printResult() override;

  private:
    class MirBuilderContext *m_ctx;
    std::pmr::list<LoweredBlock> m_loweredBlocks; // List of blocks in which at least a CALL or RET was lowered.
};

#endif // EZTRIPLE_FUCTION_ABI_LOWERER_PASS_H