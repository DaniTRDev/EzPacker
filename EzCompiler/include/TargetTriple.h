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
    TargetTriple(std::string_view arch,
                 std::string_view vendor,
                 std::string_view sys,
                 std::string_view abi);

    static TargetTriple parse(std::string_view tripleStr);
    static TargetTriple getHostTriple();

    std::string toString() const;

    const std::string &getArch() const { return m_arch; }
    const std::string &getVendor() const { return m_vendor; }
    const std::string &getSys() const { return m_sys; }
    const std::string &getAbi() const { return m_abi; }

    bool isX86_64() const;
    bool isWindows() const;
    bool isLinux() const;
    bool isElf() const;
    bool isCoff() const;

    bool operator==(const TargetTriple &other) const;
    bool operator!=(const TargetTriple &other) const;

  private:
    std::string m_arch{ "x86_64" };
    std::string m_vendor{ "unknown" };
    std::string m_sys{ "linux" };
    std::string m_abi{ "gnu" };
};

} // namespace EzCompiler

#endif // EZPACKER_TARGET_TRIPLE_H
