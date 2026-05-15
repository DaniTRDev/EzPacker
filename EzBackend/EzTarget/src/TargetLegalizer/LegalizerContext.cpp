#include "TargetLegalizer/LegalizerContext.h"

LegalizerContext::LegalizerContext(TargetDesc *targetDesc,
                                   LegalizerActionList *actionList,
                                   LegalizerHandlerList *handlerList,
                                   MirEmitter *emitter) :
    m_targetDesc(targetDesc), m_actionList(actionList), m_handlerList(handlerList), m_emitter(emitter)
{
}

bool LegalizerContext::isRegisterExpanded(MirRegister *reg)
{
    const auto &it = m_expandedRegisters.find(reg->getRegId());
    if (it != m_expandedRegisters.end())
    {
        return true;
    }
    return false;
}

LegalizerActionList *LegalizerContext::getActionList() const { return m_actionList; };

LegalizerHandlerList *LegalizerContext::getHandlerList() const { return m_handlerList; };

MirEmitter *LegalizerContext::getEmitter() const { return m_emitter; }

TargetDesc *LegalizerContext::getTargetDesc() const { return m_targetDesc; }

void LegalizerContext::addExpandedRegister(MirRegister *reg, const std::array<MirRegister*, 2> &expanded)
{
    m_expandedRegisters[reg->getRegId()] = expanded;
}

const std::array<MirRegister*, 2> &LegalizerContext::getExpandedRegister(MirRegister *reg)
{
    return m_expandedRegisters.at(reg->getRegId());
}
