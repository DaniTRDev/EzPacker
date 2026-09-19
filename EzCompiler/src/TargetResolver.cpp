#include "TargetResolver.h"

#include <algorithm>
#include <cctype>
#include <map>

namespace EzCompiler
{

namespace
{

// Lowercases an architecture name and maps '-' to '_' so lookups are case/separator insensitive.
std::string normalizeArch(std::string_view arch)
{
    std::string result(arch);
    for (char &c : result)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        c = static_cast<char>(std::tolower(uc));
        if (c == '-')
        {
            c = '_';
        }
    }
    return result;
}

// Process-wide registry of architecture factories; function-local static avoids init-order issues.
std::map<std::string, TargetFactory, std::less<>> &targetRegistry()
{
    static std::map<std::string, TargetFactory, std::less<>> registry;
    return registry;
}

} // namespace

void TargetResolver::registerTarget(std::string_view arch, TargetFactory factory)
{
    targetRegistry()[normalizeArch(arch)] = std::move(factory);
}

ResolvedTarget TargetResolver::resolve(const TargetTriple &triple, MirBuilderContext *mirCtx)
{
    auto &registry = targetRegistry();
    auto it = registry.find(normalizeArch(triple.getArch()));
    if (it == registry.end())
    {
        // Unknown architecture: report an empty target so the driver can diagnose it.
        return {};
    }

    return it->second(triple, mirCtx);
}

} // namespace EzCompiler
