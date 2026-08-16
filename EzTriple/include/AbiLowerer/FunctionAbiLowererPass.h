#ifndef EZPACKER_FUCTIONABILOWERERPASS_H
#define EZPACKER_FUCTIONABILOWERERPASS_H

#include "EzTripleCommon.h"
#include "AbiLowerer.h"

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
    bool m_terminated{ false };         // Is this block ended with a termination instruction? Used for error catching.
    MirBlock *m_targetBlock{ nullptr }; // In which block was the final termination instruction.
    UnloweredBlockType m_type;
    std::pmr::list<MirInstruction *>::iterator m_termIt{}; // Iterator pointing to the instruction to the terminator..
    std::pmr::vector<MirInstruction *> m_pushList{}; // List used to store all the PUSH_RET/PUSH_ARG.
    std::pmr::vector<MirInstruction *> m_popList{}; // List used to store all the POP_RET/POP_ARG.

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
 * It uses a helper called AbiLowerer that implements methods to lower returns/calls.
 */
class FunctionAbiLowererPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    FunctionAbiLowererPass(MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "FunctionAbiLowerer".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Function.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given function and returns the result.
     * @param funcList
     * @param it
     * @param passManager
     * @return
     */
    MirPassResult run(std::pmr::list<MirFunction *> &funcList,
                      std::pmr::list<MirFunction *>::iterator it,
                      MirPassManager *passManager) override;

    /**
     * Prints the resulting block of of the lowered returns.
     */
    void printResult() const override;

  private:
    MirBuilderContext *m_ctx;
    std::pmr::list<LoweredBlock> m_loweredBlocks; // List of blocks in which at least a CALL or RET was lowered.
};

#endif // EZPACKER_FUCTIONABILOWERERPASS_H