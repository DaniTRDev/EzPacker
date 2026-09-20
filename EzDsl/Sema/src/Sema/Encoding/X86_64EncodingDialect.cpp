#include "Sema/Encoding/X86_64EncodingDialect.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Encoding/X86_64EncodingVocabulary.h"

#include <unordered_set>

namespace Sema::Encoding
{
namespace
{
constexpr auto PassName = "Sema::Encoding::X86_64";

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

std::optional<uint8_t> x86PrefixBitFromName(std::string_view name)
{
    if (name == "P66")
        return static_cast<uint8_t>(1u << 0);
    if (name == "P67")
        return static_cast<uint8_t>(1u << 1);
    if (name == "F2")
        return static_cast<uint8_t>(1u << 2);
    if (name == "F3")
        return static_cast<uint8_t>(1u << 3);
    if (name == "F0")
        return static_cast<uint8_t>(1u << 4);
    return std::nullopt;
}

bool decodeX86_64Encoding(const AstEnc::EncodingDecl &encoding, X86EncodingSpec &out, DiagnosticCollector *diag)
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
        else if (key == "opcode" || key == "sse_opcode")
        {
            const auto *bytes = std::get_if<std::pmr::vector<uint8_t>>(&directive.m_value);
            if (!bytes)
            {
                reportDecodeError(diag, "directive '" + std::string(key) + "' expects a byte list", directive.m_key);
                ok = false;
                continue;
            }
            if (key == "opcode")
            {
                out.m_opcode.assign(bytes->begin(), bytes->end());
            }
            else
            {
                out.m_sseOpcode.assign(bytes->begin(), bytes->end());
                out.m_hasSseVariant = true;
            }
        }
        else if (key == "opcode_digit" || key == "cond")
        {
            const auto *value = std::get_if<int64_t>(&directive.m_value);
            if (!value)
            {
                reportDecodeError(diag, "directive '" + std::string(key) + "' expects an integer", directive.m_key);
                ok = false;
                continue;
            }
            if (key == "opcode_digit")
            {
                out.m_opcodeDigit = static_cast<uint8_t>(*value);
            }
            else
            {
                out.m_condCode = static_cast<uint8_t>(*value);
            }
        }
        else if (key == "rex_w" || key == "rex_w_size" || key == "shift_cl" || key == "byte_rex")
        {
            const auto *value = std::get_if<bool>(&directive.m_value);
            if (!value)
            {
                reportDecodeError(diag, "directive '" + std::string(key) + "' expects a boolean", directive.m_key);
                ok = false;
                continue;
            }
            if (key == "rex_w")
                out.m_rexW = *value;
            else if (key == "rex_w_size")
                out.m_rexWBySize = *value;
            else if (key == "shift_cl")
                out.m_shiftByCL = *value;
            else
                out.m_byteRex = *value;
        }
        else if (key == "prefixes" || key == "sse_prefix")
        {
            const auto *id = std::get_if<DSL::Ast::Common::Identifier>(&directive.m_value);
            if (!id)
            {
                reportDecodeError(diag,
                                  "directive '" + std::string(key) + "' expects a prefix identifier",
                                  directive.m_key);
                ok = false;
                continue;
            }
            const auto bit = x86PrefixBitFromName(id->m_node);
            if (!bit)
            {
                reportDecodeError(diag, "unknown x86 prefix '" + std::string(id->m_node) + "'", directive.m_key);
                ok = false;
                continue;
            }
            if (key == "prefixes")
            {
                out.m_prefixes |= *bit;
            }
            else
            {
                out.m_ssePrefixes |= *bit;
                out.m_hasSseVariant = true;
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
        else if (key == "coalesce" || key == "size")
        {
            const auto *id = std::get_if<DSL::Ast::Common::Identifier>(&directive.m_value);
            if (!id)
            {
                reportDecodeError(diag,
                                  "directive '" + std::string(key) + "' expects an operand name",
                                  directive.m_key);
                ok = false;
                continue;
            }
            if (key == "coalesce")
            {
                out.m_coalesce = id->m_node;
            }
            else
            {
                out.m_sizeOperand = id->m_node;
            }
        }
        else
        {
            reportDecodeError(diag, "unknown x86-64 encoding directive", directive.m_key);
            ok = false;
        }
    }

    return ok;
}

bool X86_64EncodingDialect::validate(const AstEnc::EncodingDecl &encoding,
                                     const DSL::Ast::TargetInstDef::TargetInstDecl &inst,
                                     DiagnosticCollector *diag)
{
    SourceReference *ref = inst.m_instName.m_sourceRef;
    const std::string_view instName = inst.m_instName.m_node;

    X86EncodingSpec spec;
    if (!decodeX86_64Encoding(encoding, spec, diag))
    {
        return false;
    }

    // Reports an error against the instruction name.
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

    if (!X86Vocab::isKnownForm(spec.m_form))
    {
        instError("ENCODING has an unknown x86-64 form '" + std::string(spec.m_form) + "'");
        return false;
    }

    if (spec.m_opcode.empty() || spec.m_opcode.size() > 3)
    {
        instError("ENCODING opcode must contain 1..3 bytes");
        return false;
    }

    if (spec.m_opcodeDigit.has_value() && spec.m_opcodeDigit.value() > 7)
    {
        instError("ENCODING opcode_digit must be in [0,7]");
        return false;
    }

    std::unordered_set<std::string_view> declaredOperands;
    for (const auto &op : inst.m_operands)
    {
        declaredOperands.insert(op.m_name.m_node);
    }

    int regCount = 0;
    int rmRegCount = 0;
    int rmMemCount = 0;
    bool hasRel = false;

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

        if (!X86Vocab::isKnownField(binding.m_field))
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

        if (binding.m_field == "reg")
            ++regCount;
        else if (binding.m_field == "rm_reg")
            ++rmRegCount;
        else if (binding.m_field == "rm_mem")
            ++rmMemCount;
        else if (binding.m_field == "rel8" || binding.m_field == "rel32")
            hasRel = true;
    }

    if (regCount > 1 || rmRegCount > 1 || rmMemCount > 1)
    {
        instError("ENCODING has more than one slot of the same register/memory kind");
        return false;
    }

    const bool isJcc = (spec.m_form == "jcc");
    const bool isSetcc = (spec.m_form == "setcc");
    const bool isBranch = (spec.m_form == "jcc" || spec.m_form == "jmp" || spec.m_form == "call");

    if (isJcc && !spec.m_condCode.has_value())
    {
        instError("Jcc ENCODING requires a condition code");
        return false;
    }

    if (isSetcc && !spec.m_condCode.has_value())
    {
        instError("SETcc ENCODING requires a condition code");
        return false;
    }

    if (isBranch && !hasRel && spec.m_form != "jmp" && spec.m_form != "call")
    {
        instError("branch ENCODING requires a rel32 operand");
        return false;
    }

    return true;
}

namespace
{
// Registers the x86-64 dialect (and legacy target-name aliases) at load time.
struct X86_64DialectRegistrar
{
    X86_64DialectRegistrar()
    {
        static X86_64EncodingDialect s_dialect;
        registerEncodingDialect("x86_64", &s_dialect);
        registerEncodingDialect("amd64", &s_dialect);
        registerEncodingDialect("x86-64", &s_dialect);
    }
};

const X86_64DialectRegistrar s_registrar;
} // namespace

} // namespace Sema::Encoding
