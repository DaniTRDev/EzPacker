#ifndef EZDSL_X86_64_ENCODING_CODEGEN_BACKEND_H
#define EZDSL_X86_64_ENCODING_CODEGEN_BACKEND_H

#include "CodeGenerators/EncodingCodegenBackend.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * x86-64 code generation backend for the generic encoding table.
 *
 * Decodes the generic ENCODING directives with x86-64 semantics and emits
 * `EzCodeEmitter::X86_64::EncodingDesc` rows.
 */
class X86_64EncodingCodegenBackend : public EncodingCodegenBackend
{
  public:
    std::string includeHeader() const override { return "X86_64/Encoding/X86_64EncodingDesc.h"; }
    std::string namespaceName() const override { return "EzCodeEmitter::X86_64"; }
    std::string arrayType() const override { return "EncodingDesc"; }

    /** Serializes one instruction's encoding into an EncodingDesc brace initializer. */
    std::string row(const Symbols::TargetInstructionSymbol &sym) const override;
};

} // namespace CodeGenerators

#endif // EZDSL_X86_64_ENCODING_CODEGEN_BACKEND_H
