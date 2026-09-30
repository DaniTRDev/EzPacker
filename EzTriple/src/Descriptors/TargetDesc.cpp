#include "Descriptors/TargetDesc.h"
#include "GenericCodeEmitter.h"
#include "Libcall/LibcallKind.h"
#include "Libcall/TargetLibcallRegistry.h"

/**
 * Default emitter factory for targets that do not provide one.
 */
std::unique_ptr<GenericCodeEmitter> TargetDesc::createCodeEmitter()
{
    return nullptr;
}

std::string_view TargetDesc::getLibcallStr(LibcallKind kind)
{
    if (auto *reg = getLibcallRegistry())
    {
        return reg->getLibcallName(kind);
    }
    return getDefaultLibcallName(kind);
}
