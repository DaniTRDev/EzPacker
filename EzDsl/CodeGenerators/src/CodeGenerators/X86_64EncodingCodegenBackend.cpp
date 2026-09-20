#include "CodeGenerators/X86_64EncodingCodegenBackend.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Encoding/X86_64EncodingDialect.h"
#include "Sema/Encoding/X86_64EncodingVocabulary.h"

#include <format>
#include <string>
#include <vector>

namespace CodeGenerators
{
namespace
{

// Classifies a DSL operand type/class name into the generated encoder register class.
const char *regClassToString(std::string_view type)
{
    if (type.rfind("FPR", 0) == 0)
    {
        return "EncRegClass::FPR";
    }
    if (type.rfind("GPR", 0) == 0 || type.rfind("Mem", 0) == 0)
    {
        return "EncRegClass::GPR";
    }
    return "EncRegClass::Any";
}

// Renders a byte vector as a C++ brace-initializer list of 0xNN literals.
std::string toByteList(const std::vector<uint8_t> &bytes)
{
    std::string result = "{ ";
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        if (i > 0)
        {
            result += ", ";
        }
        result += std::format("0x{:02X}", bytes[i]);
    }
    while (!result.empty() && result.back() == ' ')
    {
        result.pop_back();
    }
    result += " }";
    return result;
}

// Serializes a decoded x86 encoding spec into an EncodingDesc brace initializer.
std::string emitEncodingDesc(const Symbols::TargetInstructionSymbol &data, const Sema::Encoding::X86EncodingSpec &enc)
{
    // Resolve an operand name to its index within the instruction signature.
    auto operandIndex = [&](std::string_view name) -> uint8_t
    {
        for (size_t i = 0; i < data.m_operands.size(); ++i)
        {
            if (data.m_operands[i].m_name == name)
            {
                return static_cast<uint8_t>(i);
            }
        }
        return 0xFF;
    };

    std::string operands = "{ ";
    for (size_t i = 0; i < enc.m_operands.size(); ++i)
    {
        const auto &binding = enc.m_operands[i];
        uint8_t index = operandIndex(binding.m_operand);
        std::string_view type =
                (index < data.m_operands.size()) ? data.m_operands[index].m_regClassOrType : std::string_view{};
        if (i > 0)
        {
            operands += ", ";
        }
        operands += std::format("EncOperandBinding{{ {}, {}, {} }}",
                                Sema::Encoding::X86Vocab::slotEnum(binding.m_field),
                                index,
                                regClassToString(type));
    }
    if (enc.m_operands.empty())
    {
        operands += "EncOperandBinding{}";
    }
    operands += " }";

    // The size operand is optional and supplies the operation width at emission time.
    uint8_t sizeOperand = 0xFF;
    if (enc.m_sizeOperand.has_value())
    {
        sizeOperand = operandIndex(enc.m_sizeOperand.value());
    }
    // Coalescing sources the encoding's register from another operand when the two must match.
    uint8_t coalesceSrc = 0xFF;
    if (enc.m_coalesce.has_value())
    {
        coalesceSrc = operandIndex(enc.m_coalesce.value());
    }

    // Any rel8/rel32 field is the branch target that needs relocation.
    uint8_t relocOperand = 0xFF;
    for (const auto &binding : enc.m_operands)
    {
        if (binding.m_field == "rel32" || binding.m_field == "rel8")
        {
            relocOperand = operandIndex(binding.m_operand);
            break;
        }
    }

    // 0 = never REX.W, 1 = always REX.W, 2 = infer from operand size.
    uint8_t rexPolicy = enc.m_rexWBySize ? 2 : (enc.m_rexW ? 1 : 0);

    return std::format("EncodingDesc{{"
                       ".m_form = {}, .m_prefixes = {}, .m_rexW = {}, .m_digit = {}, "
                       ".m_opcode = {}, .m_opcodeLen = {}, .m_operandCount = {}, .m_operands = {}, "
                       ".m_sizeOperand = {}, .m_coalesceSrc = {}, .m_relocOperand = {}, "
                       ".m_shiftByCL = {}, .m_movabs = {}, .m_byteRex = {}, .m_condCode = {}, "
                       ".m_hasSseVariant = {}, .m_ssePrefixes = {}, .m_sseOpcode = {}, .m_sseOpcodeLen = {} }}",
                       Sema::Encoding::X86Vocab::formEnum(enc.m_form),
                       static_cast<unsigned>(enc.m_prefixes),
                       rexPolicy,
                       enc.m_opcodeDigit.has_value() ? static_cast<unsigned>(enc.m_opcodeDigit.value()) : 0xFFu,
                       toByteList(enc.m_opcode),
                       static_cast<unsigned>(enc.m_opcode.size()),
                       static_cast<unsigned>(enc.m_operands.size()),
                       operands,
                       static_cast<unsigned>(sizeOperand),
                       static_cast<unsigned>(coalesceSrc),
                       static_cast<unsigned>(relocOperand),
                       enc.m_shiftByCL ? "true" : "false",
                       (enc.m_form == "movri") ? "true" : "false",
                       enc.m_byteRex ? "true" : "false",
                       enc.m_condCode.has_value() ? static_cast<unsigned>(enc.m_condCode.value()) : 0u,
                       enc.m_hasSseVariant ? "true" : "false",
                       static_cast<unsigned>(enc.m_ssePrefixes),
                       toByteList(enc.m_sseOpcode),
                       static_cast<unsigned>(enc.m_sseOpcode.size()));
}

// Registers the x86-64 backend (and legacy target-name aliases) at load time.
struct X86_64BackendRegistrar
{
    X86_64BackendRegistrar()
    {
        static X86_64EncodingCodegenBackend s_backend;
        registerEncodingBackend("x86_64", &s_backend);
        registerEncodingBackend("amd64", &s_backend);
        registerEncodingBackend("x86-64", &s_backend);
    }
};

const X86_64BackendRegistrar s_registrar;

} // namespace

std::string X86_64EncodingCodegenBackend::row(const Symbols::TargetInstructionSymbol &sym,
                                              DiagnosticCollector *diag) const
{
    if (!sym.m_encoding.has_value())
    {
        return "EncodingDesc{}";
    }

    Sema::Encoding::X86EncodingSpec spec;
    if (!Sema::Encoding::decodeX86_64Encoding(sym.m_encoding.value(), spec, diag))
    {
        return "EncodingDesc{}";
    }

    return emitEncodingDesc(sym, spec);
}

} // namespace CodeGenerators
