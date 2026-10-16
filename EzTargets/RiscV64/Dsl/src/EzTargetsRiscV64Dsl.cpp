#include "EzTargetsRiscV64Dsl.h"

#include "CodeGenerators/RiscVEncodingCodegenBackend.h"
#include "Sema/Encoding/RiscVEncodingDialect.h"

namespace EzTargets::RiscV64
{

void registerDsl()
{
    Sema::Encoding::registerRiscVEncodingDialect();
    CodeGenerators::registerRiscVEncodingCodegenBackend();
}

} // namespace EzTargets::RiscV64
