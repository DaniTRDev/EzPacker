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
    /**
     * Constructs a generator that writes the register-info header for the given target.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .reg definitions.
     * @param outPath Destination file or directory for the generated header.
     * @param targetName Target identifier substituted into generated table names.
     * @param namespaceRoot Namespace root the generated table is emitted into.
     */
    CppRegisterInfoGenerator(DiagnosticCollector *collector,
                             SymbolTable *table,
                             std::filesystem::path outPath,
                             std::string targetName = "Target",
                             std::string namespaceRoot = "EzTargets");

    /** Generates the register-info header; returns false if validation or emission fails. */
    bool run() override;

    /** Returns the target identifier used to name generated tables. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated tables. */
    void setTargetName(std::string targetName) { m_targetName = SanitizeCppIdentifier(targetName, "Target"); }

    /** Overrides the namespace root the generated table is emitted into. */
    void setNamespaceRoot(std::string namespaceRoot) { m_namespaceRoot = std::move(namespaceRoot); }

  private:
    /** Target identifier substituted into generated table and include names. */
    std::string m_targetName;

    /** Namespace root the generated table is emitted into. */
    std::string m_namespaceRoot;
};

/**
 * Convenience entry point for generating register info headers.
 */
extern bool GenerateRegisterInfo(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 std::filesystem::path outPath,
                                 std::string targetName = "Target",
                                 std::string namespaceRoot = "EzTargets");

} // namespace CodeGenerators

#endif // EZDSL_CPP_REGISTER_INFO_GENERATOR_H
