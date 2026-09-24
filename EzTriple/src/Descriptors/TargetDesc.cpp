#include "Descriptors/TargetDesc.h"
#include "GenericCodeEmitter.h"

/**
 * Default emitter factory for targets that do not provide one.
 */
std::unique_ptr<GenericCodeEmitter> TargetDesc::createCodeEmitter()
{
    return nullptr;
}
