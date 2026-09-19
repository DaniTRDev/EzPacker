#ifndef EZDSL_CPP_REGISTER_INFO_GENERATOR_H
#define EZDSL_CPP_REGISTER_INFO_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes the header-only {Target}RegisterInfo.h from parsed .reg definitions.
 *
 * The emitted file exposes flat, dependency-free tables that both EzTriple (bank/class
 * construction) and EzCodeEmitter (hardware-encoding lookup) can consume without creating
 * an EzCodeEmitter -> EzTriple dependency.
 */
class CppRegisterInfoGenerator : public CodeGenerator
{
  public:
    CppRegisterInfoGenerator(DiagnosticCollector *collector,
                             SymbolTable *table,
                             std::filesystem::path outPath,
                             std::string targetName = "Target");

    bool run() override;

    const std::string &getTargetName() const noexcept { return m_targetName; }
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    /**
     * Emits the complete register-info header into the given emitter.
     */
    void emitHeader(CppSourceEmitter &emitter) const;

  private:
    std::string m_targetName;
};

/**
 * Convenience entry point for generating register info headers.
 */
extern bool GenerateRegisterInfo(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 std::filesystem::path outPath,
                                 std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_REGISTER_INFO_GENERATOR_H
