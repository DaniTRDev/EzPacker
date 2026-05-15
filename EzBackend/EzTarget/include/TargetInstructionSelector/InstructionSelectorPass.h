#ifndef EZPACKER_INSTRUCTIONSELECTORPASS_H
#define EZPACKER_INSTRUCTIONSELECTORPASS_H

#include "EzTargetCommon.h"
#include "InstructionSelectorContext.h"

class InstructionSelectorPass : public IMirTransformPass
{
  public:
    /**
     * Runs the pass on the given MIR instruction and generates the valid target instruction.
     */
    bool run(TypedPoolLinkedList<class MirInstruction> *instrList,
             TypedPoolLinkedList<class MirInstruction>::Iterator it,
             class MirPassManager *passManager) override;

    /**
     * Returns MirPassIterationPlace::Instruction, as this pass runs on each instruction.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

  private:
};

#endif // EZPACKER_INSTRUCTIONSELECTORPASS_H
