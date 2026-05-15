#ifndef EZPACKER_LEGALIZERHANDLER_H
#define EZPACKER_LEGALIZERHANDLER_H

#include "EzTargetCommon.h"

struct LegalizerHandlerResult
{
    bool m_error;     // Whether an error occurred during legalization.
    bool m_legalized; // Whether the instruction was successfully legalized.
    bool m_modified;  // Whether the instruction was modified during legalization.
};

class LegalizerHandlerList
{
  public:
    using Callback = LegalizerHandlerResult(TypedPoolLinkedList<struct MirInstruction> *instrList,
                                            TypedPoolLinkedList<class MirInstruction>::Iterator it);

    /**
     * Registers a legalization handler for a specific instruction opcode. These handlers will only be invoked if
     * the legalization action is set to custom.
     * @param opcode
     * @param handler
     */
    void addInstructionHandler(MirInstructionOpCode opcode, Callback *handler);
    
    /**
     * Retrieves the list of legalization handlers registered for a specific instruction opcode.
     * @param opcode
     * @return
     */
    std::vector<Callback *> getInstructionHandlers(MirInstructionOpCode opcode) const;

  private:
    std::array<std::vector<Callback *>, static_cast<size_t>(MirInstructionOpCode::OPCODE_COUNT)> m_instructionHandlers;
    std::unordered_map<size_t, Callback*> m_otherCallbacks; // Callbacks that are not related to instructions.
};

#endif // EZPACKER_LEGALIZERHANDLER_H
