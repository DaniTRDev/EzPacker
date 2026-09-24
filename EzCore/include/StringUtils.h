#ifndef EZCORE_STRING_UTILS_H
#define EZCORE_STRING_UTILS_H

#include "EzCoreCommon.h"
#include <algorithm>
#include <cctype>
#include <format>

/**
 * Selects the escape table applied by EscapeString.
 */
enum class EscapeMode
{
    CppStringLiteral, ///< Escapes C++ string-literal syntax (\ and " plus the common control escapes).
    Json              ///< Escapes JSON string syntax, including \b, \f, \r and \uXXXX control sequences.
};

/**
 * Escapes value for embedding inside a quoted string of the given target syntax.
 *
 * Shared by the C++ code generators (string-literal escaping) and the CLI
 * info dumper (JSON escaping) so the two escape tables cannot drift apart.
 */
inline std::string EscapeString(std::string_view value, EscapeMode mode)
{
    std::string result;
    result.reserve(value.size() + 8);
    for (char c : value)
    {
        switch (c)
        {
            case '\\':
                result += "\\\\";
                break;
            case '"':
                result += "\\\"";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\t':
                result += "\\t";
                break;
            case '\b':
            case '\f':
            case '\r':
                // JSON has dedicated short escapes; C++ string literals keep them verbatim.
                if (mode == EscapeMode::Json)
                {
                    result += c == '\b' ? "\\b" : (c == '\f' ? "\\f" : "\\r");
                }
                else
                {
                    result.push_back(c);
                }
                break;
            default:
                if (mode == EscapeMode::Json && static_cast<unsigned char>(c) < 0x20)
                {
                    result += std::format("\\u{:04x}", static_cast<unsigned int>(static_cast<unsigned char>(c)));
                }
                else
                {
                    result.push_back(c);
                }
                break;
        }
    }
    return result;
}

/**
 * Returns true when the given character is legal inside a C++ identifier (ASCII letter, digit or '_').
 */
inline bool IsCppIdentifierChar(char c) noexcept
{
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

/**
 * Rewrites raw into a valid C++ identifier and returns it.
 *
 * Every character that cannot appear in a C++ identifier becomes '_', a leading digit is prefixed
 * with '_', and an empty result falls back to the given string. Shared by the EzDSL generators and
 * the CLI so target names such as "x86-64" always produce compilable code.
 */
inline std::string SanitizeCppIdentifier(std::string_view raw, std::string_view fallback)
{
    std::string result;
    result.reserve(raw.size());

    for (char c : raw)
    {
        result.push_back(IsCppIdentifierChar(c) ? c : '_');
    }

    if (result.empty())
    {
        result.assign(fallback);
    }

    if (std::isdigit(static_cast<unsigned char>(result.front())) != 0)
    {
        result.insert(result.begin(), '_');
    }

    return result;
}

/**
 * Builds a canonical registry key from the given name.
 *
 * Lowercases ASCII and maps '-' to '_' so case- and separator-variant spellings ("x86-64",
 * "X86_64", "AMD64") resolve to the same entry. Used by the target/dialect registries.
 */
inline std::string NormalizeKey(std::string_view key)
{
    std::string result;
    result.reserve(key.size());

    for (char c : key)
    {
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        result.push_back(lower == '-' ? '_' : lower);
    }

    return result;
}

/**
 * Converts all characters in a given string view to lowercase using ASCII transformations.
 * Allocates and returns a new std::string of the same length with lowercase characters.
 */
inline std::string StrToLower(std::string_view str)
{
    std::string lowerStr;
    lowerStr.resize(str.size());

    // Cast through unsigned char: passing a negative char to tolower is undefined behavior.
    std::transform(str.begin(), str.end(), lowerStr.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lowerStr;
}

/**
 * Converts all characters in a given string view to uppercase using ASCII transformations.
 * Allocates and returns a new std::string of the same length with uppercase characters.
 */
inline std::string StrToUpper(std::string_view str)
{
    std::string upperStr;
    upperStr.resize(str.size());

    // Cast through unsigned char: passing a negative char to toupper is undefined behavior.
    std::transform(str.begin(), str.end(), upperStr.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return upperStr;
}

#endif // EZCORE_STRING_UTILS_H
