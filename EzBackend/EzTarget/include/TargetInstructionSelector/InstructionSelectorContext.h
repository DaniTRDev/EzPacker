#ifndef EZPACKER_INSTRUCTIONSELECTORCONTEXT_H
#define EZPACKER_INSTRUCTIONSELECTORCONTEXT_H

#include "EzTargetCommon.h"
#include "TargetDesc.h"
#include "InstructionSelectionTable.h"

class InstructionSelectionContext
{
  public:
    /**
     * Constructor for InstructionSelectionContext.
     * @param emitter
     * @param targetDesc
     */
    InstructionSelectionContext(InstructionSelectionTable *selectionTable, MirEmitter *emitter, TargetDesc *targetDesc);

    /**
     * Returns the emitter used.
     * @return
     */
    MirEmitter *getEmitter() const;

    /**
     * Returns the target desc used.
     * @return
     */
    TargetDesc *getTargetDesc() const;

    /**
     * Returns the selection table.
     * @return
     */
    InstructionSelectionTable *getSelectionTable() const;

  private:
    InstructionSelectionTable *m_selectionTable;
    MirEmitter *m_emitter;
    TargetDesc *m_targetDesc;
};

#endif // EZPACKER_INSTRUCTIONSELECTORCONTEXT_H
