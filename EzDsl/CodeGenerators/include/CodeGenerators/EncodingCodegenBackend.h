#ifndef EZDSL_ENCODING_CODEGEN_BACKEND_H
#define EZDSL_ENCODING_CODEGEN_BACKEND_H

#include "EzDslCodeGeneratorsCommon.h"
#include "Sema/Symbols/TargetSymbols.h"
#include <string>
#include <string_view>

namespace CodeGenerators
{

/**
 * Per-target code generation strategy for the generic encoding table.
 *
 * The generic `CppEncodingTableGenerator` emits the boilerplate (table array,
 * count, name table and lookup helpers) and delegates the architecture-specific
 * parts — the runtime header to include, the namespace, the array element type
 * and each row's initializer — to a backend. Adding an ISA means adding a
 * backend, never editing the generic generator.
 */
class EncodingCodegenBackend
{
  public:
    virtual ~EncodingCodegenBackend() = default;

    /** Runtime header that defines the array element type, e.g. "X86_64/Encoding/X86_64EncodingDesc.h". */
    virtual std::string includeHeader() const = 0;

    /** C++ namespace the generated table lives in. */
    virtual std::string namespaceName() const = 0;

    /** Array element type name, e.g. "EncodingDesc". */
    virtual std::string arrayType() const = 0;

    /** Initializer for one table row (including the type and braces). */
    virtual std::string row(const Symbols::TargetInstructionSymbol &sym) const = 0;
};

/** Registers a backend under a name (matched case-insensitively); used for aliases too. */
void registerEncodingBackend(std::string_view name, EncodingCodegenBackend *backend);

/** Returns the backend registered under (an alias of) name, or nullptr. */
EncodingCodegenBackend *findEncodingBackend(std::string_view name);

} // namespace CodeGenerators

#endif // EZDSL_ENCODING_CODEGEN_BACKEND_H
