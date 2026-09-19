#ifndef EZTRIPLE_MIR_ABI_LOWERER_H
#define EZTRIPLE_MIR_ABI_LOWERER_H

#include "EzTripleCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"

class MirInstruction;

/**
 * Lowers token-bound ABI sequences (PUSH_RET/RET, PUSH_ARG/CALL, POP_ARG/END_ARG) into the concrete
 * register/stack moves mandated by a target calling convention.
 */
class MirAbiLowerer
{
  public:
    /**
     * Creates the lowerer with the given context.
     */
    MirAbiLowerer(class MirBuilderContext *ctx);

    /**
     * Process the given block of PUSH_RET+RET instructions and modifies it to follow CallingConvention's guidelines.
     */
    bool processReturnBlock(class CallingConvDesc *cc,
                            class MirBlock *targetBlock,
                            class MirFunction *func,
                            class MirType *retType,
                            IntrusiveLinkedList<MirInstruction>::iterator it,
                            std::pmr::vector<class MirInstruction *> &pushRets);

    /**
     * Process the given block of PUSH_ARG+CALL instructions and modifies it to follow CallingConvention's guidelines.
     */
    bool processCallBlock(class CallingConvDesc *cc,
                          class MirBlock *targetBlock,
                          class MirFunction *func,
                          IntrusiveLinkedList<MirInstruction>::iterator it,
                          std::pmr::vector<class MirInstruction *> &pushArgs);

    /**
     * Process the given block of POP_RET instructions and modifies it to follow CallingConvention's guidelines.
     */
    bool processCallReturnBlock(class CallingConvDesc *cc,
                                class MirBlock *targetBlock,
                                class MirFunction *func,
                                IntrusiveLinkedList<MirInstruction>::iterator it,
                                std::pmr::vector<class MirInstruction *> &popRet);
    /**
     * Process the given block of PUSH_ARG+CALL instructions and modifies it to follow CallingConvention's guidelines.
     */
    bool processFunctionArguments(class CallingConvDesc *cc,
                                  class MirBlock *targetBlock,
                                  class MirFunction *func,
                                  IntrusiveLinkedList<MirInstruction>::iterator it,
                                  std::pmr::vector<class MirInstruction *> &popArgs);

  private:
    class MirBuilderContext *m_ctx; ///< Builder context used to create the lowered move/load instructions.
};

#endif // EZTRIPLE_MIR_ABI_LOWERER_H