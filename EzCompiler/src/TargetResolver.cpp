#include "TargetResolver.h"
#include "Targets/X86_64/X86_64TargetDesc.h"
#include "Builder/MirBuilderContext.h"
#include "FrameLowerer/MirFrameLowerer.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "InstructionSelector/MirAddressingModeMatcher.h"
#include "Legalizer/MirLegalizer.h"
#include "Legalizer/LegalizerInfo.h"
#include "RegisterAllocator/MirRegisterAllocator.h"
#include "Function/CallingConvDesc.h"
#include "Descriptors/TargetBinaryDesc.h"

namespace EzCompiler
{

ResolvedTarget TargetResolver::resolve(const TargetTriple &triple, MirBuilderContext *mirCtx)
{
    ResolvedTarget result;

    if (triple.isX86_64())
    {
        auto x86Target = std::make_unique<EzTriple::X86_64TargetDesc>(mirCtx);
        x86Target->initialize();

        if (triple.isWindows())
        {
            result.m_callingConv = x86Target->getWin64CallingConv();
            result.m_binaryDesc = x86Target->getCoffBinaryDesc();
        }
        else
        {
            result.m_callingConv = x86Target->getSysVCallingConv();
            result.m_binaryDesc = x86Target->getElfBinaryDesc();
        }

        result.m_targetDesc = std::move(x86Target);
    }

    return result;
}

} // namespace EzCompiler
