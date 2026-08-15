#ifndef EZPACKER_CODEEMITTER_H
#define EZPACKER_CODEEMITTER_H

#include "EzCodeEmitterCommon.h"
#include "GenericCodeEmitter.h"

struct CodeEmitterCtx
{
    MirBuilderContext *m_ctx;
    GenericCodeEmitter *m_emitter;

    // Instruction that's going to be emitted.
    std::pmr::list<MirInstruction *>::iterator m_instrIt;

    // A map is used because there will surely be lots of relocations near in the same code, an unordered_map's hash
    // table would not fit here.
    std::pmr::map<uint64_t, Relocation> m_relocations;

    // In this case a hash table is good enough because the pointer addresses are going to be distant one from another.
    std::pmr::unordered_map<MirBlock *, CodeLabel> m_labels;

    CodeEmitterCtx(MirBuilderContext *ctx,
                   GenericCodeEmitter *emitter,
                   std::pmr::list<MirInstruction *>::iterator instrIt,
                   std::pmr::memory_resource *allocator) :
        m_ctx(ctx), m_emitter(emitter), m_instrIt(instrIt), m_relocations(allocator), m_labels(allocator)
    {
    }
};

enum class CodeEmitterResult : uint8_t
{
    Emitted,  // The instruction was correctly emitted.
    NoAction, // There wasn't any action registered for the given target instruction.
    Error     // There was an error during emission, more information is available in the diagnostics collector.
};

// Type used to define an emitter action, which is responsible of emitting a target machine instruction.
using CodeEmitterAction = std::function<CodeEmitterResult(CodeEmitterCtx &ctx)>;

class CodeEmitter
{
  public:
    /**
     * Creates the registry with the given context.
     */
    CodeEmitter(MirBuilderContext *ctx);

    /**
     * Tries to emit the instruction at m_instIt. If there wasn't any rule, NoAction is returned. If there was any
     * error, Error is returned. If the function succeeded, it returns Emitted.
     */
    CodeEmitterResult emit(CodeEmitterCtx &ctx);

    /**
     * Adds a rule for the given target that will execute the given action. If there was any other action registered, it
     * will be replaced.
     */
    void addRule(MirTargetInstructionId target, CodeEmitterAction action);

  private:
    std::pmr::unordered_map<MirTargetInstructionId, CodeEmitterAction> m_rules;
};

#endif // EZPACKER_CODEEMITTER_H