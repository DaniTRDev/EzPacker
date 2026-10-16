#ifndef EZDSL_SEMA_RISCV_ENCODING_DIALECT_H
#define EZDSL_SEMA_RISCV_ENCODING_DIALECT_H

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
struct RiscVOperandBindingSpec
{
    std::string_view m_operand; // Declared operand name.
    std::string_view m_field;   // RISC-V field name, e.g. "rd", "rs1", "rs2", "imm12_signed", "rel".
};

/**
 * Fully decoded RISC-V interpretation of a generic ENCODING block.
 */
struct RiscVEncodingSpec
{
    bool m_hasForm{ false };
    std::string_view m_form; ///< "r_type", "i_type", "s_type", "b_type", "u_type", "j_type", "pseudo"
    bool m_hasOpcode{ false };
    uint8_t m_opcode{ 0 };
    bool m_hasFunct3{ false };
    uint8_t m_funct3{ 0 };
    bool m_hasFunct7{ false };
    uint8_t m_funct7{ 0 };
    std::vector<RiscVOperandBindingSpec> m_operands;
    std::optional<uint8_t> m_funct2;
    bool m_aq{ false };
    bool m_rl{ false };
};

/**
 * Decodes a generic ENCODING block using RISC-V field semantics.
 */
bool decodeRiscVEncoding(const DSL::Ast::Encoding::EncodingDecl &encoding,
                         RiscVEncodingSpec &out,
                         DiagnosticCollector *diag = nullptr);

/**
 * RISC-V encoding dialect: validates decoded fields against the instruction signature.
 */
class RiscVEncodingDialect : public EncodingDialect
{
  public:
    std::string_view name() const override { return "riscv64"; }

    bool validate(const DSL::Ast::Encoding::EncodingDecl &encoding,
                  const DSL::Ast::TargetInstDef::TargetInstDecl &inst,
                  DiagnosticCollector *diag) override;
};

/** Registers the RISC-V encoding dialect (and aliases) into the registry. */
void registerRiscVEncodingDialect();

} // namespace Sema::Encoding

#endif // EZDSL_SEMA_RISCV_ENCODING_DIALECT_H
