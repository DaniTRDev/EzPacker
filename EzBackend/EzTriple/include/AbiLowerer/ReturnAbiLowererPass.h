#ifndef EZPACKER_RETURNABILOWERERPASS_H
#define EZPACKER_RETURNABILOWERERPASS_H

#include "EzTripleCommon.h"

/**
 * A return block is formed by a set of PUSH_RET and a final RET instructions. These instructions are all bound to a
 * binding token. This design allows for non-ordered return pushes.
 */
struct UnloweredReturnBlock
{
    bool m_hasRet{ false };                               // Is this block ended with a RET instruction?
    MirBlock *m_targetBlock{ nullptr };                   // In which block was the final RET instruction.
    std::pmr::list<MirInstruction *>::iterator m_retIt{}; // Iterator pointing to the instruction to the RET.
    std::pmr::vector<MirInstruction *> m_pushRetInstrs; // List used to store all the PUSH_RET + RET of a return block.

    explicit UnloweredReturnBlock(std::pmr::polymorphic_allocator<std::byte> alloc) : m_pushRetInstrs(alloc) {}
};

/**
 * This pass lowers iterates over the blocks of a function, searched for RET blocks (PUSH_RET/RET with the same binding
 * token) and aggrupates them. Then it will lower each block into a physical destination provided by the
 * CallingConvention (dictated by ABI).
 */
class ReturnAbiLowerer : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    ReturnAbiLowerer(MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "ReturnAbiLowererPass".
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
    /**
     * Process the given block of PUSH_RET instructions and modifies it to follow CallingConvention's guidelines.
     * @param cc
     * @param targetBlock
     * @param func
     * @param retType
     * @param it
     * @param retBlock
     */
    bool processReturnBlock(CallingConvDesc *cc,
                            MirBlock *targetBlock,
                            MirFunction *func,
                            MirType *retType,
                            std::pmr::list<MirInstruction *>::iterator it,
                            std::pmr::vector<MirInstruction *> &retBlock);

  private:
    MirBuilderContext *m_ctx;
    std::pmr::map<MirId, UnloweredReturnBlock> m_unloweredReturns;
};

#endif // EZPACKER_RETURNABILOWERERPASS_H
