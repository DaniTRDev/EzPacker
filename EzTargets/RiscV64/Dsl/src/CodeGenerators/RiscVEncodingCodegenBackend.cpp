#include "CodeGenerators/RiscVEncodingCodegenBackend.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Encoding/RiscVEncodingDialect.h"
#include "Sema/Encoding/RiscVEncodingVocabulary.h"

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

// Serializes a decoded RISC-V encoding spec into an EncodingDesc brace initializer.
std::string emitEncodingDesc(const Symbols::TargetInstructionSymbol &data, const Sema::Encoding::RiscVEncodingSpec &enc)
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
                                Sema::Encoding::RiscVVocab::slotEnum(binding.m_field),
                                index,
                                regClassToString(type));
    }
    if (enc.m_operands.empty())
    {
        operands += "EncOperandBinding{}";
    }
    operands += " }";

    // Any imm12_branch, imm20_jump, or rel field is the branch/jump/relocation target.
    uint8_t relocOperand = 0xFF;
    for (const auto &binding : enc.m_operands)
    {
        if (binding.m_field == "imm12_branch" || binding.m_field == "imm20_jump" || binding.m_field == "rel")
        {
            relocOperand = operandIndex(binding.m_operand);
            break;
        }
    }

    return std::format("EncodingDesc{{ "
                       ".m_form = {}, .m_opcode = 0x{:02X}, .m_funct3 = 0x{:02X}, .m_funct7 = 0x{:02X}, "
                       ".m_operandCount = {}, .m_operands = {}, .m_relocOperand = {} }}",
                       Sema::Encoding::RiscVVocab::formEnum(enc.m_form),
                       static_cast<unsigned>(enc.m_opcode),
                       static_cast<unsigned>(enc.m_funct3),
                       static_cast<unsigned>(enc.m_funct7),
                       static_cast<unsigned>(enc.m_operands.size()),
                       operands,
                       static_cast<unsigned>(relocOperand));
}

struct RiscVBackendRegistrar
{
    RiscVBackendRegistrar() { registerRiscVEncodingCodegenBackend(); }
};

const RiscVBackendRegistrar s_registrar;

} // namespace

void registerRiscVEncodingCodegenBackend()
{
    static RiscVEncodingCodegenBackend s_backend;
    registerEncodingBackend("riscv", &s_backend);
    registerEncodingBackend("riscv64", &s_backend);
    registerEncodingBackend("risc-v", &s_backend);
    registerEncodingBackend("riscv32", &s_backend);
}

std::string RiscVEncodingCodegenBackend::row(const Symbols::TargetInstructionSymbol &sym,
                                             DiagnosticCollector *diag) const
{
    if (!sym.m_encoding.has_value())
    {
        return "EncodingDesc{}";
    }

    Sema::Encoding::RiscVEncodingSpec spec;
    if (!Sema::Encoding::decodeRiscVEncoding(sym.m_encoding.value(), spec, diag))
    {
        return "EncodingDesc{}";
    }

    return emitEncodingDesc(sym, spec);
}

} // namespace CodeGenerators
