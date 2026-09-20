#ifndef EZDSL_SEMA_X86_64_ENCODING_DIALECT_H
#define EZDSL_SEMA_X86_64_ENCODING_DIALECT_H

#include "Sema/Encoding/EncodingDialect.h"
#include "Ast/EncodingDefLangAst.h"
#include "Ast/TargetInstDefLangAst.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace Sema::Encoding
{

/**
 * One `operand => field` binding decoded from the generic encoding IR.
 */
struct X86OperandBindingSpec
{
    std::string_view m_operand; // Declared operand name.
    std::string_view m_field;   // x86 field name, e.g. "reg", "rm_reg", "rel32".
};

/**
 * Fully decoded x86-64 interpretation of a generic ENCODING block.
 *
 * This is the single place that knows the x86 encoding vocabulary; both the
 * semantic dialect and the code generation backend consume it.
 */
struct X86EncodingSpec
{
    bool m_hasForm{ false };
    std::string_view m_form; ///< "rr", "ri", "jcc", ...
    std::vector<uint8_t> m_opcode;
    std::optional<uint8_t> m_opcodeDigit; ///< ModR/M /digit (0..7).
    bool m_rexW{ false };                 ///< Always set REX.W.
    bool m_rexWBySize{ false };           ///< Set REX.W only when the size operand is 64-bit.
    uint8_t m_prefixes{ 0 };              ///< Legacy prefix bitmask (see runtime EncPrefix*).
    std::vector<X86OperandBindingSpec> m_operands;
    std::optional<std::string_view> m_coalesce;    ///< Two-address source operand.
    std::optional<std::string_view> m_sizeOperand; ///< Operand that determines the operation size.
    bool m_shiftByCL{ false };                     ///< Shift amount is implicitly CL.
    bool m_byteRex{ false };                       ///< Force REX on byte forms.
    std::optional<uint8_t> m_condCode;             ///< Jcc/Setcc condition digit.
    bool m_hasSseVariant{ false };                 ///< Also carries an SSE opcode form.
    uint8_t m_ssePrefixes{ 0 };
    std::vector<uint8_t> m_sseOpcode;
};

/**
 * Decodes a generic ENCODING block using x86-64 field semantics.
 *
 * When `diag` is non-null, decoding errors are reported with source locations.
 * Returns true when decoding completed without errors.
 */
bool decodeX86_64Encoding(const DSL::Ast::Encoding::EncodingDecl &encoding,
                          X86EncodingSpec &out,
                          DiagnosticCollector *diag = nullptr);

/** Maps an x86 prefix keyword ("P66", "F3", ...) to its one-bit mask, or nullopt. */
std::optional<uint8_t> x86PrefixBitFromName(std::string_view name);

/**
 * x86-64 encoding dialect: validates decoded fields against the instruction signature.
 */
class X86_64EncodingDialect : public EncodingDialect
{
  public:
    std::string_view name() const override { return "x86_64"; }

    bool validate(const DSL::Ast::Encoding::EncodingDecl &encoding,
                  const DSL::Ast::TargetInstDef::TargetInstDecl &inst,
                  DiagnosticCollector *diag) override;
};

} // namespace Sema::Encoding

#endif // EZDSL_SEMA_X86_64_ENCODING_DIALECT_H
