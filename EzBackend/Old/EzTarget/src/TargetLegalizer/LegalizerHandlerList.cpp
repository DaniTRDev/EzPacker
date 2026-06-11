#include "TargetLegalizer/LegalizerHandlerList.h"

void LegalizerHandlerList::addInstructionHandler(MirInstructionOpCode opcode, Callback *handler)
{
    m_instructionHandlers[static_cast<size_t>(opcode)].push_back(handler);
}

std::vector<LegalizerHandlerList::Callback *> LegalizerHandlerList::getInstructionHandlers(MirInstructionOpCode opcode) const
{
    const auto &handlersForOpcode = m_instructionHandlers[static_cast<size_t>(opcode)];
    if (handlersForOpcode.empty())
    {
        return {}; // No handlers registered for this opcode.
    }

    return handlersForOpcode;
}