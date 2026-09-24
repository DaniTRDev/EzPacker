#include "EzTargetsX86_64Dsl.h"

#include "CodeGenerators/X86_64EncodingCodegenBackend.h"
#include "Sema/Encoding/X86_64EncodingDialect.h"

namespace EzTargets::X86_64
{

void registerDsl()
{
    Sema::Encoding::registerX86_64EncodingDialect();
    CodeGenerators::registerX86_64EncodingCodegenBackend();
}

} // namespace EzTargets::X86_64
