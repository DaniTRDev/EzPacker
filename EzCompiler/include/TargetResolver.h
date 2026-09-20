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
    std::unique_ptr<TargetDesc> m_targetDesc;  ///< Owning pointer to the selected target descriptor.
    CallingConvDesc *m_callingConv{ nullptr }; ///< Non-owning calling convention from the target.
    TargetBinaryDesc *m_binaryDesc{ nullptr }; ///< Non-owning binary descriptor from the target.
};

/**
 * Factory invoked when a registered architecture matches a requested triple.
 * The factory fully encapsulates architecture-specific target, calling convention and
 * binary descriptor selection so TargetResolver stays target-agnostic.
 */
using TargetFactory = std::function<ResolvedTarget(const TargetTriple &, MirBuilderContext *, bool isPositionIndependent)>;

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
     *
     * @param triple                Requested architecture/OS/ABI triple.
     * @param mirCtx                Shared builder context passed to the target.
     * @param isPositionIndependent Forwards the driver's -fPIC request to the target factory.
     */
    static ResolvedTarget resolve(const TargetTriple &triple, MirBuilderContext *mirCtx, bool isPositionIndependent);
};

} // namespace EzCompiler

#endif // EZPACKER_TARGET_RESOLVER_H
