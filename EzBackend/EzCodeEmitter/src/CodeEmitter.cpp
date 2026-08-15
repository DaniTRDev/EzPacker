#include "CodeEmitter.h"

CodeEmitter::CodeEmitter(MirBuilderContext *ctx) : m_rules(ctx->getGlobalAllocator()) {}

CodeEmitterResult CodeEmitter::emit(CodeEmitterCtx &ctx)
{
    MirInstruction *instr = *ctx.m_instrIt;
    MirTargetInstructionId targetId = instr->getTargetId();

    auto it = m_rules.find(targetId);
    if (it == m_rules.end())
        return CodeEmitterResult::NoAction;

    return it->second(ctx);
}

void CodeEmitter::addRule(MirTargetInstructionId target, CodeEmitterAction action)
{
    m_rules[target] = std::move(action);
}