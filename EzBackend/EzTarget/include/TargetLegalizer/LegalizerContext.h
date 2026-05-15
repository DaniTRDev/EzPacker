#ifndef EZPACKER_LEGALIZERCONTEXT_H
#define EZPACKER_LEGALIZERCONTEXT_H

#include "EzTargetCommon.h"
#include "LegalizerActionList.h"
#include "LegalizerHandlerList.h"
#include "TargetDesc.h"

class LegalizerContext
{
  public:
    /**
     * Constructor for LegalizerContext.
     * @param actionList
     * @param handlerList
     * @param emitter
     */
    LegalizerContext(TargetDesc *targetDesc,
                     LegalizerActionList *actionList,
                     LegalizerHandlerList *handlerList,
                     MirEmitter *emitter);

    /**
     * Returns true if the given register has been previously expanded.
     * @param reg
     * @return
     */
    bool isRegisterExpanded(MirRegister *reg);

    /**
     * Returns the action list.
     * @return
     */
    LegalizerActionList *getActionList() const;

    /**
     * Returns the handler list.
     * @return
     */
    LegalizerHandlerList *getHandlerList() const;

    /**
     * Returns the MIR emitter.
     * @return
     */
    MirEmitter *getEmitter() const;

    /**
     * Returns the target description.
     * @return
     */
    TargetDesc *getTargetDesc() const;

    /**
     * Adds a register to the list of expanded registers.
     * @param registerId
     * @param expanded
     */
    void addExpandedRegister(MirRegister *reg, const std::array<MirRegister*, 2> &expanded);

    /**
     * Returns the list of expanded registers.  isRegisterExpanded must have returned true.
     * @param reg
     * @return
     */
    const std::array<MirRegister*, 2> &getExpandedRegister(MirRegister *reg);

  private:
    LegalizerActionList *m_actionList;
    LegalizerHandlerList *m_handlerList;
    MirEmitter *m_emitter;
    TargetDesc *m_targetDesc;

    std::unordered_map<size_t, std::array<MirRegister *, 2>> m_expandedRegisters;
};

#endif // EZPACKER_LEGALIZERCONTEXT_H
