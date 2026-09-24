#include "Descriptors/TargetExtensionSet.h"
#include <algorithm>
#include <cctype>
#include <format>
#include <sstream>

std::string TargetExtensionSet::normalizeName(std::string_view name)
{
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.front())))
    {
        name.remove_prefix(1);
    }
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back())))
    {
        name.remove_suffix(1);
    }
    if (!name.empty() && (name.front() == '+' || name.front() == '-'))
    {
        name.remove_prefix(1);
    }
    std::string result;
    result.reserve(name.size());
    for (char c : name)
    {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
}

void TargetExtensionSet::registerExtension(std::string_view name,
                                           std::string_view description,
                                           bool isDefault,
                                           const std::vector<std::string> &implies)
{
    std::string canonical = normalizeName(name);
    if (canonical.empty())
    {
        return;
    }

    std::vector<std::string> normalizedImplies;
    normalizedImplies.reserve(implies.size());
    for (const auto &imp : implies)
    {
        std::string normImp = normalizeName(imp);
        if (!normImp.empty())
        {
            normalizedImplies.push_back(std::move(normImp));
        }
    }

    TargetExtensionInfo info;
    info.m_name = canonical;
    info.m_description = std::string(description);
    info.m_isDefault = isDefault;
    info.m_implies = std::move(normalizedImplies);

    m_known[canonical] = std::move(info);

    if (isDefault)
    {
        enable(canonical);
    }
}

bool TargetExtensionSet::isSupported(std::string_view name) const
{
    std::string canonical = normalizeName(name);
    if (m_known.empty())
    {
        return true;
    }
    return m_known.find(canonical) != m_known.end();
}

bool TargetExtensionSet::has(std::string_view name) const
{
    std::string canonical = normalizeName(name);
    return m_enabled.find(canonical) != m_enabled.end();
}

bool TargetExtensionSet::enable(std::string_view name)
{
    std::string canonical = normalizeName(name);
    if (canonical.empty())
    {
        return false;
    }

    if (!m_known.empty() && m_known.find(canonical) == m_known.end())
    {
        return false;
    }

    m_enabled.insert(canonical);

    // Recursively enable implied extensions
    auto it = m_known.find(canonical);
    if (it != m_known.end())
    {
        for (const auto &imp : it->second.m_implies)
        {
            enable(imp);
        }
    }

    return true;
}

bool TargetExtensionSet::disable(std::string_view name)
{
    std::string canonical = normalizeName(name);
    if (canonical.empty())
    {
        return false;
    }

    if (!m_known.empty() && m_known.find(canonical) == m_known.end())
    {
        return false;
    }

    m_enabled.erase(canonical);

    // Recursively disable any known extension that implies this one
    for (const auto &[k, info] : m_known)
    {
        for (const auto &imp : info.m_implies)
        {
            if (imp == canonical && has(k))
            {
                disable(k);
                break;
            }
        }
    }

    return true;
}

bool TargetExtensionSet::set(std::string_view name, bool enabled)
{
    return enabled ? enable(name) : disable(name);
}

bool TargetExtensionSet::applyModifier(std::string_view mod, std::string *outError)
{
    while (!mod.empty() && std::isspace(static_cast<unsigned char>(mod.front())))
    {
        mod.remove_prefix(1);
    }
    while (!mod.empty() && std::isspace(static_cast<unsigned char>(mod.back())))
    {
        mod.remove_suffix(1);
    }
    if (mod.empty())
    {
        return true;
    }

    bool enableFeature = true;
    if (mod.front() == '-')
    {
        enableFeature = false;
        mod.remove_prefix(1);
    }
    else if (mod.front() == '+')
    {
        enableFeature = true;
        mod.remove_prefix(1);
    }

    std::string canonical = normalizeName(mod);
    if (canonical.empty())
    {
        return true;
    }

    if (!isSupported(canonical))
    {
        if (outError)
        {
            *outError = std::format("unknown target extension '{}'", canonical);
        }
        return false;
    }

    set(canonical, enableFeature);
    return true;
}

bool TargetExtensionSet::applyFeatureString(std::string_view featureSpec, std::string *outError)
{
    std::string_view remaining = featureSpec;
    while (!remaining.empty())
    {
        auto commaPos = remaining.find(',');
        std::string_view token = (commaPos == std::string_view::npos) ? remaining : remaining.substr(0, commaPos);
        if (!applyModifier(token, outError))
        {
            return false;
        }
        if (commaPos == std::string_view::npos)
        {
            break;
        }
        remaining = remaining.substr(commaPos + 1);
    }
    return true;
}

bool TargetExtensionSet::applyFeatures(const std::vector<std::string> &features, std::string *outError)
{
    for (const auto &feat : features)
    {
        if (!applyFeatureString(feat, outError))
        {
            return false;
        }
    }
    return true;
}

void TargetExtensionSet::resetToDefaults()
{
    m_enabled.clear();
    for (const auto &[k, info] : m_known)
    {
        if (info.m_isDefault)
        {
            enable(k);
        }
    }
}

void TargetExtensionSet::clear()
{
    m_enabled.clear();
}

std::vector<std::string> TargetExtensionSet::getEnabledExtensions() const
{
    return std::vector<std::string>(m_enabled.begin(), m_enabled.end());
}
