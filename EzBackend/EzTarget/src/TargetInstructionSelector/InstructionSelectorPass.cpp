#include "TargetInstructionSelector/InstructionSelectorPass.h"

bool InstructionSelectorPass::run(TypedPoolLinkedList<struct MirInstruction> *instrList,
                                  TypedPoolLinkedList<struct MirInstruction>::Iterator it,
                                  struct MirPassManager *passManager)
{
    bool modified = false;

    
    
    return modified;
}

MirPassIterationPlace InstructionSelectorPass::getIterationPlace() const { return MirPassIterationPlace::Instruction; }
