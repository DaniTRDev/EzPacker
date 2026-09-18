#ifndef EZPACKER_TARGET_RESOLVER_H
#define EZPACKER_TARGET_RESOLVER_H

#include "EzCompilerCommon.h"
#include "TargetTriple.h"
#include "Descriptors/TargetDesc.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "Function/CallingConvDesc.h"

class MirBuilderContext;

namespace EzCompiler
{

struct ResolvedTarget
{
    std::unique_ptr<TargetDesc> m_targetDesc;
    CallingConvDesc *m_callingConv{ nullptr };
    TargetBinaryDesc *m_binaryDesc{ nullptr };
};

/**
 * Resolves a TargetTriple to a concrete TargetDesc, CallingConvDesc, and TargetBinaryDesc.
 */
class TargetResolver
{
  public:
    static ResolvedTarget resolve(const TargetTriple &triple, MirBuilderContext *mirCtx);
};

} // namespace EzCompiler

#endif // EZPACKER_TARGET_RESOLVER_H
