#ifndef EZTRIPLE_TARGET_LIBCALL_REGISTRY_H
#define EZTRIPLE_TARGET_LIBCALL_REGISTRY_H

#include "EzTripleCommon.h"
#include "Instruction/MirInstructionSet.h"
#include "Libcall/LibcallKind.h"
#include "Libcall/LibcallSignature.h"

#include <bitset>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class CallingConvDesc;
class MirType;

/**
 * Manages target runtime library functions, symbol mappings, calling conventions, and ABI availability.
 * Provides target descriptors with fine-grained control over compiler-rt and C-rt functions.
 */
class TargetLibcallRegistry
{
  public:
    TargetLibcallRegistry();
    virtual ~TargetLibcallRegistry() = default;

    /**
     * Initializes default symbol mappings, availability, and calling conventions for the given
     * architecture and OS environment.
     */
    void initDefaults(std::string_view arch, std::string_view os, CrtFlavor crtFlavor = CrtFlavor::Gnu);

    /**
     * Returns the symbol name for the given libcall, consulting target overrides first and falling
     * back to the CRT flavor's default name.
     */
    std::string_view getLibcallName(LibcallKind kind) const;

    /**
     * Sets a target-specific override for the runtime symbol name of a LibcallKind.
     */
    void setLibcallName(LibcallKind kind, std::string_view name);

    /**
     * Returns the calling convention configured for the given libcall, or nullptr if it should
     * follow the target's standard C calling convention.
     */
    CallingConvDesc *getCallingConvention(LibcallKind kind) const;

    /**
     * Assigns a specific calling convention to a libcall.
     */
    void setCallingConvention(LibcallKind kind, CallingConvDesc *cc);

    /**
     * Returns true if the libcall is supported and available on this target/environment.
     */
    bool isAvailable(LibcallKind kind) const;

    /**
     * Sets whether a given libcall is available.
     */
    void setAvailable(LibcallKind kind, bool available);

    /**
     * Controls whether the target operates in hosted mode (full C-rt available) or freestanding
     * mode (only compiler-rt and freestanding memory builtins).
     */
    void setHosted(bool isHosted);
    bool isHosted() const { return m_isHosted; }

    /**
     * Returns the C runtime flavor configured for this registry.
     */
    CrtFlavor getCrtFlavor() const { return m_crtFlavor; }
    void setCrtFlavor(CrtFlavor flavor);

    /**
     * Registers a custom runtime function with an optional signature, returning a unique ID.
     */
    uint16_t registerCustomLibcall(std::string_view name, LibcallSignature sig = {});

    /**
     * Resolves a custom libcall ID back to its symbol name.
     */
    std::string_view getCustomLibcallName(uint16_t id) const;

    /**
     * Looks up a standard LibcallKind by its default or overridden symbol name.
     */
    std::optional<LibcallKind> findKindByName(std::string_view name) const;

    /**
     * Infers the canonical LibcallKind for a generic opcode and type combination (e.g. SDIV + i128 -> DivI128).
     */
    std::optional<LibcallKind> findKindForOpcode(MirInstructionOpCode op, MirType *type) const;

  private:
    CrtFlavor m_crtFlavor{ CrtFlavor::Gnu };
    bool m_isHosted{ true };
    std::bitset<static_cast<size_t>(LibcallKind::COUNT)> m_availability;
    std::unordered_map<LibcallKind, std::string> m_symbolOverrides;
    std::unordered_map<LibcallKind, CallingConvDesc *> m_callingConvs;
    std::vector<std::pair<std::string, LibcallSignature>> m_customLibcalls;
};

#endif // EZTRIPLE_TARGET_LIBCALL_REGISTRY_H
