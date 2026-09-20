#ifndef EZPACKER_GENERICCODEEMITTER_H
#define EZPACKER_GENERICCODEEMITTER_H

#include "EzCodeEmitterCommon.h"
#include "CodeEmitterContext.h"
#include "Function/MirFunction.h"

class MirOperand;
class MirTargetInstructionDesc;

/**
 * Interface used as to contain the basic functionality needed by a code emitter to be able to emit code in a target.
 *
 * If any error is generated, an EXCEPTION must be thrown and error information must be pushed into the
 * diagnostic collector.
 *
 * If an instruction reaches this step, it's guaranteed that it has the correct types/values. This means that for
 * example using mov(MirRegisterRef, FlexInt), is a 1:1 translation that doesn't need further checks because the backend
 * has ensured the value can be moved.
 */
class GenericCodeEmitter
{
  public:
    virtual ~GenericCodeEmitter() = default;

    /**
     * Begins a function.
     */
    virtual void beginFunction(CodeEmitterContext *ctx, std::string_view name) = 0;

    /**
     * Begins a function described by a MIR function.
     *
     * The default delegates to the name-based overload using the function's name, so targets
     * that do not need the function body only implement the string_view overload.
     */
    virtual void beginFunction(CodeEmitterContext *ctx, MirFunction *func)
    {
        beginFunction(ctx, func ? func->getName() : std::string_view{});
    }

    /**
     * Binds the given label ID to the emitter so the next instructions are emitted inside this label.
     */
    virtual void bindLabel(MirId labelId) = 0;

    /**
     * Ends a function.
     */
    virtual void endFunction(CodeEmitterContext *ctx) = 0;

    /**
     * Ends a function described by a MIR function.
     *
     * The default delegates to the context-only overload.
     */
    virtual void endFunction(CodeEmitterContext *ctx, MirFunction *func) { endFunction(ctx); }

    /**
     * Emits the instruction with the given operands and target desc.
     */
    virtual void emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands) = 0;
};

#endif // EZPACKER_GENERICCODEEMITTER_H