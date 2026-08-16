#include "Legalizer/Expand/MirExpansionRuleRegistry.h"

MirExpansionRuleRegistry::MirExpansionRuleRegistry(MirBuilderContext *ctx) :
    m_rules(ctx->getGlobalAllocator()), m_allocator(ctx->getGlobalAllocator())
{
}

ExpansionRule *MirExpansionRuleRegistry::getRule(ExpansionContext &ctx)
{
    auto instr = *ctx.m_it;
    auto it = m_rules.find(instr->getOpCode());

    if (it == m_rules.end())
        return nullptr;

    for (auto &rule : it->second)
    {
        if (!rule->m_pred || rule->m_pred(ctx))
            return rule;
    }

    return nullptr;
}

void MirExpansionRuleRegistry::addRule(MirInstructionOpCode opcode, ExpansionRule rule)
{
    std::pmr::polymorphic_allocator<ExpansionRule> allocator(m_allocator);
    ExpansionRule *ptr = allocator.new_object<ExpansionRule>(std::move(rule));
    m_rules[opcode].push_back(ptr);
}

std::pmr::memory_resource *MirExpansionRuleRegistry::getAllocator() { return m_allocator; }