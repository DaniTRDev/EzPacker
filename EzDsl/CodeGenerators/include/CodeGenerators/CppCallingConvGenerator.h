#ifndef EZDSL_CPP_CALLING_CONV_GENERATOR_H
#define EZDSL_CPP_CALLING_CONV_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes concrete CallingConvDesc C++ implementations from EzDSL .ezcc/.ccd definitions.
 * Emits complete ABI lowerers implementing argument/return placement, stack layout,
 * caller/callee preservation rules, and structure return (sret) policies.
 */
class CppCallingConvGenerator : public CodeGenerator
{
  public:
    CppCallingConvGenerator(DiagnosticCollector *collector,
                            SymbolTable *table,
                            std::filesystem::path outPath,
                            std::string targetName = "Target");

    bool run() override;

    const std::string &getTargetName() const noexcept { return m_targetName; }
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    void emitHeader(CppSourceEmitter &emitter) const;
    void emitSource(CppSourceEmitter &emitter) const;

  private:
    std::string m_targetName;
};

/**
 * Convenience entry point for generating calling convention descriptors.
 */
extern bool GenerateCallingConvDesc(DiagnosticCollector *collector,
                                    SymbolTable *table,
                                    std::filesystem::path outPath,
                                    std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_CALLING_CONV_GENERATOR_H
