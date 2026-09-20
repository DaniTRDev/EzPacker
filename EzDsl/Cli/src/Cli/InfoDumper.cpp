#include "Cli/InfoDumper.h"

#include "Ast/CommonAstNodes.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TypeDefLangAst.h"
#include "Ast/CallingConvDefLangAst.h"
#include "Ast/RegisterDefLangAst.h"
#include "Ast/TargetDescDefLangAst.h"

#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/EnumNames.h"
#include "Sema/Symbols/IrSymbols.h"
#include "Sema/Symbols/LegalizeSymbols.h"
#include "Sema/Symbols/TypeSymbols.h"
#include "Sema/Symbols/CallingConvSymbols.h"

namespace Cli
{

namespace
{
/**
 * Text and JSON labels for one SymbolType. Both dump branches consult this single list so a
 * newly added symbol type cannot silently fall through to a generic "Other"/"[Symbol]" label,
 * and the text and JSON spellings stay together.
 */
struct SymbolTypeLabels
{
    std::string_view m_json; ///< Label used in the JSON "type" field.
    std::string_view m_text; ///< Label used in the text "[...]" prefix.
};

/** Maps every SymbolType to its shared display labels. */
constexpr SymbolTypeLabels symbolTypeLabels(SymbolType type) noexcept
{
    switch (type)
    {
        case SymbolType::Type:
            return { "Type", "Type" };
        case SymbolType::IrInstruction:
            return { "IrInstruction", "Instruction" };
        case SymbolType::IrInstructionOperand:
            return { "IrInstructionOperand", "IrOperand" };
        case SymbolType::LegalizeAction:
            return { "LegalizeAction", "LegalizeAction" };
        case SymbolType::TypeSet:
            return { "TypeSet", "TypeSet" };
        case SymbolType::LegalizeRule:
            return { "LegalizeRule", "LegalizeRule" };
        case SymbolType::TargetInstruction:
            return { "TargetInstruction", "TargetInstruction" };
        case SymbolType::TargetOperand:
            return { "TargetOperand", "TargetOperand" };
        case SymbolType::AddressingMode:
            return { "AddressingMode", "AddressingMode" };
        case SymbolType::SelectionPattern:
            return { "SelectionPattern", "SelectionPattern" };
        case SymbolType::CallingConv:
            return { "CallingConv", "CallingConv" };
        case SymbolType::RegisterFile:
            return { "RegisterFile", "RegisterFile" };
        case SymbolType::RegisterBank:
            return { "RegisterBank", "RegisterBank" };
        case SymbolType::RegisterClass:
            return { "RegisterClass", "RegisterClass" };
        case SymbolType::Register:
            return { "Register", "Register" };
        case SymbolType::SpecialRegister:
            return { "SpecialRegister", "SpecialRegister" };
        case SymbolType::TargetDesc:
            return { "TargetDesc", "TargetDesc" };
        case SymbolType::SsaVariable:
            return { "SsaVariable", "SsaVariable" };
        case SymbolType::ImmediateVariable:
            return { "ImmediateVariable", "ImmediateVariable" };
    }
    return { "Unknown", "Unknown" };
}
} // namespace

// Escapes a string for safe embedding in a JSON string literal (shared EscapeString, Json mode).
std::string InfoDumper::escapeJson(std::string_view str) { return EscapeString(str, EscapeMode::Json); }

// Renders a TypeKind enum value as its display string (shared canonical spelling).
std::string_view InfoDumper::typeKindToString(DSL::Ast::TypeDef::TypeKind kind)
{
    return Sema::EnumNames::typeKindName(kind);
}

// Renders an IR instruction category as its display string.
std::string_view InfoDumper::irCategoryToString(DSL::Ast::IrInstDef::IrInstCategory cat)
{
    return Sema::EnumNames::irCategoryName(cat);
}

// Renders an IR instruction tier as its display string.
std::string_view InfoDumper::irTierToString(DSL::Ast::IrInstDef::IrInstTier tier)
{
    return Sema::EnumNames::irTierName(tier);
}

// Renders an IR operand direction as IN/OUT/INOUT.
std::string_view InfoDumper::irOperandDirToString(DSL::Ast::IrInstDef::IrOperandDir dir)
{
    return Sema::EnumNames::irOperandDirName(dir);
}

// Expands an operand-type bitmask into a `|`-joined list of type names, or "None" when empty.
std::string InfoDumper::irOperandTypeToString(uint16_t typeMask)
{
    std::vector<std::string> parts;
    if (typeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::Register))
        parts.push_back("Register");
    if (typeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::Integer))
        parts.push_back("Integer");
    if (typeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::FloatingPoint))
        parts.push_back("FloatingPoint");
    if (typeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::Memory))
        parts.push_back("Memory");
    if (typeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::Reference))
        parts.push_back("Reference");
    if (typeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::RuntimeSymbol))
        parts.push_back("RuntimeSymbol");
    if (typeMask & static_cast<uint16_t>(DSL::Ast::IrInstDef::IrOperandType::VariadicArgs))
        parts.push_back("VariadicArgs");

    if (parts.empty())
        return "None";

    std::string result;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
            result += "|";
        result += parts[i];
    }
    return result;
}

// Expands an instruction-flag bitmask into the list of set flag names.
std::vector<std::string> InfoDumper::irFlagsToStrings(uint32_t flagMask)
{
    std::vector<std::string> flags;
    // Adds name to flags when the matching bit is present in the mask.
    auto check = [&](DSL::Ast::IrInstDef::IrInstFlag f, const char *name)
    {
        if ((flagMask & static_cast<uint32_t>(f)) != 0)
            flags.push_back(name);
    };

    check(DSL::Ast::IrInstDef::IrInstFlag::SizeMatch, "SizeMatch");
    check(DSL::Ast::IrInstDef::IrInstFlag::DestLarger, "DestLarger");
    check(DSL::Ast::IrInstDef::IrInstFlag::DestSmaller, "DestSmaller");
    check(DSL::Ast::IrInstDef::IrInstFlag::ReadsMemory, "ReadsMemory");
    check(DSL::Ast::IrInstDef::IrInstFlag::WritesMemory, "WritesMemory");
    check(DSL::Ast::IrInstDef::IrInstFlag::IsTerminator, "IsTerminator");
    check(DSL::Ast::IrInstDef::IrInstFlag::IsBranch, "IsBranch");
    check(DSL::Ast::IrInstDef::IrInstFlag::IsCall, "IsCall");
    check(DSL::Ast::IrInstDef::IrInstFlag::IsReturn, "IsReturn");
    check(DSL::Ast::IrInstDef::IrInstFlag::HasSideEffect, "HasSideEffect");
    check(DSL::Ast::IrInstDef::IrInstFlag::IsCommutative, "IsCommutative");
    check(DSL::Ast::IrInstDef::IrInstFlag::ReadsCPUFlags, "ReadsCPUFlags");
    check(DSL::Ast::IrInstDef::IrInstFlag::WritesCPUFlags, "WritesCPUFlags");
    check(DSL::Ast::IrInstDef::IrInstFlag::TreatAsSigned, "TreatAsSigned");
    check(DSL::Ast::IrInstDef::IrInstFlag::VariadicArgs, "VariadicArgs");

    return flags;
}

// Writes the aggregated input/output summary as JSON or aligned text.
void InfoDumper::dumpGeneralInfo(const GeneralFileInfo &info, OutputFormat format, std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << std::format("  \"input\": \"{}\",\n", escapeJson(info.inputPath.string()));
        os << std::format("  \"size_bytes\": {},\n", info.fileSizeBytes);
        os << std::format("  \"dialect\": \"{}\",\n", escapeJson(info.dialectName));
        os << std::format("  \"construct_count\": {},\n", info.constructCount);
        os << std::format("  \"generator\": \"{}\",\n", escapeJson(info.generatorName));
        os << std::format("  \"working_mode\": \"{}\",\n", escapeJson(info.workingMode));
        os << std::format("  \"output_directory\": \"{}\",\n", escapeJson(info.outputDirectory.string()));
        os << "  \"outputs\": [\n";
        for (size_t i = 0; i < info.outputs.size(); ++i)
        {
            const auto &out = info.outputs[i];
            os << "    {\n";
            os << std::format("      \"role\": \"{}\",\n", escapeJson(out.role));
            os << std::format("      \"path\": \"{}\",\n", escapeJson(out.path.string()));
            os << std::format("      \"exists\": {}\n", out.exists ? "true" : "false");
            os << (i + 1 < info.outputs.size() ? "    },\n" : "    }\n");
        }
        os << "  ]\n";
        os << "}\n";
    }
    else
    {
        os << "======================================================================\n";
        os << "EzDSL File Information\n";
        os << "======================================================================\n";
        os << std::format("  Input File:        {}\n", info.inputPath.string());
        os << std::format("  File Size:         {} bytes\n", info.fileSizeBytes);
        os << std::format("  Recognized Dialect:{}\n", info.dialectName);
        os << std::format("  Constructs Found:  {}\n", info.constructCount);
        os << std::format("  Target Generator:  {}\n", info.generatorName);
        os << std::format("  Working Mode:      {}\n", info.workingMode);
        os << std::format("  Output Directory:  {}\n", info.outputDirectory.string());
        os << "  Expected Outputs:\n";
        for (const auto &out : info.outputs)
        {
            os << std::format("    - [{}] {} (exists: {})\n", out.role, out.path.string(), out.exists ? "yes" : "no");
        }
        os << "======================================================================\n";
    }
}

// Writes the expected/generated output file list as JSON or one line per file in text mode.
void InfoDumper::dumpOutputFiles(const std::vector<OutputFileInfo> &files, OutputFormat format, std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"outputs\": [\n";
        for (size_t i = 0; i < files.size(); ++i)
        {
            const auto &out = files[i];
            os << "    {\n";
            os << std::format("      \"role\": \"{}\",\n", escapeJson(out.role));
            os << std::format("      \"path\": \"{}\",\n", escapeJson(out.path.string()));
            os << std::format("      \"exists\": {}\n", out.exists ? "true" : "false");
            os << (i + 1 < files.size() ? "    },\n" : "    }\n");
        }
        os << "  ]\n";
        os << "}\n";
    }
    else
    {
        for (const auto &out : files)
        {
            os << std::format("[{}] {}\n", out.role, out.path.string());
        }
    }
}

// Dumps the parsed .tyf type declarations, omitting absent bit-width/alignment fields in JSON.
void InfoDumper::dumpTypeDefAst(const DSL::Ast::TypeDef::TypeDefFile &file, OutputFormat format, std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"types\": [\n";
        for (size_t i = 0; i < file.m_types.size(); ++i)
        {
            const auto &t = file.m_types[i];
            os << "    {\n";
            os << std::format("      \"name\": \"{}\",\n", escapeJson(t.m_name.m_node));
            os << std::format("      \"kind\": \"{}\",\n", typeKindToString(t.m_kind));
            if (t.m_bitSize.has_value())
                os << std::format("      \"bit_width\": {},\n", t.m_bitSize->m_node);
            else
                os << "      \"bit_width\": null,\n";

            if (t.m_alignment.has_value())
                os << std::format("      \"alignment\": {}\n", t.m_alignment->m_node);
            else
                os << "      \"alignment\": null\n";

            os << (i + 1 < file.m_types.size() ? "    },\n" : "    }\n");
        }
        os << "  ]\n";
        os << "}\n";
    }
    else
    {
        os << "=== TypeDef AST Summary (" << file.m_types.size() << " types) ===\n";
        for (const auto &t : file.m_types)
        {
            std::string bw = t.m_bitSize ? std::to_string(t.m_bitSize->m_node) : "n/a";
            std::string al = t.m_alignment ? std::to_string(t.m_alignment->m_node) : "n/a";
            os << std::format("  Type: {:<12} Kind: {:<14} BitWidth: {:<6} Alignment: {}\n",
                              t.m_name.m_node,
                              typeKindToString(t.m_kind),
                              bw,
                              al);
        }
    }
}

// Dumps the parsed .irdf instructions including their flags and operand signatures.
void InfoDumper::dumpIrInstDefAst(const DSL::Ast::IrInstDef::IrInstDefFile &file, OutputFormat format, std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"instructions\": [\n";
        for (size_t i = 0; i < file.m_instructions.size(); ++i)
        {
            const auto &inst = file.m_instructions[i];
            os << "    {\n";
            os << std::format("      \"name\": \"{}\",\n", escapeJson(inst.m_name.m_node));
            os << std::format("      \"category\": \"{}\",\n",
                              irCategoryToString(inst.m_body.m_category));
            os << std::format("      \"tier\": \"{}\",\n", irTierToString(inst.m_body.m_tier));

            // Flags
            // The AST stores flags individually; OR them into the bitmask the formatter expects.
            uint32_t combinedFlags = 0;
            for (auto f : inst.m_body.m_flags)
                combinedFlags |= static_cast<uint32_t>(f);
            auto flagStrs = irFlagsToStrings(combinedFlags);
            os << "      \"flags\": [";
            for (size_t fIdx = 0; fIdx < flagStrs.size(); ++fIdx)
            {
                os << "\"" << escapeJson(flagStrs[fIdx]) << "\"";
                if (fIdx + 1 < flagStrs.size())
                    os << ", ";
            }
            os << "],\n";

            // Operands
            os << "      \"operands\": [\n";
            for (size_t opIdx = 0; opIdx < inst.m_operands.size(); ++opIdx)
            {
                const auto &op = inst.m_operands[opIdx];
                os << "        {\n";
                os << std::format("          \"name\": \"{}\",\n", escapeJson(op.m_name.m_node));
                os << std::format("          \"type\": \"{}\",\n",
                                  irOperandTypeToString(static_cast<uint16_t>(op.m_type)));
                os << std::format("          \"direction\": \"{}\"\n",
                                  irOperandDirToString(op.m_dir));
                os << (opIdx + 1 < inst.m_operands.size() ? "        },\n" : "        }\n");
            }
            os << "      ]\n";

            os << (i + 1 < file.m_instructions.size() ? "    },\n" : "    }\n");
        }
        os << "  ]\n";
        os << "}\n";
    }
    else
    {
        os << "=== IR Instruction AST Summary (" << file.m_instructions.size() << " instructions) ===\n";
        for (const auto &inst : file.m_instructions)
        {
            os << std::format("  Instruction: {} [Category: {}, Tier: {}]\n",
                              inst.m_name.m_node,
                              irCategoryToString(inst.m_body.m_category),
                              irTierToString(inst.m_body.m_tier));

            uint32_t combinedFlags = 0;
            for (auto f : inst.m_body.m_flags)
                combinedFlags |= static_cast<uint32_t>(f);
            auto flagStrs = irFlagsToStrings(combinedFlags);
            if (!flagStrs.empty())
            {
                os << "    Flags: ";
                for (size_t fIdx = 0; fIdx < flagStrs.size(); ++fIdx)
                {
                    if (fIdx > 0)
                        os << ", ";
                    os << flagStrs[fIdx];
                }
                os << "\n";
            }

            if (!inst.m_operands.empty())
            {
                os << "    Operands (" << inst.m_operands.size() << "):\n";
                for (const auto &op : inst.m_operands)
                {
                    os << std::format("      - {:<16} {:<12} [{}]\n",
                                      op.m_name.m_node,
                                      irOperandTypeToString(static_cast<uint16_t>(op.m_type)),
                                      irOperandDirToString(op.m_dir));
                }
            }
        }
    }
}

// Dumps the parsed .lad opcode actions and their clause counts.
void InfoDumper::dumpLegalizeActionAst(const DSL::Ast::LegalizeActionDef::LegalizeActionFile &file,
                                       OutputFormat format,
                                       std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"actions\": [\n";
        for (size_t i = 0; i < file.m_legalizeInstrDecls.size(); ++i)
        {
            const auto &act = file.m_legalizeInstrDecls[i];
            os << "    {\n";
            os << std::format("      \"instruction\": \"{}\",\n", escapeJson(act.m_instName.m_node));
            os << std::format("      \"clause_count\": {}\n", act.m_actionClauses.size());
            os << (i + 1 < file.m_legalizeInstrDecls.size() ? "    },\n" : "    }\n");
        }
        os << "  ]\n";
        os << "}\n";
    }
    else
    {
        os << "=== LegalizeAction AST Summary (" << file.m_legalizeInstrDecls.size() << " opcode actions) ===\n";
        for (const auto &act : file.m_legalizeInstrDecls)
        {
            os << std::format("  Action: {} ({} clauses)\n", act.m_instName.m_node, act.m_actionClauses.size());
        }
    }
}

// Dumps the parsed .lrd rules: match patterns, when-predicates and emit sequences.
void InfoDumper::dumpLegalizeRuleAst(const DSL::Ast::LegalizeRuleDef::LegalizeRuleFile &file,
                                     OutputFormat format,
                                     std::ostream &os)
{
    // Renders a rule-operand kind as its display string.
    auto opKindToStr = [](DSL::Ast::LegalizeRuleDef::RuleOperandKind kind) -> std::string_view
    {
        switch (kind)
        {
            case DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister:
                return "SsaRegister";
            case DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol:
                return "ImmediateSymbol";
            case DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral:
                return "ImmediateLiteral";
            case DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform:
                return "CustomTransform";
        }
        return "Unknown";
    };

    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"rules\": [\n";
        for (size_t rIdx = 0; rIdx < file.m_rules.size(); ++rIdx)
        {
            const auto &r = file.m_rules[rIdx];
            os << "    {\n";
            os << std::format("      \"name\": \"{}\",\n", escapeJson(r.m_ruleName.m_node));

            // Matches
            os << "      \"matches\": [\n";
            for (size_t mIdx = 0; mIdx < r.m_matchClauses.size(); ++mIdx)
            {
                const auto &m = r.m_matchClauses[mIdx];
                os << "        {\n";
                os << std::format("          \"opcode\": \"{}\",\n", escapeJson(m.m_opcode.m_node));
                os << "          \"operands\": [\n";
                for (size_t oIdx = 0; oIdx < m.m_operands.size(); ++oIdx)
                {
                    const auto &op = m.m_operands[oIdx];
                    os << "            {\n";
                    os << std::format("              \"kind\": \"{}\",\n", opKindToStr(op.m_kind));
                    if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral)
                    {
                        os << std::format("              \"val\": {}\n", op.m_immLiteral ? op.m_immLiteral->m_node : 0);
                    }
                    else
                    {
                        os << std::format("              \"name\": \"{}\"", escapeJson(op.m_name.m_node));
                        std::string typeStr;
                        if (op.m_typeParam.has_value())
                            typeStr = std::string(op.m_typeParam->m_node);
                        else if (op.m_type.has_value() && op.m_type->m_node != "imm")
                            typeStr = std::string(op.m_type->m_node);
                        if (!typeStr.empty())
                        {
                            os << ",\n" << std::format("              \"type\": \"{}\"\n", escapeJson(typeStr));
                        }
                        else
                        {
                            os << "\n";
                        }
                    }
                    os << (oIdx + 1 < m.m_operands.size() ? "            },\n" : "            }\n");
                }
                os << "          ]\n";
                os << (mIdx + 1 < r.m_matchClauses.size() ? "        },\n" : "        }\n");
            }
            os << "      ],\n";

            // Predicates
            os << "      \"predicates\": [\n";
            for (size_t pIdx = 0; pIdx < r.m_whenClauses.size(); ++pIdx)
            {
                const auto &p = r.m_whenClauses[pIdx];
                os << "        {\n";
                os << std::format("          \"name\": \"{}\",\n", escapeJson(p.m_predicateName.m_node));
                os << "          \"args\": [";
                for (size_t aIdx = 0; aIdx < p.m_arguments.size(); ++aIdx)
                {
                    if (aIdx > 0)
                        os << ", ";
                    if (const auto *ident = std::get_if<DSL::Ast::Common::Identifier>(&p.m_arguments[aIdx]))
                    {
                        os << std::format("\"{}\"", escapeJson(ident->m_node));
                    }
                    else if (const auto *lit = std::get_if<DSL::Ast::Common::IntegerLiteral>(&p.m_arguments[aIdx]))
                    {
                        os << lit->m_node;
                    }
                }
                os << "]\n";
                os << (pIdx + 1 < r.m_whenClauses.size() ? "        },\n" : "        }\n");
            }
            os << "      ],\n";

            // Emits
            os << "      \"emits\": [\n";
            for (size_t eIdx = 0; eIdx < r.m_emitClauses.size(); ++eIdx)
            {
                const auto &e = r.m_emitClauses[eIdx];
                os << "        {\n";
                os << std::format("          \"opcode\": \"{}\",\n", escapeJson(e.m_opcode.m_node));
                os << "          \"operands\": [\n";
                for (size_t oIdx = 0; oIdx < e.m_operands.size(); ++oIdx)
                {
                    const auto &op = e.m_operands[oIdx];
                    os << "            {\n";
                    os << std::format("              \"kind\": \"{}\",\n", opKindToStr(op.m_kind));
                    if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform)
                    {
                        os << std::format("              \"func\": \"{}\",\n", escapeJson(op.m_name.m_node));
                        os << "              \"args\": [";
                        for (size_t aIdx = 0; aIdx < op.m_callArgs.size(); ++aIdx)
                        {
                            if (aIdx > 0)
                                os << ", ";
                            os << std::format("\"{}\"", escapeJson(op.m_callArgs[aIdx].m_node));
                        }
                        os << "]\n";
                    }
                    else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral)
                    {
                        os << std::format("              \"val\": {}\n", op.m_immLiteral ? op.m_immLiteral->m_node : 0);
                    }
                    else
                    {
                        os << std::format("              \"name\": \"{}\"", escapeJson(op.m_name.m_node));
                        std::string typeStr;
                        if (op.m_typeParam.has_value())
                            typeStr = std::string(op.m_typeParam->m_node);
                        else if (op.m_type.has_value() && op.m_type->m_node != "imm")
                            typeStr = std::string(op.m_type->m_node);
                        if (!typeStr.empty())
                        {
                            os << ",\n" << std::format("              \"type\": \"{}\"\n", escapeJson(typeStr));
                        }
                        else
                        {
                            os << "\n";
                        }
                    }
                    os << (oIdx + 1 < e.m_operands.size() ? "            },\n" : "            }\n");
                }
                os << "          ]\n";
                os << (eIdx + 1 < r.m_emitClauses.size() ? "        },\n" : "        }\n");
            }
            os << "      ]\n";

            os << (rIdx + 1 < file.m_rules.size() ? "    },\n" : "    }\n");
        }
        os << "  ]\n";
        os << "}\n";
    }
    else
    {
        os << "======================================================================\n";
        os << std::format("LegalizeRule AST Dump ({} rules)\n", file.m_rules.size());
        os << "======================================================================\n";
        for (const auto &r : file.m_rules)
        {
            os << std::format("  Rule: {}\n", r.m_ruleName.m_node);

            os << "    [Match Pattern]\n";
            for (const auto &m : r.m_matchClauses)
            {
                os << std::format("      - {} (operands: {})\n", m.m_opcode.m_node, m.m_operands.size());
                for (size_t i = 0; i < m.m_operands.size(); ++i)
                {
                    const auto &op = m.m_operands[i];
                    os << std::format("          #{}: ", i);
                    if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister)
                    {
                        os << "SSA Register $" << op.m_name.m_node;
                        if (op.m_type.has_value())
                            os << " (type: " << op.m_type->m_node << ")";
                    }
                    else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol)
                    {
                        os << "Symbolic Immediate $" << op.m_name.m_node;
                        if (op.m_typeParam.has_value())
                            os << " (type: " << op.m_typeParam->m_node << ")";
                        else if (op.m_type.has_value())
                            os << " (type: " << op.m_type->m_node << ")";
                    }
                    else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral)
                    {
                        os << "Immediate Literal " << (op.m_immLiteral.has_value() ? op.m_immLiteral->m_node : 0);
                    }
                    else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform)
                    {
                        os << "Custom Transform " << op.m_name.m_node << "(";
                        for (size_t a = 0; a < op.m_callArgs.size(); ++a)
                        {
                            if (a > 0)
                                os << ", ";
                            os << "$" << op.m_callArgs[a].m_node;
                        }
                        os << ")";
                    }
                    os << "\n";
                }
            }

            if (!r.m_whenClauses.empty())
            {
                os << "    [When Predicates]\n";
                for (const auto &w : r.m_whenClauses)
                {
                    os << std::format("      - {}(", w.m_predicateName.m_node);
                    for (size_t a = 0; a < w.m_arguments.size(); ++a)
                    {
                        if (a > 0)
                            os << ", ";
                        if (const auto *ident = std::get_if<DSL::Ast::Common::Identifier>(&w.m_arguments[a]))
                            os << "$" << ident->m_node;
                        else if (const auto *lit = std::get_if<DSL::Ast::Common::IntegerLiteral>(&w.m_arguments[a]))
                            os << lit->m_node;
                    }
                    os << ")\n";
                }
            }

            os << "    [Emit Sequence]\n";
            for (const auto &e : r.m_emitClauses)
            {
                os << std::format("      - {} (operands: {})\n", e.m_opcode.m_node, e.m_operands.size());
                for (size_t i = 0; i < e.m_operands.size(); ++i)
                {
                    const auto &op = e.m_operands[i];
                    os << std::format("          #{}: ", i);
                    if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::SsaRegister)
                    {
                        os << "SSA Register $" << op.m_name.m_node;
                        if (op.m_type.has_value())
                            os << " (type: " << op.m_type->m_node << ")";
                    }
                    else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateSymbol)
                    {
                        os << "Symbolic Immediate $" << op.m_name.m_node;
                        if (op.m_typeParam.has_value())
                            os << " (type: " << op.m_typeParam->m_node << ")";
                        else if (op.m_type.has_value())
                            os << " (type: " << op.m_type->m_node << ")";
                    }
                    else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::ImmediateLiteral)
                    {
                        os << "Immediate Literal " << (op.m_immLiteral.has_value() ? op.m_immLiteral->m_node : 0);
                    }
                    else if (op.m_kind == DSL::Ast::LegalizeRuleDef::RuleOperandKind::CustomTransform)
                    {
                        os << "Custom Transform " << op.m_name.m_node << "(";
                        for (size_t a = 0; a < op.m_callArgs.size(); ++a)
                        {
                            if (a > 0)
                                os << ", ";
                            os << "$" << op.m_callArgs[a].m_node;
                        }
                        os << ")";
                    }
                    os << "\n";
                }
            }
        }
        os << "======================================================================\n";
    }
}

// Dumps the parsed .ezcc/.ccd calling convention stack and rule counts.
void InfoDumper::dumpCallingConvAst(const DSL::Ast::CallingConvDef::CallingConventionDefFile &file,
                                    OutputFormat format,
                                    std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << std::format("  \"name\": \"{}\",\n", escapeJson(file.m_name.m_node));
        os << std::format("  \"stack_align\": {},\n", file.m_stack.m_alignment.m_node);
        os << std::format("  \"stack_growth\": \"{}\",\n",
                          file.m_stack.m_growth == DSL::Ast::CallingConvDef::StackGrowth::Down ? "down" : "up");
        os << std::format("  \"shadow_space\": {},\n", file.m_stack.m_shadowSpace.m_node);
        os << std::format("  \"red_zone\": {}\n", file.m_stack.m_redZone ? file.m_stack.m_redZone->m_node : 0);
        os << "}\n";
    }
    else
    {
        os << "======================================================================\n";
        os << std::format("CallingConvention AST Dump: {}\n", file.m_name.m_node);
        os << "======================================================================\n";
        os << std::format("  Stack Alignment: {}\n", file.m_stack.m_alignment.m_node);
        os << std::format("  Stack Growth:    {}\n",
                          file.m_stack.m_growth == DSL::Ast::CallingConvDef::StackGrowth::Down ? "down" : "up");
        os << std::format("  Shadow Space:    {} bytes\n", file.m_stack.m_shadowSpace.m_node);
        if (file.m_stack.m_redZone)
            os << std::format("  Red Zone:        {} bytes\n", file.m_stack.m_redZone->m_node);
        os << std::format("  Argument Rules:  {}\n", file.m_arguments.m_rules.size());
        os << std::format("  Return Rules:    {}\n", file.m_returns.m_rules.size());
        os << "======================================================================\n";
    }
}

// Dumps the parsed .reg bank/class/register hierarchy and special registers.
void InfoDumper::dumpRegisterDefAst(const DSL::Ast::RegisterDef::RegisterFile &file,
                                    OutputFormat format,
                                    std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << std::format("  \"target\": \"{}\",\n", escapeJson(file.m_target.m_node));
        os << "  \"banks\": [\n";
        for (size_t b = 0; b < file.m_banks.size(); ++b)
        {
            const auto &bank = file.m_banks[b];
            os << "    {\n";
            os << std::format("      \"name\": \"{}\",\n", escapeJson(bank.m_name.m_node));

            os << "      \"classes\": [";
            for (size_t c = 0; c < bank.m_classes.size(); ++c)
            {
                os << std::format("{{ \"name\": \"{}\", \"bits\": {} }}",
                                  escapeJson(bank.m_classes[c].m_name.m_node),
                                  bank.m_classes[c].m_bitSize.m_node);
                if (c + 1 < bank.m_classes.size())
                    os << ", ";
            }
            os << "],\n";

            os << "      \"sub_register_edges\": [";
            for (size_t e = 0; e < bank.m_subRegisterEdges.size(); ++e)
            {
                os << std::format("{{ \"wide\": \"{}\", \"narrow\": \"{}\" }}",
                                  escapeJson(bank.m_subRegisterEdges[e].m_wideClass.m_node),
                                  escapeJson(bank.m_subRegisterEdges[e].m_narrowClass.m_node));
                if (e + 1 < bank.m_subRegisterEdges.size())
                    os << ", ";
            }
            os << "],\n";

            os << "      \"registers\": [\n";
            for (size_t r = 0; r < bank.m_registers.size(); ++r)
            {
                const auto &reg = bank.m_registers[r];
                os << "        {\n";
                os << std::format("          \"name\": \"{}\",\n", escapeJson(reg.m_canonicalName.m_node));
                os << std::format("          \"hw_encoding\": {},\n", reg.m_encoding.m_node);
                os << "          \"names\": [";
                for (size_t n = 0; n < reg.m_names.size(); ++n)
                {
                    os << std::format("{{ \"asm\": \"{}\", \"class\": \"{}\" }}",
                                      escapeJson(reg.m_names[n].m_asmName.m_node),
                                      escapeJson(reg.m_names[n].m_className.m_node));
                    if (n + 1 < reg.m_names.size())
                        os << ", ";
                }
                os << "]\n";
                os << (r + 1 < bank.m_registers.size() ? "        },\n" : "        }\n");
            }
            os << "      ]\n";
            os << (b + 1 < file.m_banks.size() ? "    },\n" : "    }\n");
        }
        os << "  ],\n";

        os << "  \"special_registers\": [";
        for (size_t s = 0; s < file.m_specialRegs.size(); ++s)
        {
            os << std::format("{{ \"name\": \"{}\", \"id\": {} }}",
                              escapeJson(file.m_specialRegs[s].m_name.m_node),
                              file.m_specialRegs[s].m_id.m_node);
            if (s + 1 < file.m_specialRegs.size())
                os << ", ";
        }
        os << "]\n";
        os << "}\n";
    }
    else
    {
        os << "======================================================================\n";
        os << std::format("RegisterDef AST Dump (target: {}, {} banks)\n", file.m_target.m_node, file.m_banks.size());
        os << "======================================================================\n";
        for (const auto &bank : file.m_banks)
        {
            os << std::format("  Bank: {} ({} classes, {} registers)\n",
                              bank.m_name.m_node,
                              bank.m_classes.size(),
                              bank.m_registers.size());
            for (const auto &cls : bank.m_classes)
            {
                os << std::format("    Class: {:<12} {} bits\n", cls.m_name.m_node, cls.m_bitSize.m_node);
            }
            for (const auto &edge : bank.m_subRegisterEdges)
            {
                os << std::format("    Sub: {} <: {}\n", edge.m_wideClass.m_node, edge.m_narrowClass.m_node);
            }
            for (const auto &reg : bank.m_registers)
            {
                os << std::format("    Reg: {:<12} enc {:<3}", reg.m_canonicalName.m_node, reg.m_encoding.m_node);
                for (const auto &name : reg.m_names)
                {
                    os << std::format(" [{}: {}]", name.m_asmName.m_node, name.m_className.m_node);
                }
                os << "\n";
            }
        }
        for (const auto &special : file.m_specialRegs)
        {
            os << std::format("  Special: {} = {}\n", special.m_name.m_node, special.m_id.m_node);
        }
        os << "======================================================================\n";
    }
}

// Dumps the parsed .tdesc manifest: formats, components, libcalls and ABI settings.
void InfoDumper::dumpTargetDescAst(const DSL::Ast::TargetDesc::TargetDescFile &file,
                                   OutputFormat format,
                                   std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << std::format("  \"target\": \"{}\",\n", escapeJson(file.m_name.m_node));
        os << std::format("  \"pointer_size\": {},\n", file.m_pointerSize ? file.m_pointerSize->m_node : 0);
        os << std::format("  \"stack_slot\": {},\n", file.m_stackSlot ? file.m_stackSlot->m_node : 0);
        os << std::format("  \"instruction_pointer\": \"{}\",\n",
                          file.mInstructionPointer ? escapeJson(file.mInstructionPointer->m_node) : "");
        os << std::format("  \"default_calling_conv\": \"{}\",\n",
                          file.mDefaultCallingConv ? escapeJson(file.mDefaultCallingConv->m_node) : "");
        os << "  \"object_formats\": [";
        for (size_t i = 0; i < file.mObjectFormats.size(); ++i)
        {
            os << std::format("\"{}\"", escapeJson(file.mObjectFormats[i].m_node));
            if (i + 1 < file.mObjectFormats.size())
                os << ", ";
        }
        os << "],\n";
        os << "  \"components\": [";
        for (size_t i = 0; i < file.mComponents.size(); ++i)
        {
            os << std::format("{{ \"slot\": \"{}\", \"type\": \"{}\" }}",
                              escapeJson(file.mComponents[i].m_slot.m_node),
                              escapeJson(file.mComponents[i].m_type.m_node));
            if (i + 1 < file.mComponents.size())
                os << ", ";
        }
        os << "],\n";
        os << "  \"libcalls\": [";
        for (size_t i = 0; i < file.mLibcalls.size(); ++i)
        {
            os << std::format("{{ \"id\": \"{}\", \"symbol\": \"{}\" }}",
                              escapeJson(file.mLibcalls[i].m_name.m_node),
                              escapeJson(file.mLibcalls[i].m_symbol.m_node));
            if (i + 1 < file.mLibcalls.size())
                os << ", ";
        }
        os << "]\n";
        os << "}\n";
    }
    else
    {
        os << "======================================================================\n";
        os << std::format("TargetDesc AST Dump (target: {})\n", file.m_name.m_node);
        os << "======================================================================\n";
        if (file.m_pointerSize)
            os << std::format("  Pointer Size:       {}\n", file.m_pointerSize->m_node);
        if (file.m_stackSlot)
            os << std::format("  Stack Slot:         {}\n", file.m_stackSlot->m_node);
        if (file.mInstructionPointer)
            os << std::format("  Instruction Ptr:    {}\n", file.mInstructionPointer->m_node);
        if (file.mDefaultCallingConv)
            os << std::format("  Default CallingConv:{}\n", file.mDefaultCallingConv->m_node);
        os << "  Object Formats:     ";
        for (size_t i = 0; i < file.mObjectFormats.size(); ++i)
        {
            if (i > 0)
                os << ", ";
            os << file.mObjectFormats[i].m_node;
        }
        os << "\n";
        os << std::format("  Components:         {}\n", file.mComponents.size());
        for (const auto &component : file.mComponents)
        {
            os << std::format("    - {}: {}\n", component.m_slot.m_node, component.m_type.m_node);
        }
        os << std::format("  Libcalls:           {}\n", file.mLibcalls.size());
        for (const auto &libcall : file.mLibcalls)
        {
            os << std::format("    - {}: {}\n", libcall.m_name.m_node, libcall.m_symbol.m_node);
        }
        os << "======================================================================\n";
    }
}

// Dumps the semantic symbol table, selecting per-symbol detail based on the symbol type.
void InfoDumper::dumpSymbols(const SymbolTable &symbolTable, OutputFormat format, std::ostream &os)
{
    const auto &symbols = symbolTable.getSymbols();

    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"symbols\": [\n";
        // Tracks whether the next symbol needs a leading comma (null entries are skipped).
        bool first = true;
        for (size_t i = 0; i < symbols.size(); ++i)
        {
            const auto *sym = symbols[i];
            if (!sym)
                continue;

            if (!first)
                os << ",\n";
            first = false;

            os << "    {\n";
            os << std::format("      \"id\": {},\n", sym->getId());
            os << std::format("      \"name\": \"{}\",\n", escapeJson(sym->getName()));
            os << std::format("      \"scope_id\": {},\n", sym->getDefiningScopeId());

            const auto labels = symbolTypeLabels(sym->getType());
            switch (sym->getType())
            {
                case SymbolType::Type:
                {
                    os << std::format("      \"type\": \"{}\",\n", labels.m_json);
                    if (const auto *td = sym->getIf<Symbols::TypeSymbol>())
                    {
                        os << std::format("      \"kind\": \"{}\",\n", typeKindToString(td->m_kind));
                        os << std::format("      \"bit_width\": {},\n", td->m_bitWidth);
                        os << std::format("      \"alignment\": {},\n", td->m_alignment);
                        os << std::format("      \"compact_id\": {}\n", td->m_compactId);
                    }
                    else
                    {
                        os << "      \"details\": null\n";
                    }
                    break;
                }
                case SymbolType::IrInstruction:
                {
                    os << std::format("      \"type\": \"{}\",\n", labels.m_json);
                    if (const auto *id = sym->getIf<Symbols::IrInstructionSymbol>())
                    {
                        os << std::format("      \"category\": \"{}\",\n", irCategoryToString(id->m_category));
                        os << std::format("      \"tier\": \"{}\",\n", irTierToString(id->m_tier));
                        os << std::format("      \"operand_count\": {}\n", id->m_operands.size());
                    }
                    else
                    {
                        os << "      \"details\": null\n";
                    }
                    break;
                }
                case SymbolType::LegalizeRule:
                {
                    os << std::format("      \"type\": \"{}\",\n", labels.m_json);
                    if (const auto *rd = sym->getIf<Symbols::LegalizeRuleSymbol>())
                    {
                        os << std::format("      \"match_count\": {},\n", rd->m_matchPatterns.size());
                        os << std::format("      \"emit_count\": {}\n", rd->m_expansionSequence.size());
                    }
                    else
                    {
                        os << "      \"details\": null\n";
                    }
                    break;
                }
                default:
                {
                    os << std::format("      \"type\": \"{}\",\n", labels.m_json);
                    os << "      \"details\": null\n";
                    break;
                }
            }
            os << "    }";
        }
        os << "\n  ]\n";
        os << "}\n";
    }
    else
    {
        os << "======================================================================\n";
        os << "Symbol Table Dump (" << symbols.size() << " symbols)\n";
        os << "======================================================================\n";
        for (const auto *sym : symbols)
        {
            if (!sym)
                continue;

            const auto labels = symbolTypeLabels(sym->getType());
            switch (sym->getType())
            {
                case SymbolType::Type:
                {
                    if (const auto *td = sym->getIf<Symbols::TypeSymbol>())
                    {
                        os << std::format("  [{}] ID: {:<3} Scope: {:<2} Name: {:<12} Kind: {:<12} Width: {:<4} "
                                          "Align: {:<4} CompactID: {}\n",
                                          labels.m_text,
                                          sym->getId(),
                                          sym->getDefiningScopeId(),
                                          sym->getName(),
                                          typeKindToString(td->m_kind),
                                          td->m_bitWidth,
                                          td->m_alignment,
                                          td->m_compactId);
                    }
                    break;
                }
                case SymbolType::IrInstruction:
                {
                    if (const auto *id = sym->getIf<Symbols::IrInstructionSymbol>())
                    {
                        os << std::format("  [{}] ID: {:<3} Scope: {:<2} Name: {:<16} Category: {:<14} Tier: "
                                          "{:<12} Operands: {}\n",
                                          labels.m_text,
                                          sym->getId(),
                                          sym->getDefiningScopeId(),
                                          sym->getName(),
                                          irCategoryToString(id->m_category),
                                          irTierToString(id->m_tier),
                                          id->m_operands.size());
                    }
                    break;
                }
                case SymbolType::LegalizeRule:
                {
                    if (const auto *rd = sym->getIf<Symbols::LegalizeRuleSymbol>())
                    {
                        std::string matchOp =
                                rd->m_matchPatterns.empty() ? "?" : std::string(rd->m_matchPatterns[0].m_opcode);
                        std::string emitOp = rd->m_expansionSequence.empty()
                                ? "?"
                                : std::string(rd->m_expansionSequence[0].m_opcode);
                        os << std::format("  [{}] ID: {:<3} Scope: {:<2} Name: {:<16} Matches: {:<2} Emits: "
                                          "{:<2} [{} -> {}]\n",
                                          labels.m_text,
                                          sym->getId(),
                                          sym->getDefiningScopeId(),
                                          sym->getName(),
                                          rd->m_matchPatterns.size(),
                                          rd->m_expansionSequence.size(),
                                          matchOp,
                                          emitOp);
                    }
                    else
                    {
                        os << std::format("  [{}] ID: {:<3} Scope: {:<2} Name: {}\n",
                                          labels.m_text,
                                          sym->getId(),
                                          sym->getDefiningScopeId(),
                                          sym->getName());
                    }
                    break;
                }
                default:
                {
                    os << std::format("  [{}] ID: {:<3} Scope: {:<2} Name: {}\n",
                                      labels.m_text,
                                      sym->getId(),
                                      sym->getDefiningScopeId(),
                                      sym->getName());
                    break;
                }
            }
        }
        os << "======================================================================\n";
    }
}

} // namespace Cli
