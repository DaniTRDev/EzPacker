#include "TargetResolver.h"
#include "NameRegistry.h"

namespace EzCompiler
{

namespace
{

// Process-wide registry of architecture factories; function-local static avoids init-order issues.
NameRegistry<TargetFactory> &targetRegistry()
{
    static NameRegistry<TargetFactory> registry;
    return registry;
}

} // namespace

void TargetResolver::registerTarget(std::string_view arch, TargetFactory factory)
{
    targetRegistry().add(arch, std::move(factory));
}

ResolvedTarget TargetResolver::resolve(const TargetTriple &triple,
                                      MirBuilderContext *mirCtx,
                                      bool isPositionIndependent,
                                      const std::vector<std::string> &features)
{
    TargetFactory factory = targetRegistry().find(triple.getArch());
    if (!factory)
    {
        // Unknown architecture: report an empty target so the driver can diagnose it.
        return {};
    }

    return factory(triple, mirCtx, isPositionIndependent, features);
}

} // namespace EzCompiler
