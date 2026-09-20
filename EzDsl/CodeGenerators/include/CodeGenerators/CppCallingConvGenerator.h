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
    /**
     * Constructs a generator that synthesizes calling convention descriptors for the given target.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed calling convention definitions.
     * @param outPath Destination file or directory for the generated artifacts.
     * @param targetName Target identifier substituted into generated class names.
     */
    CppCallingConvGenerator(DiagnosticCollector *collector,
                            SymbolTable *table,
                            std::filesystem::path outPath,
                            std::string targetName = "Target");

    /** Generates the descriptor header/source pair; returns false if validation or emission fails. */
    bool run() override;

    /** Returns the target identifier used to name generated classes. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to name generated classes. */
    void setTargetName(std::string targetName) { m_targetName = SanitizeCppIdentifier(targetName, "Target"); }

    /** Emits the CallingConvDesc class declarations and inline helpers into the header. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Emits the CallingConvDesc out-of-line method definitions into the source. */
    void emitSource(CppSourceEmitter &emitter) const;

  private:
    /** Target identifier substituted into generated class and include names. */
    std::string m_targetName;
};

} // namespace CodeGenerators

#endif // EZDSL_CPP_CALLING_CONV_GENERATOR_H
