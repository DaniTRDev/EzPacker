#include "EzTargetsRiscV64Registration.h"

#include "TargetResolver.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "Function/CallingConvDesc.h"
#include "RiscV64TargetDesc.h"

#include <memory>

namespace EzTargets::RiscV64
{

void registerTarget()
{
    auto factory = [](const EzCompiler::TargetTriple &triple,
                      MirBuilderContext *mirCtx,
                      bool isPositionIndependent,
                      const std::vector<std::string> &features) -> EzCompiler::ResolvedTarget
    {
        (void)triple;
        EzCompiler::ResolvedTarget result;

        auto target = std::make_unique<EzTargets::RiscV64::RiscV64TargetDesc>(mirCtx);
        target->setPositionIndependent(isPositionIndependent);
        target->applyFeatures(features);
        target->initialize();

        result.m_callingConv = target->getDefaultCallingConv();
        result.m_binaryDesc = target->getElfBinaryDesc();
        result.m_targetDesc = std::move(target);
        return result;
    };

    EzCompiler::TargetResolver::registerTarget("riscv64", factory);
    EzCompiler::TargetResolver::registerTarget("riscv", factory);
    EzCompiler::TargetResolver::registerTarget("risc-v", factory);
}

} // namespace EzTargets::RiscV64

namespace
{

struct RiscV64TargetRegistration
{
    RiscV64TargetRegistration() { EzTargets::RiscV64::registerTarget(); }
};

const RiscV64TargetRegistration g_riscv64TargetRegistration;

} // namespace
