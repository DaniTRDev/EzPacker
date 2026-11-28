#ifndef EZPACKER_CODEVIEWEMITTER_H
#define EZPACKER_CODEVIEWEMITTER_H

#include "Debug/IDebugInfoEmitter.h"

namespace EzCodeEmitter::Debug
{

/**
 * Serializes a DebugCompileUnit into Microsoft CodeView CV8 C13 PE/COFF debug sections
 * (.debug$T and .debug$S).
 */
class CodeViewEmitter : public IDebugInfoEmitter
{
  public:
    CodeViewEmitter() = default;
    ~CodeViewEmitter() override = default;

    /**
     * Emits .debug$T and .debug$S extra object sections for the given compile unit.
     * Returns an empty vector when cu.level == DebugLevel::None.
     */
    std::vector<ObjectFormat::ExtraObjectSection> emit(const EzCore::Debug::DebugCompileUnit &cu) override;
};

} // namespace EzCodeEmitter::Debug

#endif // EZPACKER_CODEVIEWEMITTER_H
