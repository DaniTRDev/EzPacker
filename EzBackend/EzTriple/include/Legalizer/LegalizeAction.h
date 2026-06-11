#ifndef EZPACKER_LEGALIZEACTION_H
#define EZPACKER_LEGALIZEACTION_H

#include "EzTripleCommon.h"

struct LegalizeActionResult
{
    bool m_executed;
    bool m_succeeded;
    bool m_mirChanged;
};

class LegalizeAction
{
  public:
    virtual ~LegalizeAction() = default;

    /**
     * Returns the name of the action.
     * @return
     */
    virtual const char *getName() = 0;

    /**
     * Executes the given legalize action on the given instruction (pointed by the iterator received). Passes an
     * input iterator that will point to the new end of the added instructions.
     * @param instrList
     * @param it
     * @return
     */
    virtual LegalizeActionResult run(std::pmr::list<class MirInstruction *> &instrList,
                                     std::pmr::list<class MirInstruction *>::iterator &it) = 0;
};

#endif // EZPACKER_LEGALIZEACTION_H
