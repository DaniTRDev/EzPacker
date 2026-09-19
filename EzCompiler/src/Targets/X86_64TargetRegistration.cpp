#include "TargetResolver.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "Descriptors/TargetRelocationResolver.h"
#include "FrameLowerer/MirFrameLowerer.h"
#include "Function/CallingConvDesc.h"
#include "InstructionSelector/MirAddressingModeMatcher.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "Legalizer/LegalizerInfo.h"
#include "Legalizer/MirLegalizer.h"
#include "RegisterAllocator/MirRegisterAllocator.h"
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
                [](const EzCompiler::TargetTriple &triple, MirBuilderContext *mirCtx) -> EzCompiler::ResolvedTarget
                {
                    EzCompiler::ResolvedTarget result;

                    auto target = std::make_unique<EzTriple::X86_64TargetDesc>(mirCtx);
                    target->initialize();

                    if (triple.isWindows())
                    {
                        result.m_callingConv = target->getWin64CallingConv();
                        result.m_binaryDesc = target->getCoffBinaryDesc();
                    }
                    else
                    {
                        result.m_callingConv = target->getSysVCallingConv();
                        result.m_binaryDesc = target->getElfBinaryDesc();
                    }

                    result.m_targetDesc = std::move(target);
                    return result;
                });
    }
};

const X86_64TargetRegistration g_x86_64TargetRegistration;

} // namespace
