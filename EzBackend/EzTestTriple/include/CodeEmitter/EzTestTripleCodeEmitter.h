#ifndef EZPACKER_EZTESTTRIPLECODEEMITTER_H
#define EZPACKER_EZTESTTRIPLECODEEMITTER_H

#include "EzTestTripleCommon.h"

class EzTestTripleCodeEmitter : public GenericCodeEmitter
{
  public:
    /**
     * Initializes the context for the function and aligns the text section entry point.
     */
    void beginFunction(CodeEmitterContext *ctx, std::string_view name) override;

    /**
     * Binds the given label ID in the active context/section and positions the emission cursor.
     */
    void bindLabel(MirId labelId) override;

    /**
     * Completes function emission and cleans up function-local state.
     */
    void endFunction(CodeEmitterContext *ctx) override;

    /**
     * Emits a target instruction using EzTestTriple's encoding example into the current section.
     */
    void emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands) override;

  private:
    CodeEmitterContext *m_ctx{ nullptr };
};

#endif // EZPACKER_EZTESTTRIPLECODEEMITTER_H