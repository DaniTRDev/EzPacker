#include "Sema/Encoding/EncodingDialect.h"

#include <cctype>
#include <string>
#include <unordered_map>

namespace Sema::Encoding
{
namespace
{

// Normalizes a dialect name to a case-insensitive lookup key.
std::string normalize(std::string_view name)
{
    std::string result;
    result.reserve(name.size());
    for (char c : name)
    {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

std::unordered_map<std::string, EncodingDialect *> &registry()
{
    static std::unordered_map<std::string, EncodingDialect *> s_registry;
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
    registry()[normalize(name)] = dialect;
    lastRegistered() = dialect;
}

EncodingDialect *findEncodingDialect(std::string_view name)
{
    if (name.empty())
    {
        return nullptr;
    }
    auto &reg = registry();
    auto it = reg.find(normalize(name));
    return it == reg.end() ? nullptr : it->second;
}

EncodingDialect *getDefaultEncodingDialect() { return lastRegistered(); }

} // namespace Sema::Encoding
