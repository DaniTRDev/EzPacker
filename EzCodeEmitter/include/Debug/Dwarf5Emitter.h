#ifndef EZPACKER_DWARF5EMITTER_H
#define EZPACKER_DWARF5EMITTER_H

#include "Debug/IDebugInfoEmitter.h"

#include <cstdint>
#include <vector>

namespace EzCodeEmitter::Debug
{

/**
 * Serializes a DebugCompileUnit into DWARF v5 ELF64 debug sections
 * (.debug_str, .debug_line_str, .debug_abbrev, .debug_info, .debug_line, .debug_frame).
 */
class Dwarf5Emitter : public IDebugInfoEmitter
{
  public:
    Dwarf5Emitter() = default;
    ~Dwarf5Emitter() override = default;

    /**
     * Emits all DWARF v5 extra object sections for the given compile unit.
     * Returns an empty vector when cu.level == DebugLevel::None.
     */
    std::vector<ObjectFormat::ExtraObjectSection> emit(const EzCore::Debug::DebugCompileUnit &cu) override;

    /**
     * Appends an unsigned LEB128-encoded 64-bit integer to out.
     */
    static void appendUleb128(std::vector<uint8_t> &out, uint64_t value);

    /**
     * Appends a signed LEB128-encoded 64-bit integer to out.
     */
    static void appendSleb128(std::vector<uint8_t> &out, int64_t value);
};

} // namespace EzCodeEmitter::Debug

#endif // EZPACKER_DWARF5EMITTER_H
