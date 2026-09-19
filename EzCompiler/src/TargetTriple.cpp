#include "TargetTriple.h"
#include <sstream>
#include <algorithm>

namespace EzCompiler
{

TargetTriple::TargetTriple(std::string_view arch, std::string_view vendor, std::string_view sys, std::string_view abi) :
    m_arch(arch), m_vendor(vendor), m_sys(sys), m_abi(abi)
{
}

TargetTriple TargetTriple::parse(std::string_view tripleStr)
{
    if (tripleStr.empty())
    {
        return getHostTriple();
    }

    std::vector<std::string> parts;
    std::string current;
    for (char c : tripleStr)
    {
        if (c == '-')
        {
            if (!current.empty())
            {
                parts.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }
    if (!current.empty())
    {
        parts.push_back(current);
    }

    // Single alias overrides
    if (parts.size() == 1)
    {
        if (parts[0] == "x86_64" || parts[0] == "amd64")
        {
            return getHostTriple();
        }
        return TargetTriple(parts[0], "unknown", "none", "elf");
    }

    // Two components are interpreted as <arch>-<os/format> shorthand.
    if (parts.size() == 2)
    {
        const std::string &p0 = parts[0];
        const std::string &p1 = parts[1];

        if (p1 == "elf")
            return TargetTriple(p0, "unknown", "none", "elf");
        if (p1 == "coff")
            return TargetTriple(p0, "pc", "windows", "coff");
        if (p1 == "linux")
            return TargetTriple(p0, "unknown", "linux", "gnu");
        if (p1 == "windows" || p1 == "win32")
            return TargetTriple(p0, "pc", "windows", "msvc");

        return TargetTriple(p0, "unknown", p1, "none");
    }

    // Three components collapse to <arch>-<vendor>-<os/format>; otherwise vendor-abi are assumed.
    if (parts.size() == 3)
    {
        const std::string &p0 = parts[0];
        const std::string &p1 = parts[1];
        const std::string &p2 = parts[2];

        if (p1 == "linux" || p1 == "windows" || p1 == "none")
        {
            return TargetTriple(p0, "unknown", p1, p2);
        }
        return TargetTriple(p0, p1, p2, "none");
    }

    return TargetTriple(parts[0], parts[1], parts[2], parts[3]);
}

TargetTriple TargetTriple::getHostTriple()
{
#if defined(_WIN32) || defined(_WIN64)
    return TargetTriple("x86_64", "pc", "windows", "msvc");
#elif defined(__APPLE__)
    return TargetTriple("x86_64", "apple", "darwin", "macho");
#else
    return TargetTriple("x86_64", "unknown", "linux", "gnu");
#endif
}

std::string TargetTriple::toString() const { return std::format("{}-{}-{}-{}", m_arch, m_vendor, m_sys, m_abi); }

bool TargetTriple::isX86_64() const { return m_arch == "x86_64" || m_arch == "amd64" || m_arch == "x64"; }

bool TargetTriple::isWindows() const
{
    return m_sys == "windows" || m_sys == "win32" || m_abi == "msvc" || m_abi == "coff";
}

bool TargetTriple::isLinux() const { return m_sys == "linux"; }

bool TargetTriple::isElf() const
{
    // Fall back to ELF for any target that is neither Windows nor explicitly Mach-O.
    return m_abi == "elf" || m_abi == "gnu" || m_sys == "linux" || (!isWindows() && m_abi != "macho");
}

bool TargetTriple::isCoff() const { return isWindows(); }

bool TargetTriple::operator==(const TargetTriple &other) const
{
    return m_arch == other.m_arch && m_vendor == other.m_vendor && m_sys == other.m_sys && m_abi == other.m_abi;
}

bool TargetTriple::operator!=(const TargetTriple &other) const { return !(*this == other); }

} // namespace EzCompiler
