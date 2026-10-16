#include "Sema/Encoding/RiscVEncodingDialect.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Encoding/RiscVEncodingVocabulary.h"

#include <unordered_set>

namespace Sema::Encoding
{
namespace
{
constexpr auto PassName = "Sema::Encoding::RiscV";

namespace AstEnc = DSL::Ast::Encoding;

// Reports a decode error against a directive's key span when diagnostics are enabled.
void reportDecodeError(DiagnosticCollector *diag, std::string_view message, const DSL::Ast::Common::Identifier &key)
{
    if (diag)
    {
        diag->error(PassName, "{}: {}", message, key.m_node) << key.m_sourceRef;
    }
}

} // namespace

bool decodeRiscVEncoding(const AstEnc::EncodingDecl &encoding, RiscVEncodingSpec &out, DiagnosticCollector *diag)
{
    out = {};
    bool ok = true;

    for (const auto &directive : encoding.m_directives)
    {
        const std::string_view key = directive.m_key.m_node;

        if (key == "form")
        {
            if (const auto *id = std::get_if<DSL::Ast::Common::Identifier>(&directive.m_value))
            {
                out.m_form = id->m_node;
                out.m_hasForm = true;
            }
            else
            {
                reportDecodeError(diag, "directive 'form' expects an identifier", directive.m_key);
                ok = false;
            }
        }
        else if (key == "opcode")
        {
            if (const auto *bytes = std::get_if<std::pmr::vector<uint8_t>>(&directive.m_value))
            {
                if (bytes->empty())
                {
                    reportDecodeError(diag, "directive 'opcode' byte list cannot be empty", directive.m_key);
                    ok = false;
                }
                else
                {
                    out.m_opcode = bytes->front();
                    out.m_hasOpcode = true;
                }
            }
            else if (const auto *val = std::get_if<int64_t>(&directive.m_value))
            {
                out.m_opcode = static_cast<uint8_t>(*val & 0x7F);
                out.m_hasOpcode = true;
            }
            else
            {
                reportDecodeError(diag, "directive 'opcode' expects a byte list or integer", directive.m_key);
                ok = false;
            }
        }
        else if (key == "funct3")
        {
            if (const auto *val = std::get_if<int64_t>(&directive.m_value))
            {
                out.m_funct3 = static_cast<uint8_t>(*val & 0x7);
                out.m_hasFunct3 = true;
            }
            else
            {
                reportDecodeError(diag, "directive 'funct3' expects an integer in [0, 7]", directive.m_key);
                ok = false;
            }
        }
        else if (key == "funct7")
        {
            if (const auto *val = std::get_if<int64_t>(&directive.m_value))
            {
                out.m_funct7 = static_cast<uint8_t>(*val & 0x7F);
                out.m_hasFunct7 = true;
            }
            else
            {
                reportDecodeError(diag, "directive 'funct7' expects an integer in [0, 127]", directive.m_key);
                ok = false;
            }
        }
        else if (key == "funct2")
        {
            if (const auto *val = std::get_if<int64_t>(&directive.m_value))
            {
                out.m_funct2 = static_cast<uint8_t>(*val & 0x3);
            }
            else
            {
                reportDecodeError(diag, "directive 'funct2' expects an integer in [0, 3]", directive.m_key);
                ok = false;
            }
        }
        else if (key == "aq" || key == "rl")
        {
            if (const auto *b = std::get_if<bool>(&directive.m_value))
            {
                if (key == "aq")
                    out.m_aq = *b;
                else
                    out.m_rl = *b;
            }
            else if (const auto *val = std::get_if<int64_t>(&directive.m_value))
            {
                if (key == "aq")
                    out.m_aq = (*val != 0);
                else
                    out.m_rl = (*val != 0);
            }
            else
            {
                reportDecodeError(diag, "directive '" + std::string(key) + "' expects a boolean", directive.m_key);
                ok = false;
            }
        }
        else if (key == "operands")
        {
            const auto *bindings = std::get_if<std::pmr::vector<AstEnc::OperandBinding>>(&directive.m_value);
            if (!bindings)
            {
                reportDecodeError(diag, "directive 'operands' expects an operand binding block", directive.m_key);
                ok = false;
                continue;
            }
            for (const auto &binding : *bindings)
            {
                out.m_operands.push_back({ binding.m_operand.m_node, binding.m_field.m_node });
            }
        }
        else
        {
            reportDecodeError(diag, "unknown RISC-V encoding directive", directive.m_key);
            ok = false;
        }
    }

    return ok;
}

bool RiscVEncodingDialect::validate(const AstEnc::EncodingDecl &encoding,
                                    const DSL::Ast::TargetInstDef::TargetInstDecl &inst,
                                    DiagnosticCollector *diag)
{
    SourceReference *ref = inst.m_instName.m_sourceRef;
    const std::string_view instName = inst.m_instName.m_node;

    RiscVEncodingSpec spec;
    if (!decodeRiscVEncoding(encoding, spec, diag))
    {
        return false;
    }

    auto instError = [&](std::string_view what)
    {
        if (diag)
        {
            diag->error(PassName, "Target instruction '{}': {}", instName, what) << ref;
        }
    };

    if (!spec.m_hasForm)
    {
        instError("ENCODING requires a valid form");
        return false;
    }

    if (!RiscVVocab::isKnownForm(spec.m_form))
    {
        instError("ENCODING has an unknown RISC-V form '" + std::string(spec.m_form) + "'");
        return false;
    }

    if (!spec.m_hasOpcode && spec.m_form != "pseudo")
    {
        instError("ENCODING requires an opcode");
        return false;
    }

    std::unordered_set<std::string_view> declaredOperands;
    for (const auto &op : inst.m_operands)
    {
        declaredOperands.insert(op.m_name.m_node);
    }

    int rdCount = 0;
    int rs1Count = 0;
    int rs2Count = 0;
    int rs3Count = 0;

    for (const auto &binding : spec.m_operands)
    {
        if (!declaredOperands.contains(binding.m_operand))
        {
            if (diag)
            {
                diag->error(PassName,
                            "Target instruction '{}': ENCODING binds unknown operand '{}'",
                            instName,
                            binding.m_operand)
                        << ref;
            }
            return false;
        }

        if (!RiscVVocab::isKnownField(binding.m_field))
        {
            if (diag)
            {
                diag->error(PassName,
                            "Target instruction '{}': ENCODING uses unknown field '{}'",
                            instName,
                            binding.m_field)
                        << ref;
            }
            return false;
        }

        if (binding.m_field == "rd")
            ++rdCount;
        else if (binding.m_field == "rs1")
            ++rs1Count;
        else if (binding.m_field == "rs2")
            ++rs2Count;
        else if (binding.m_field == "rs3")
            ++rs3Count;
    }

    if (rdCount > 1 || rs1Count > 1 || rs2Count > 1 || rs3Count > 1)
    {
        instError("ENCODING has duplicate register slot bindings");
        return false;
    }

    return true;
}

namespace
{
struct RiscVDialectRegistrar
{
    RiscVDialectRegistrar() { registerRiscVEncodingDialect(); }
};

const RiscVDialectRegistrar s_registrar;
} // namespace

void registerRiscVEncodingDialect()
{
    static RiscVEncodingDialect s_dialect;
    registerEncodingDialect("riscv", &s_dialect);
    registerEncodingDialect("riscv64", &s_dialect);
    registerEncodingDialect("risc-v", &s_dialect);
    registerEncodingDialect("riscv32", &s_dialect);
}

} // namespace Sema::Encoding
