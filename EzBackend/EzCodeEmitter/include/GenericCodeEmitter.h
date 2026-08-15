#ifndef EZPACKER_GENERICCODEEMITTER_H
#define EZPACKER_GENERICCODEEMITTER_H

#include "EzCodeEmitterCommon.h"
#include "CodeEmitterContext.h"

/**
 * Interface used as to contain the basic functionality needed by a code emitter to be able to emit code in a target.
 *
 * If any error is generated, an EXCEPTION must be thrown and error information must be pushed into the
 * diagnostic collector.
 *
 * If an instruction reaches this step, it's guaranteed that it has the correct types/values. This means that for
 * example using mov(RegisterRef, FlexInt), is a 1:1 translation that doesn't need further checks because the backend
 * has ensured the value can be moved.
 */
class GenericCodeEmitter
{
  public:
    virtual ~GenericCodeEmitter() = default;

    /**
     * Creates a code label.
     */
    virtual CodeLabel *getOrCreateLabel(MirId id) = 0;

    /**
     * Creates a relocation out of the given reference and address.
     */
    virtual CodeLabel *createReloc(MirReference *ref, uint64_t address) = 0;

    /**
     * Binds the given label ID to the emitter so the next instructions are emitted inside this label.
     */
    virtual void bindBlockLabel(size_t labelId) = 0;

    /**
     * Begins a function.
     */
    virtual void beginFunction(CodeEmitterContext *ctx, std::string_view name) = 0;

    /**
     * Ends a function.
     */
    virtual void endFunction(CodeEmitterContext *ctx) = 0;

    /**
     * Emits the instruction with the given operands and target desc.
     */
    virtual void emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands) = 0;
};

#endif // EZPACKER_GENERICCODEEMITTER_H