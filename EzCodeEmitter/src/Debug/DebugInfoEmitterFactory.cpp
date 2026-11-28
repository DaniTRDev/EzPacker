#include "Debug/IDebugInfoEmitter.h"
#include "Debug/CodeViewEmitter.h"
#include "Debug/Dwarf5Emitter.h"

namespace EzCodeEmitter::Debug
{

std::unique_ptr<IDebugInfoEmitter> createDebugInfoEmitter(EzCore::Debug::DebugFormat format, bool isCoff)
{
    switch (format)
    {
        case EzCore::Debug::DebugFormat::CodeView:
            return std::make_unique<CodeViewEmitter>();
        case EzCore::Debug::DebugFormat::Dwarf5:
            return std::make_unique<Dwarf5Emitter>();
        case EzCore::Debug::DebugFormat::Auto:
        default:
            if (isCoff)
            {
                return std::make_unique<CodeViewEmitter>();
            }
            return std::make_unique<Dwarf5Emitter>();
    }
}

} // namespace EzCodeEmitter::Debug
