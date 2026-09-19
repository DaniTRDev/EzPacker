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
    /**
     * Constructs a generator that synthesizes the target descriptor for the given target.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .tdesc manifest and referenced symbols.
     * @param outPath Destination file or directory for the generated artifacts.
     * @param targetName Target identifier substituted into generated class names.
     */
    CppTargetDescGenerator(DiagnosticCollector *collector,
                           SymbolTable *table,
                           std::filesystem::path outPath,
                           std::string targetName = "Target");

    /** Generates the target descriptor header/source pair; returns false if validation or emission fails. */
    bool run() override;

    /** Returns the target identifier used to name generated classes. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated classes. */
    void setTargetName(std::string targetName) { m_targetName = std::move(targetName); }

    /** Emits the TargetDesc subclass declaration and component bindings into the header. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Emits the TargetDesc subclass constructor and register-bank/component wiring into the source. */
    void emitSource(CppSourceEmitter &emitter) const;

  private:
    /** Target identifier substituted into generated class and include names. */
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
