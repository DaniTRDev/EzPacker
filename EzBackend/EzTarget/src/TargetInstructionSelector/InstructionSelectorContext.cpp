#include "TargetInstructionSelector/InstructionSelectorContext.h"

InstructionSelectionContext::InstructionSelectionContext(InstructionSelectionTable *selectionTable,
                                                         MirEmitter *emitter,
                                                         TargetDesc *targetDesc) :
    m_selectionTable(selectionTable), m_emitter(emitter), m_targetDesc(targetDesc)
{
}

MirEmitter *InstructionSelectionContext::getEmitter() const { return m_emitter; }

TargetDesc *InstructionSelectionContext::getTargetDesc() const { return m_targetDesc; }

InstructionSelectionTable *InstructionSelectionContext::getSelectionTable() const { return m_selectionTable; }
