#ifndef EZCORE_NAME_REGISTRY_H
#define EZCORE_NAME_REGISTRY_H

#include "EzCoreCommon.h"
#include "StringUtils.h"
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

namespace detail
{
struct TransparentStringHash
{
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept { return std::hash<std::string_view>{}(sv); }
    size_t operator()(const std::string &s) const noexcept { return std::hash<std::string_view>{}(s); }
};

struct TransparentStringEqual
{
    using is_transparent = void;
    bool operator()(std::string_view lhs, std::string_view rhs) const noexcept { return lhs == rhs; }
};
} // namespace detail

/**
 * Case-insensitive, alias-aware registry mapping names to values of T.
 *
 * Names are canonicalized with NormalizeKey (ASCII-lowercased and '-' folded to '_'), so a
 * single entry answers every spelling variant ("x86_64", "X86-64", "AMD64"). An empty name
 * never matches; find() returns a value-initialized T (nullptr for pointer registries) when
 * no entry exists.
 *
 * Uses transparent string hashing and stack buffering to perform lookups with zero heap allocations.
 * The registry is deliberately a plain value type: callers own the storage strategy.
 */
template <typename T> class NameRegistry
{
  public:
    /** Registers value under name (and every name that canonicalizes to the same key). */
    void add(std::string_view name, T value)
    {
        if (name.empty())
        {
            return;
        }

        m_entries[NormalizeKey(name)] = std::move(value);
    }

    /** Returns the value registered under name, or a value-initialized T when absent. */
    T find(std::string_view name) const
    {
        if (name.empty())
        {
            return T{};
        }

        char stackBuf[64];
        if (name.size() <= sizeof(stackBuf))
        {
            std::string_view norm = NormalizeKeyToBuffer(name, stackBuf);
            auto it = m_entries.find(norm);
            return it == m_entries.end() ? T{} : it->second;
        }

        auto it = m_entries.find(NormalizeKey(name));
        return it == m_entries.end() ? T{} : it->second;
    }

    /** Returns true when name resolves to an entry. */
    bool contains(std::string_view name) const
    {
        if (name.empty())
        {
            return false;
        }

        char stackBuf[64];
        if (name.size() <= sizeof(stackBuf))
        {
            std::string_view norm = NormalizeKeyToBuffer(name, stackBuf);
            return m_entries.contains(norm);
        }

        return m_entries.contains(NormalizeKey(name));
    }

  private:
    std::unordered_map<std::string, T, detail::TransparentStringHash, detail::TransparentStringEqual> m_entries; ///< Canonical key to registered value.
};

#endif // EZCORE_NAME_REGISTRY_H
