#include "EzTargetsX86_64Registration.h"

#include "TargetResolver.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "Function/CallingConvDesc.h"
#include "X86_64TargetDesc.h"

#include <memory>

namespace EzTargets::X86_64
{

/**
 * Registers the x86-64 target factory. Keeping the architecture-specific selection here
 * lets TargetResolver remain a pure registry with no target includes.
 */
void registerTarget()
{
    EzCompiler::TargetResolver::registerTarget(
            "x86_64",
            [](const EzCompiler::TargetTriple &triple,
               MirBuilderContext *mirCtx,
               bool isPositionIndependent,
               const std::vector<std::string> &features) -> EzCompiler::ResolvedTarget
            {
                EzCompiler::ResolvedTarget result;

                auto target = std::make_unique<EzTargets::X86_64::X86_64TargetDesc>(mirCtx);
                target->setPositionIndependent(isPositionIndependent);
                target->applyFeatures(features);
                target->initialize();

                // Select the object format from the triple's own predicates: Windows maps to
                // COFF, ELF is the non-Mach-O default, and anything else is unsupported.
                if (triple.isCoff())
                {
                    result.m_callingConv = target->getWin64CallingConv();
                    result.m_binaryDesc = target->getCoffBinaryDesc();
                }
                else if (triple.isElf())
                {
                    result.m_callingConv = target->getSysVCallingConv();
                    result.m_binaryDesc = target->getElfBinaryDesc();
                }
                else
                {
                    return {};
                }

                result.m_targetDesc = std::move(target);
                return result;
            });
}

} // namespace EzTargets::X86_64

namespace
{

// Registers the x86-64 target factory at load time.
struct X86_64TargetRegistration
{
    X86_64TargetRegistration() { EzTargets::X86_64::registerTarget(); }
};

const X86_64TargetRegistration g_x86_64TargetRegistration;

} // namespace
