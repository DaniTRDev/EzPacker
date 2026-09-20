#ifndef EZDSL_SEMA_ENCODING_DIALECT_H
#define EZDSL_SEMA_ENCODING_DIALECT_H

#include "EzDslSemaCommon.h"
#include "Ast/EncodingDefLangAst.h"
#include "Ast/TargetInstDefLangAst.h"
#include <string_view>

class DiagnosticCollector;

namespace Sema::Encoding
{

/**
 * Per-target validation strategy for a generic ENCODING block.
 *
 * A dialect owns the vocabulary of its architecture: it interprets the
 * target-defined directive keys/values and validates them against the
 * instruction's declared operand signature. The shared AST and parser never
 * know about any specific ISA.
 */
class EncodingDialect
{
  public:
    virtual ~EncodingDialect() = default;

    /** Returns the canonical dialect name, e.g. "x86_64". */
    virtual std::string_view name() const = 0;

    /**
     * Validates an encoding block against its instruction.
     * Returns false and reports diagnostics when the encoding is malformed for this ISA.
     */
    virtual bool validate(const DSL::Ast::Encoding::EncodingDecl &encoding,
                          const DSL::Ast::TargetInstDef::TargetInstDecl &inst,
                          DiagnosticCollector *diag) = 0;
};

/**
 * Registers a dialect (and aliases) for lookup by name. Names are matched
 * case-insensitively after normalization.
 */
void registerEncodingDialect(std::string_view name, EncodingDialect *dialect);

/** Returns the dialect registered under (an alias of) name, or nullptr. */
EncodingDialect *findEncodingDialect(std::string_view name);

/** Returns the most recently registered dialect, used as a fallback selector. */
EncodingDialect *getDefaultEncodingDialect();

} // namespace Sema::Encoding

#endif // EZDSL_SEMA_ENCODING_DIALECT_H
