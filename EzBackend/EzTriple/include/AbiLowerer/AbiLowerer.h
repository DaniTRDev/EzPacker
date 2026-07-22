#ifndef EZPACKER_ABILOWERER_h
#define EZPACKER_ABILOWERER_h

#include "EzTripleCommon.h"

class AbiLowerer
{
  public:
    /**
     * Creates the lowerer with the given context.
     */
    AbiLowerer(MirBuilderContext *ctx);

    /**
     * Process the given block of PUSH_RET+RET instructions and modifies it to follow CallingConvention's guidelines.
     * @param cc
     * @param targetBlock
     * @param func
     * @param retType
     * @param it
     * @param pushRets
     */
    bool processReturnBlock(CallingConvDesc *cc,
                            MirBlock *targetBlock,
                            MirFunction *func,
                            MirType *retType,
                            std::pmr::list<MirInstruction *>::iterator it,
                            std::pmr::vector<MirInstruction *> &pushRets);

    /**
     * Process the given block of PUSH_ARG+CALL instructions and modifies it to follow CallingConvention's guidelines.
     * @param cc
     * @param targetBlock
     * @param func
     * @param it
     * @param pushArgs
     */
    bool processCallBlock(CallingConvDesc *cc,
                          MirBlock *targetBlock,
                          MirFunction *func,
                          std::pmr::list<MirInstruction *>::iterator it,
                          std::pmr::vector<MirInstruction *> &pushArgs);

    /**
     * Process the given block of PUSH_ARG+CALL instructions and modifies it to follow CallingConvention's guidelines.
     * @param cc
     * @param targetBlock
     * @param func
     * @param popArgs
     */
    bool processFunctionArguments(CallingConvDesc *cc,
                                  MirBlock *targetBlock,
                                  MirFunction *func,
                                  std::pmr::list<MirInstruction *>::iterator it,
                                  std::pmr::vector<MirInstruction *> &popArgs);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_ABILOWERER_h