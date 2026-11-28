#ifndef EZPACKER_IDEBUGINFOEMITTER_H
#define EZPACKER_IDEBUGINFOEMITTER_H

#include "Debug/DebugInfo.h"
#include "ObjectFormat/ObjectSymbol.h"
#include <memory>
#include <vector>

namespace EzCodeEmitter::Debug
{

/**
 * Format-independent interface for serializing a DebugCompileUnit into raw
 * object-file extra sections (e.g., DWARF 5 .debug_* or PE/COFF CodeView .debug$S/.debug$T).
 */
class IDebugInfoEmitter
{
  public:
    virtual ~IDebugInfoEmitter() = default;

    /**
     * Serializes the given compile unit's debug information into extra object sections
     * ready to be registered with an IObjectWriter.
     */
    virtual std::vector<ObjectFormat::ExtraObjectSection> emit(const EzCore::Debug::DebugCompileUnit &cu) = 0;
};

/**
 * Creates a debug information emitter for the requested format and object container.
 * When format is DebugFormat::Auto, selects CodeView for COFF targets and DWARF 5 otherwise.
 */
std::unique_ptr<IDebugInfoEmitter> createDebugInfoEmitter(EzCore::Debug::DebugFormat format, bool isCoff);

} // namespace EzCodeEmitter::Debug

#endif // EZPACKER_IDEBUGINFOEMITTER_H
