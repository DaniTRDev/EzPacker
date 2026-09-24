#ifndef EZDSL_CPP_ENCODING_TABLE_GENERATOR_H
#define EZDSL_CPP_ENCODING_TABLE_GENERATOR_H

#include "CodeGenerators/CodeGenerator.h"
#include "CodeGenerators/CppSourceEmitter.h"
#include "CodeGenerators/EncodingCodegenBackend.h"
#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * Synthesizes the target instruction encoding table header
 * (<Target>EncodingTable.h) from EzDSL .idf generic ENCODING blocks.
 *
 * The generator is architecture-neutral: it emits the array, count, name table
 * and lookup helpers, and delegates the runtime header, namespace, element type
 * and per-row initializer to the `EncodingCodegenBackend` selected by target
 * name. The emitted table is header-only so EzTriple, EzCodeEmitter and
 * downstream users can include it without an extra link dependency.
 */
class CppEncodingTableGenerator : public CodeGenerator
{
  public:
    /**
     * Constructs a generator that synthesizes the target instruction encoding table.
     * @param collector Receives diagnostics emitted while generating.
     * @param table Symbol table holding the parsed .idf ENCODING blocks.
     * @param outPath Destination file or directory for the generated header.
     * @param targetName Target identifier selecting the backend and naming the file.
     */
    CppEncodingTableGenerator(DiagnosticCollector *collector,
                              SymbolTable *table,
                              std::filesystem::path outPath,
                              std::string targetName = "Target");

    /** Generates the encoding table header; returns false if validation or emission fails. */
    bool run() override;

    /** Emits the header-only encoding table consumed by the runtime encoder. */
    void emitHeader(CppSourceEmitter &emitter) const;

    /** Returns the target identifier used to select the backend and name generated tables. */
    const std::string &getTargetName() const noexcept { return m_targetName; }

    /** Overrides the target identifier used to select the backend and name generated tables. */
    void setTargetName(std::string targetName)
    {
        m_targetName = SanitizeCppIdentifier(targetName, "Target");
        m_backend = findEncodingBackend(m_targetName);
    }

  private:
    /** Target identifier substituted into generated table and include names. */
    std::string m_targetName;
    /** Backend selected by target name, or nullptr when none is registered. */
    EncodingCodegenBackend *m_backend{ nullptr };
};

} // namespace CodeGenerators

#endif // EZDSL_CPP_ENCODING_TABLE_GENERATOR_H
