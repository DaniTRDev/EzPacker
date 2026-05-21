#ifndef EZPACKER_X64EMITTER_H
#define EZPACKER_X64EMITTER_H

#include "x64TypeLegalizer.h"
#include "x64InstrSelector.h"

class x64Emitter
{
  public:
    x64Emitter(MirEmitter *emitter);

    /**
     * Loads the emitter, initializes everything and ensure it's in a proper state before starting the emission. Returns
     * true if succeeded.
     * @return
     */
    bool load();

    /**
     * Emits the given mir function inside the code buffer.
     * @param func
     * @param buff
     * @return
     */
    bool emitFunc(MirFunction *func, CodeBuffer *buff);

  private:
    /**
     * Emits the labels for the given function. This allows forwarded refs.
     * @param func
     * @return
     */
    bool emitFuncLabels(MirFunction *func);

    /**
     * Emits a block of code. Returns true if succeeded.
     * @param block
     * @return
     */
    bool emitBlock(MirBlock *block);

    /**
     * Emits an instruction. Returns true if succeeded.
     * @param instr
     * @return
     */
    bool emitInstr(MirInstruction *instr);

    /**
     * Returns an asmjit operand out of the given MirOperand.
     * @param operand
     * @param instr
     * @return
     */
    asmjit::Operand getAsmjitOperandFromMirOperand(MirOperand *operand, asmjit::x86::Inst::Id instr);

  private:
    MirEmitter *m_emitter;

    asmjit::CodeHolder m_codeHolder;
    asmjit::Environment m_env;
    asmjit::x86::Assembler m_assembler;
    asmjit::StringLogger m_logger;

    /**
     * Used to know to which label is a MirId linked to.
     */
    std::map<size_t, asmjit::Label> m_asmjitLabels;
};

#endif // EZPACKER_X64EMITTER_H
