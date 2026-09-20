#include "Sema/Encoding/EncodingDialect.h"
#include "NameRegistry.h"

namespace Sema::Encoding
{
namespace
{

// Function-local static registry: constructed on first use to avoid static-initialization order issues.
NameRegistry<EncodingDialect *> &registry()
{
    static NameRegistry<EncodingDialect *> s_registry;
    return s_registry;
}

// Canonical name of the dialect used when an instruction's target name does not resolve to one.
// Pinning the default to a named constant keeps behavior independent of static-registration order
// (previously "last registered"), so adding a second ISA cannot silently change the fallback.
constexpr std::string_view kDefaultDialectName = "x86_64";

} // namespace

void registerEncodingDialect(std::string_view name, EncodingDialect *dialect)
{
    if (name.empty() || !dialect)
    {
        return;
    }

    registry().add(name, dialect);
}

EncodingDialect *findEncodingDialect(std::string_view name) { return registry().find(name); }

EncodingDialect *getDefaultEncodingDialect() { return registry().find(kDefaultDialectName); }

} // namespace Sema::Encoding
