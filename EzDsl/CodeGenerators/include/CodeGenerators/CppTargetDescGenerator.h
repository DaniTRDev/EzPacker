#ifndef EZDSL_CPP_TARGET_DESC_GENERATOR_H
#define EZDSL_CPP_TARGET_DESC_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes {Target}TargetDesc.h/.cpp from a parsed .tdesc manifest.
 *
 * The generated descriptor is a concrete TargetDesc whose register banks are built from the
 * generated {Target}RegisterInfo, whose target constants/libcall table come from the manifest,
 * and which exposes the declared object formats, calling conventions and component bindings
 * as generated metadata. Strategy components are wired through the .tdesc component bindings.
 */
class CppTargetDescGenerator : public CodeGenerator
{
  public:
    CppTargetDescGenerator(DiagnosticCollector *collector,
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
 * Convenience entry point for generating a target descriptor.
 */
extern bool GenerateTargetDescriptor(DiagnosticCollector *collector,
                                     SymbolTable *table,
                                     std::filesystem::path outPath,
                                     std::string targetName = "Target");

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_DESC_GENERATOR_H
