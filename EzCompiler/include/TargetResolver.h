#ifndef EZPACKER_TARGET_RESOLVER_H
#define EZPACKER_TARGET_RESOLVER_H

#include "EzCompilerCommon.h"
#include "TargetTriple.h"
#include "Descriptors/TargetDesc.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "Function/CallingConvDesc.h"

#include <functional>
#include <string_view>

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
 * Factory invoked when a registered architecture matches a requested triple.
 * The factory fully encapsulates architecture-specific target, calling convention and
 * binary descriptor selection so TargetResolver stays target-agnostic.
 */
using TargetFactory = std::function<ResolvedTarget(const TargetTriple &, MirBuilderContext *)>;

/**
 * Resolves a TargetTriple to a concrete TargetDesc, CallingConvDesc, and TargetBinaryDesc
 * using a registry of architecture factories.
 */
class TargetResolver
{
  public:
    /**
     * Registers (or replaces) the factory responsible for an architecture (e.g. "x86_64").
     */
    static void registerTarget(std::string_view arch, TargetFactory factory);

    /**
     * Resolves a target triple to a concrete target, calling convention, and binary descriptor.
     * Returns an empty ResolvedTarget when the architecture is not registered.
     */
    static ResolvedTarget resolve(const TargetTriple &triple, MirBuilderContext *mirCtx);
};

} // namespace EzCompiler

#endif // EZPACKER_TARGET_RESOLVER_H
