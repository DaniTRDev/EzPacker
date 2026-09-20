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
 * `EzTargets::X86_64::EncodingDesc` rows.
 */
class X86_64EncodingCodegenBackend : public EncodingCodegenBackend
{
  public:
    std::string_view includeHeader() const override { return "Encoding/X86_64EncodingDesc.h"; }
    std::string_view namespaceName() const override { return "EzTargets::X86_64"; }
    std::string_view arrayType() const override { return "EncodingDesc"; }

    /** Serializes one instruction's encoding into an EncodingDesc brace initializer. */
    std::string row(const Symbols::TargetInstructionSymbol &sym, DiagnosticCollector *diag = nullptr) const override;
};

} // namespace CodeGenerators

#endif // EZDSL_X86_64_ENCODING_CODEGEN_BACKEND_H
