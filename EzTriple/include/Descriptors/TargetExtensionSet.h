#ifndef EZTRIPLE_TARGET_EXTENSION_SET_H
#define EZTRIPLE_TARGET_EXTENSION_SET_H

#include "EzTripleCommon.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * Definition and metadata of an ISA extension declared for a target.
 */
struct TargetExtensionInfo
{
    std::string m_name;                 ///< Canonical lower-case name.
    std::string m_description;          ///< Human-readable description.
    bool m_isDefault{ false };          ///< Whether enabled by default in the target baseline.
    std::vector<std::string> m_implies; ///< Other extensions enabled when this extension is enabled.
};

/**
 * Manages target architecture extensions, defaults, dependencies, and enabled state.
 */
class TargetExtensionSet
{
  public:
    TargetExtensionSet() = default;

    /**
     * Registers a known extension definition for this target.
     * If the extension is marked default, it is enabled initially.
     */
    void registerExtension(std::string_view name,
                           std::string_view description = {},
                           bool isDefault = false,
                           const std::vector<std::string> &implies = {});

    /**
     * Checks if an extension is recognized and supported by this target.
     */
    bool isSupported(std::string_view name) const;

    /**
     * Checks if an extension is currently enabled on this target.
     */
    bool has(std::string_view name) const;

    /**
     * Enables an extension and recursively enables all extensions it implies.
     * Returns true if the extension was recognized, false otherwise.
     */
    bool enable(std::string_view name);

    /**
     * Disables an extension and recursively disables all extensions that depend on it.
     * Returns true if the extension was recognized, false otherwise.
     */
    bool disable(std::string_view name);

    /**
     * Enables or disables an extension.
     */
    bool set(std::string_view name, bool enabled);

    /**
     * Applies a single modifier string like "+avx", "-sse", or bare "avx".
     * Returns true on success, or false with an error message in outError if unknown.
     */
    bool applyModifier(std::string_view mod, std::string *outError = nullptr);

    /**
     * Applies a comma-separated list of modifiers, e.g. "+avx2,-sse4.1".
     */
    bool applyFeatureString(std::string_view featureSpec, std::string *outError = nullptr);

    /**
     * Applies a list of modifier strings.
     */
    bool applyFeatures(const std::vector<std::string> &features, std::string *outError = nullptr);

    /**
     * Resets all extensions to their registered default baseline states.
     */
    void resetToDefaults();

    /**
     * Clears all enabled extensions.
     */
    void clear();

    /**
     * Returns all currently enabled extension names.
     */
    std::vector<std::string> getEnabledExtensions() const;

    /**
     * Returns all registered extensions for this target.
     */
    const std::unordered_map<std::string, TargetExtensionInfo> &getKnownExtensions() const { return m_known; }

    /**
     * Normalizes an extension name to lower-case and trims surrounding whitespace or leading +/-.
     */
    static std::string normalizeName(std::string_view name);

  private:
    std::unordered_map<std::string, TargetExtensionInfo> m_known;
    std::unordered_set<std::string> m_enabled;
};

#endif // EZTRIPLE_TARGET_EXTENSION_SET_H
