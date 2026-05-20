#ifndef EZPACKER_IMIRPASS_H
#define EZPACKER_IMIRPASS_H

#include "EzMirCommon.h"

enum class MirPassType : uint8_t
{
    Analysis, // Only reads the MIR.
    Transform // Can apply changes to the MIR.
};

enum class MirPassIterationPlace : uint8_t
{
    Function,   // The pass runs on each function.
    Block,      // The pass runs on each block.
    Instruction // The pass runs on each instruction.
};

/**
 * Interface used by passes that iterate over the MIR.
 */
class IMirPass
{
  public:
    virtual ~IMirPass() = default;

    /**
     * Runs the pass on the given MIR func. Returns false if the list (or the elem inside the iterator) that holds the
     * iterator was NOT modified.
     */
    virtual bool run(TypedPoolLinkedList<class MirFunction> *funcList,
                     TypedPoolLinkedList<class MirFunction>::Iterator it,
                     class MirPassManager *passManager)
    {
        return false;
    }

    /**
     * Runs the pass on the given MIR block. Returns false if the list (or the elem inside the iterator) that holds the
     * iterator was NOT modified.
     */
    virtual bool run(TypedPoolLinkedList<class MirBlock> *blockList,
                     TypedPoolLinkedList<class MirBlock>::Iterator it,
                     class MirPassManager *passManager)
    {
        return false;
    }

    /**
     * Runs the pass on the given MIR func. Returns false if the list (or the elem inside the iterator) that holds the
     * iterator was NOT modified.
     */
    virtual bool run(TypedPoolLinkedList<class MirInstruction> *instrList,
                     TypedPoolLinkedList<class MirInstruction>::Iterator it,
                     class MirPassManager *passManager)
    {
        return false;
    }

    /**
     * Returns the name of the pass.
     * @return
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the iteration place. Depending on the place, one callback or the other will be called.
     * @return
     */
    virtual MirPassIterationPlace getIterationPlace() const = 0;

    /**
     * Returns the pass type.
     * @return
     */
    virtual MirPassType getPassType() const = 0;

    /**
     * Called by the pass manager when the pass needs to be reset.
     */
    virtual void reset() { return; };
};

#endif // EZPACKER_IMIRPASS_H
