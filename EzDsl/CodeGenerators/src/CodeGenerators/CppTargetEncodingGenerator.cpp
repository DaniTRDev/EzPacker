#include "CodeGenerators/CppTargetEncodingGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetSymbols.h"

#include <cctype>
#include <format>
#include <string>
#include <vector>

namespace CodeGenerators
{

namespace
{
namespace AstEnc = DSL::Ast::TargetInstDef;

// Rewrites raw into a valid C++ identifier, substituting illegal characters and prefixing leading digits.
std::string sanitizeIdentifier(std::string_view raw, std::string_view fallback)
{
    std::string result;
    result.reserve(raw.size());
    for (char c : raw)
    {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc) || c == '_')
        {
            result.push_back(c);
        }
        else
        {
            result.push_back('_');
        }
    }
    if (result.empty())
    {
        result = std::string(fallback);
    }
    if (std::isdigit(static_cast<unsigned char>(result.front())))
    {
        result.insert(result.begin(), '_');
    }
    return result;
}

// Maps a parsed encoding form to the generated EncForm enumerator.
std::string_view formToString(AstEnc::EncForm form)
{
    switch (form)
    {
        case AstEnc::EncForm::Rr:
            return "EncForm::Rr";
        case AstEnc::EncForm::Rm:
            return "EncForm::Rm";
        case AstEnc::EncForm::Mr:
            return "EncForm::Mr";
        case AstEnc::EncForm::Ri:
            return "EncForm::Ri";
        case AstEnc::EncForm::MovRI:
            return "EncForm::MovRI";
        case AstEnc::EncForm::Movzx:
            return "EncForm::Movzx";
        case AstEnc::EncForm::Movsx:
            return "EncForm::Movsx";
        case AstEnc::EncForm::Lea:
            return "EncForm::Lea";
        case AstEnc::EncForm::Unary:
            return "EncForm::Unary";
        case AstEnc::EncForm::Test:
            return "EncForm::Test";
        case AstEnc::EncForm::Shift:
            return "EncForm::Shift";
        case AstEnc::EncForm::ImulRR:
            return "EncForm::ImulRR";
        case AstEnc::EncForm::ImulRI:
            return "EncForm::ImulRI";
        case AstEnc::EncForm::Div:
            return "EncForm::Div";
        case AstEnc::EncForm::Jcc:
            return "EncForm::Jcc";
        case AstEnc::EncForm::Jmp:
            return "EncForm::Jmp";
        case AstEnc::EncForm::Call:
            return "EncForm::Call";
        case AstEnc::EncForm::Ret:
            return "EncForm::Ret";
        case AstEnc::EncForm::Push:
            return "EncForm::Push";
        case AstEnc::EncForm::Pop:
            return "EncForm::Pop";
        case AstEnc::EncForm::Nop:
            return "EncForm::Nop";
        case AstEnc::EncForm::Syscall:
            return "EncForm::Syscall";
        case AstEnc::EncForm::Setcc:
            return "EncForm::Setcc";
        case AstEnc::EncForm::Sse:
            return "EncForm::Sse";
        case AstEnc::EncForm::Cvt:
            return "EncForm::Cvt";
        case AstEnc::EncForm::None:
        default:
            return "EncForm::None";
    }
}

// Maps a parsed operand slot kind to the generated EncSlotKind enumerator.
std::string_view slotToString(AstEnc::EncSlotKind slot)
{
    switch (slot)
    {
        case AstEnc::EncSlotKind::Reg:
            return "EncSlotKind::Reg";
        case AstEnc::EncSlotKind::RmReg:
            return "EncSlotKind::RmReg";
        case AstEnc::EncSlotKind::RmMem:
            return "EncSlotKind::RmMem";
        case AstEnc::EncSlotKind::Imm8:
            return "EncSlotKind::Imm8";
        case AstEnc::EncSlotKind::Imm16:
            return "EncSlotKind::Imm16";
        case AstEnc::EncSlotKind::Imm32:
            return "EncSlotKind::Imm32";
        case AstEnc::EncSlotKind::Imm64:
            return "EncSlotKind::Imm64";
        case AstEnc::EncSlotKind::Imm8Signed:
            return "EncSlotKind::Imm8Signed";
        case AstEnc::EncSlotKind::Rel8:
            return "EncSlotKind::Rel8";
        case AstEnc::EncSlotKind::Rel32:
            return "EncSlotKind::Rel32";
        case AstEnc::EncSlotKind::CondCode:
            return "EncSlotKind::CondCode";
        case AstEnc::EncSlotKind::None:
        default:
            return "EncSlotKind::None";
    }
}

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
std::string toByteList(const std::pmr::vector<uint8_t> &bytes)
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

// Name/description pair used when accumulating encodings for the generated table.
struct CollectedEncoding
{
    std::string m_name; ///< Instruction mnemonic.
    std::string m_desc; ///< Generated EncodingDesc initializer text.
};

// Serializes a single parsed ENCODING declaration into an EncodingDesc brace initializer.
std::string emitEncodingDesc(const Symbols::TargetInstructionSymbol &data, const AstEnc::EncodingDecl &enc)
{
    // Resolve an operand name to its index within the instruction signature.
    auto operandIndex = [&](const DSL::Ast::Common::Identifier &name) -> uint8_t
    {
        for (size_t i = 0; i < data.m_operands.size(); ++i)
        {
            if (data.m_operands[i].m_name == name.m_node)
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
        uint8_t index = operandIndex(binding.m_name);
        std::string_view type =
                (index < data.m_operands.size()) ? data.m_operands[index].m_regClassOrType : std::string_view{};
        if (i > 0)
        {
            operands += ", ";
        }
        operands += std::format("EncOperandBinding{{ {}, {}, {} }}",
                                slotToString(binding.m_slot),
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

    // Any Rel8/Rel32 slot is the branch target that needs relocation.
    uint8_t relocOperand = 0xFF;
    for (const auto &binding : enc.m_operands)
    {
        if (binding.m_slot == AstEnc::EncSlotKind::Rel32 || binding.m_slot == AstEnc::EncSlotKind::Rel8)
        {
            relocOperand = operandIndex(binding.m_name);
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
                       formToString(enc.m_form),
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
                       (enc.m_form == AstEnc::EncForm::MovRI) ? "true" : "false",
                       enc.m_byteRex ? "true" : "false",
                       enc.m_condCode.has_value() ? static_cast<unsigned>(enc.m_condCode.value()) : 0u,
                       enc.m_hasSseVariant ? "true" : "false",
                       static_cast<unsigned>(enc.m_ssePrefixes),
                       toByteList(enc.m_sseOpcode),
                       static_cast<unsigned>(enc.m_sseOpcode.size()));
}

} // namespace

// Binds the generator to its diagnostics/symbols and normalizes an empty target name to "Target".
CppTargetEncodingGenerator::CppTargetEncodingGenerator(DiagnosticCollector *collector,
                                                       SymbolTable *table,
                                                       std::filesystem::path outPath,
                                                       std::string targetName) :
    CodeGenerator("CodeGenerators::TargetEncodings", collector, table, std::move(outPath)),
    m_targetName(std::move(targetName))
{
    if (m_targetName.empty())
    {
        m_targetName = "Target";
    }
}

// Emits the header-only encoding descriptor table and its id/name lookup helpers.
void CppTargetEncodingGenerator::emitHeader(CppSourceEmitter &emitter) const
{
    std::string ns = std::format("EzCodeEmitter::TableGen::{}", sanitizeIdentifier(m_targetName, "Target"));

    emitter.emitBanner("CppTargetEncodingGenerator");
    emitter.emitBlankLine();
    emitter.emitLine("#pragma once");
    emitter.emitBlankLine();
    emitter.emitInclude("TableGen/EncodingDesc.h");
    emitter.emitLine("#include <cstddef>");
    emitter.emitLine("#include <cstring>");
    emitter.emitBlankLine();

    emitter.emitComment("Table-driven instruction encodings generated from ENCODING blocks in the .idf file.");
    {
        auto nsScope = emitter.enterNamespace(ns);
        emitter.emitBlankLine();

        // Collect target instructions in symbol-table order; encodings stay index-aligned with them.
        std::vector<const Symbol *> instSymbols;
        if (m_table)
        {
            for (const Symbol *sym : m_table->getSymbols())
            {
                if (sym && sym->getType() == SymbolType::TargetInstruction &&
                    sym->hasData<Symbols::TargetInstructionSymbol>())
                {
                    instSymbols.push_back(sym);
                }
            }
        }

        emitter.emitLine("inline constexpr EncodingDesc s_encodings[] =");
        {
            auto scope = emitter.enterScope("{", "};");
            for (const Symbol *sym : instSymbols)
            {
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();
                if (data->m_encoding.has_value())
                {
                    emitter.emitLine("{},", emitEncodingDesc(*data, data->m_encoding.value()));
                }
                else
                {
                    emitter.emitLine("EncodingDesc{},");
                }
            }
            if (instSymbols.empty())
            {
                emitter.emitLine("EncodingDesc{},");
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_encodingCount = {};", instSymbols.size());
        emitter.emitBlankLine();

        emitter.emitLine("inline constexpr const char *s_encodingNames[] =");
        {
            auto scope = emitter.enterScope("{", "};");
            for (const Symbol *sym : instSymbols)
            {
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();
                emitter.emitLine("\"{}\",", data->m_name);
            }
            if (instSymbols.empty())
            {
                emitter.emitLine("\"\",");
            }
        }
        emitter.emitBlankLine();

        emitter.emitLine("inline const EncodingDesc *getEncodingDesc(std::size_t id)");
        {
            auto scope = emitter.enterScope();
            emitter.emitLine("if (id == 0 || id > s_encodingCount)");
            {
                auto inner = emitter.enterScope();
                emitter.emitLine("return nullptr;");
            }
            emitter.emitLine("return &s_encodings[id - 1];");
        }
        emitter.emitBlankLine();

        emitter.emitLine("inline const EncodingDesc *findEncodingDesc(const char *name)");
        {
            auto scope = emitter.enterScope();
            emitter.emitLine("if (!name) return nullptr;");
            emitter.emitLine("for (std::size_t i = 0; i < s_encodingCount; ++i)");
            {
                auto inner = emitter.enterScope();
                emitter.emitLine("if (std::strcmp(s_encodingNames[i], name) == 0) return &s_encodings[i];");
            }
            emitter.emitLine("return nullptr;");
        }
    }
}

// Emits the encoding table for the current symbol table and writes the single output header.
bool CppTargetEncodingGenerator::run()
{
    if (!validate())
    {
        return false;
    }

    const std::string baseName = std::format("{}EncodingTable", sanitizeIdentifier(m_targetName, "Target"));
    const auto targetFilePath = resolveSingleFilePath(baseName + ".h");

    CppSourceEmitter emitter;
    emitHeader(emitter);

    if (!writeOutput(targetFilePath, emitter.view()))
    {
        return false;
    }

    trace("Synthesized target encoding table into {}", targetFilePath.filename().string());
    return true;
}

// Convenience wrapper retained for callers that do not need to configure a generator object.
bool GenerateTargetEncodingTable(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 std::filesystem::path outPath,
                                 std::string targetName)
{
    CppTargetEncodingGenerator generator(collector, table, std::move(outPath), std::move(targetName));
    return generator.run();
}

} // namespace CodeGenerators
