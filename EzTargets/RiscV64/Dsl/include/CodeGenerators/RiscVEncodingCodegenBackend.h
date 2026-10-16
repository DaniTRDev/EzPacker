#ifndef EZDSL_RISCV_ENCODING_CODEGEN_BACKEND_H
#define EZDSL_RISCV_ENCODING_CODEGEN_BACKEND_H

#include "CodeGenerators/EncodingCodegenBackend.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * RISC-V code generation backend for the generic encoding table.
 *
 * Decodes generic ENCODING directives with RISC-V semantics and emits
 * `EzTargets::RiscV64::EncodingDesc` rows.
 */
class RiscVEncodingCodegenBackend : public EncodingCodegenBackend
{
  public:
    std::string_view includeHeader() const override { return "Encoding/RiscV64EncodingDesc.h"; }
    std::string_view namespaceName() const override { return "EzTargets::RiscV64"; }
    std::string_view arrayType() const override { return "EncodingDesc"; }

    /** Serializes one instruction's encoding into an EncodingDesc brace initializer. */
    std::string row(const Symbols::TargetInstructionSymbol &sym, DiagnosticCollector *diag = nullptr) const override;
};

/** Registers the RISC-V encoding codegen backend into the registry. */
void registerRiscVEncodingCodegenBackend();

} // namespace CodeGenerators

#endif // EZDSL_RISCV_ENCODING_CODEGEN_BACKEND_H
