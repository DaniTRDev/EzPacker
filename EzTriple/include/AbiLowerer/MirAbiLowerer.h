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
    /// Result of an ABI location assignment; lets callers emit their direction-specific diagnostics.
    enum class AbiAssignFailure
    {
        None,
        ValueNotRegister,
        MissingSretParam,
        UnsupportedLocation
    };

    /// Direction of a location assignment relative to the ABI-provided location.
    enum class AbiDirection
    {
        ValueToLocation, ///< Copy the value operand into the location (arguments / return values).
        LocationToValue  ///< Copy the location into the destination operand (call results / parameters).
    };

    /// How a multi-register Split location moves each piece.
    enum class AbiSplitKind
    {
        RegisterMove,     ///< MOV locationReg, value.
        LoadIntoLocation, ///< LOAD locationReg, [valueBase + pieceOffset].
        StoreFromLocation ///< STORE [valueBase + pieceOffset], locationReg.
    };

    /// Extra handling for an Indirect location beyond the plain pointer move.
    enum class AbiIndirectKind
    {
        None,       ///< Emit nothing (return location that carries no copy-on-register work).
        Plain,      ///< MOV pointerReg, value / MOV value, pointerReg.
        ByValStack, ///< Materialize a stack copy, then pass its address (call arguments).
        SretCopy    ///< Copy the hidden sret pointer into the target register (return values).
    };

    /// Everything needed to emit one ABI location assignment; see assignAbiLocation.
    struct AbiAssignSpec;

    /**
     * Emits the single "assign a value to/from an ABI location" step for one operand: maps the
     * location kind (register / split / indirect / stack) and the requested direction to the
     * concrete MIR moves.
     */
    AbiAssignFailure assignAbiLocation(const AbiAssignSpec &spec);

    class MirBuilderContext *m_ctx; ///< Builder context used to create the lowered move/load instructions.
};

#endif // EZTRIPLE_MIR_ABI_LOWERER_H