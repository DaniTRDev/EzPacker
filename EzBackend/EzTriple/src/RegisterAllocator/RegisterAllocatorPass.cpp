#include "RegisterAllocator/RegisterAllocatorPass.h"

RegisterAllocatorPass::RegisterAllocatorPass(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *RegisterAllocatorPass::getName() const { return "RegisterAllocatorPass"; }

MirPassIterationPlace RegisterAllocatorPass::getIterationPlace() const { return MirPassIterationPlace::Function; }

MirPassResult RegisterAllocatorPass::run(std::pmr::list<class MirFunction *> &funcList,
                                         std::pmr::list<class MirFunction *>::iterator it,
                                         class MirPassManager *passManager)
{
    MirFunction *func = *it;


    return { .m_modifiedMir = true, .m_executed = true, .m_succeeded = true };
}

void RegisterAllocatorPass::printResult() const {}

std::vector<std::type_index> RegisterAllocatorPass::getDependencies() const
{
    return { std::type_index(typeid(MirInstructionSelectorPass)) };
}