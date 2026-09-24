#ifndef EZPACKER_TARGET_TRIPLE_H
#define EZPACKER_TARGET_TRIPLE_H

#include "EzCompilerCommon.h"

namespace EzCompiler
{

/**
 * Standard target triple representing architecture, vendor, operating system, and ABI/format.
 * Follows the canonical format <arch>-<vendor>-<sys>-<abi>.
 */
class TargetTriple
{
  public:
    TargetTriple() = default;

    /**
     * Constructs a triple from its four canonical components.
     */
    TargetTriple(std::string_view arch, std::string_view vendor, std::string_view sys, std::string_view abi);

    /**
     * Parses a triple string, applying shorthand rules for 1-, 2- and 3-part forms.
     * An empty string yields the host triple.
     */
    static TargetTriple parse(std::string_view tripleStr);

    /**
     * Returns the triple describing the machine the compiler is running on.
     */
    static TargetTriple getHostTriple();

    /**
     * Reassembles the four components into the canonical <arch>-<vendor>-<sys>-<abi> form.
     */
    std::string toString() const;

    const std::string &getArch() const noexcept { return m_arch; }     ///< Architecture component (e.g. x86_64).
    const std::string &getVendor() const noexcept { return m_vendor; } ///< Vendor component (e.g. pc).
    const std::string &getSys() const noexcept { return m_sys; }       ///< Operating system component (e.g. linux).
    const std::string &getAbi() const noexcept { return m_abi; }       ///< ABI/object-format component (e.g. gnu).

    /**
     * True when the architecture names an x86-64 target (x86_64 / amd64 / x64).
     */
    bool isX86_64() const noexcept;

    /**
     * True when the triple targets Windows (sys/abi indicate win32/msvc/coff).
     */
    bool isWindows() const noexcept;

    /**
     * True when the triple targets Linux.
     */
    bool isLinux() const noexcept;

    /**
     * True when the triple should produce an ELF object file.
     */
    bool isElf() const noexcept;

    /**
     * True when the triple should produce a COFF object file.
     */
    bool isCoff() const noexcept;

    /**
     * Component-wise equality.
     */
    bool operator==(const TargetTriple &other) const noexcept;

    /**
     * Component-wise inequality.
     */
    bool operator!=(const TargetTriple &other) const noexcept;

  private:
    std::string m_arch{ "x86_64" };    ///< Architecture component.
    std::string m_vendor{ "unknown" }; ///< Vendor component.
    std::string m_sys{ "linux" };      ///< Operating system component.
    std::string m_abi{ "gnu" };        ///< ABI/object-format component.
};

} // namespace EzCompiler

#endif // EZPACKER_TARGET_TRIPLE_H
