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

EncodingDialect *&lastRegistered()
{
    static EncodingDialect *s_last = nullptr;
    return s_last;
}

} // namespace

void registerEncodingDialect(std::string_view name, EncodingDialect *dialect)
{
    if (name.empty() || !dialect)
    {
        return;
    }

    registry().add(name, dialect);
    lastRegistered() = dialect;
}

EncodingDialect *findEncodingDialect(std::string_view name) { return registry().find(name); }

EncodingDialect *getDefaultEncodingDialect() { return lastRegistered(); }

} // namespace Sema::Encoding
