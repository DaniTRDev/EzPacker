#include "TargetResolver.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "Function/CallingConvDesc.h"
#include "Targets/X86_64/X86_64TargetDesc.h"

#include <memory>

namespace
{

/**
 * Registers the x86-64 target factory. Keeping the architecture-specific selection here
 * lets TargetResolver remain a pure registry with no target includes.
 */
struct X86_64TargetRegistration
{
    X86_64TargetRegistration()
    {
        EzCompiler::TargetResolver::registerTarget(
                "x86_64",
                [](const EzCompiler::TargetTriple &triple,
                   MirBuilderContext *mirCtx,
                   bool isPositionIndependent) -> EzCompiler::ResolvedTarget
                {
                    EzCompiler::ResolvedTarget result;

                    auto target = std::make_unique<EzTriple::X86_64TargetDesc>(mirCtx);
                    target->setPositionIndependent(isPositionIndependent);
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
};

const X86_64TargetRegistration g_x86_64TargetRegistration;

} // namespace
