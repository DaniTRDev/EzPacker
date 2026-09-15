#include "Cli/InfoDumper.h"

#include "Ast/CommonAstNodes.h"
#include "Ast/IrInstructionDefLangAst.h"
#include "Ast/LegalizeActionDefLangAst.h"
#include "Ast/LegalizeRuleDefLangAst.h"
#include "Ast/TypeDefLangAst.h"

#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/IrSymbols.h"
#include "Sema/Symbols/LegalizeSymbols.h"
#include "Sema/Symbols/TypeSymbols.h"

namespace Cli
{

std::string InfoDumper::escapeJson(std::string_view str)
{
    std::string res;
    res.reserve(str.size() + 8);
    for (char c : str)
    {
        switch (c)
        {
            case '\"': res += "\\\""; break;
            case '\\': res += "\\\\"; break;
            case '\b': res += "\\b"; break;
            case '\f': res += "\\f"; break;
            case '\n': res += "\\n"; break;
            case '\r': res += "\\r"; break;
            case '\t': res += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    res += std::format("\\u{:04x}", static_cast<unsigned int>(c));
                }
                else
                {
                    res += c;
                }
                break;
        }
    }
    return res;
}

std::string InfoDumper::typeKindToString(int kind)
{
    switch (static_cast<DSL::Ast::TypeDef::TypeKind>(kind))
    {
        case DSL::Ast::TypeDef::TypeKind::Integer: return "Integer";
        case DSL::Ast::TypeDef::TypeKind::FloatingPoint: return "FloatingPoint";
        case DSL::Ast::TypeDef::TypeKind::Void: return "Void";
        case DSL::Ast::TypeDef::TypeKind::BindingToken: return "BindingToken";
        case DSL::Ast::TypeDef::TypeKind::Pointer: return "Pointer";
    }
    return "Unknown";
}

std::string InfoDumper::irCategoryToString(int cat)
{
    switch (static_cast<DSL::Ast::IrInstDef::IrInstCategory>(cat))
    {
        case DSL::Ast::IrInstDef::IrInstCategory::Invalid: return "Invalid";
        case DSL::Ast::IrInstDef::IrInstCategory::DataMovement: return "DataMovement";
        case DSL::Ast::IrInstDef::IrInstCategory::Memory: return "Memory";
        case DSL::Ast::IrInstDef::IrInstCategory::Arithmetic: return "Arithmetic";
        case DSL::Ast::IrInstDef::IrInstCategory::Bitwise: return "Bitwise";
        case DSL::Ast::IrInstDef::IrInstCategory::Compare: return "Compare";
        case DSL::Ast::IrInstDef::IrInstCategory::ControlFlow: return "ControlFlow";
        case DSL::Ast::IrInstDef::IrInstCategory::Casting: return "Casting";
        case DSL::Ast::IrInstDef::IrInstCategory::System: return "System";
    }
    return "Unknown";
}

std::string InfoDumper::irTierToString(int tier)
{
    switch (static_cast<DSL::Ast::IrInstDef::IrInstTier>(tier))
    {
        case DSL::Ast::IrInstDef::IrInstTier::HighLevel: return "HighLevel";
        case DSL::Ast::IrInstDef::IrInstTier::PassInternal: return "PassInternal";
        case DSL::Ast::IrInstDef::IrInstTier::TargetLow: return "TargetLow";
    }
    return "Unknown";
}

std::string InfoDumper::irOperandDirToString(int dir)
{
    switch (static_cast<DSL::Ast::IrInstDef::IrOperandDir>(dir))
    {
        case DSL::Ast::IrInstDef::IrOperandDir::ArgIn: return "IN";
        case DSL::Ast::IrInstDef::IrOperandDir::ArgOut: return "OUT";
        case DSL::Ast::IrInstDef::IrOperandDir::ArgInOut: return "INOUT";
    }
    return "IN";
}

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

std::vector<std::string> InfoDumper::irFlagsToStrings(uint32_t flagMask)
{
    std::vector<std::string> flags;
    auto check = [&](DSL::Ast::IrInstDef::IrInstFlag f, const char *name) {
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
            os << std::format("    - [{}] {} (exists: {})\n", out.role, out.path.string(),
                              out.exists ? "yes" : "no");
        }
        os << "======================================================================\n";
    }
}

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
            os << std::format("      \"kind\": \"{}\",\n", typeKindToString(static_cast<int>(t.m_kind)));
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
            os << std::format("  Type: {:<12} Kind: {:<14} BitWidth: {:<6} Alignment: {}\n", t.m_name.m_node,
                              typeKindToString(static_cast<int>(t.m_kind)), bw, al);
        }
    }
}

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
                              irCategoryToString(static_cast<int>(inst.m_body.m_category)));
            os << std::format("      \"tier\": \"{}\",\n", irTierToString(static_cast<int>(inst.m_body.m_tier)));

            // Flags
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
                                  irOperandDirToString(static_cast<int>(op.m_dir)));
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
            os << std::format("  Instruction: {} [Category: {}, Tier: {}]\n", inst.m_name.m_node,
                              irCategoryToString(static_cast<int>(inst.m_body.m_category)),
                              irTierToString(static_cast<int>(inst.m_body.m_tier)));

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
                    os << std::format("      - {:<16} {:<12} [{}]\n", op.m_name.m_node,
                                      irOperandTypeToString(static_cast<uint16_t>(op.m_type)),
                                      irOperandDirToString(static_cast<int>(op.m_dir)));
                }
            }
        }
    }
}

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

void InfoDumper::dumpLegalizeRuleAst(const DSL::Ast::LegalizeRuleDef::LegalizeRuleFile &file,
                                   OutputFormat format,
                                   std::ostream &os)
{
    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"rules\": [\n";
        for (size_t i = 0; i < file.m_rules.size(); ++i)
        {
            const auto &r = file.m_rules[i];
            os << "    {\n";
            os << std::format("      \"name\": \"{}\",\n", escapeJson(r.m_ruleName.m_node));
            os << std::format("      \"match_count\": {},\n", r.m_matchClauses.size());
            os << std::format("      \"emit_count\": {}\n", r.m_emitClauses.size());
            os << (i + 1 < file.m_rules.size() ? "    },\n" : "    }\n");
        }
        os << "  ]\n";
        os << "}\n";
    }
    else
    {
        os << "=== LegalizeRule AST Summary (" << file.m_rules.size() << " rewrite rules) ===\n";
        for (const auto &r : file.m_rules)
        {
            os << std::format("  Rule: {} (matches: {}, emits: {})\n", r.m_ruleName.m_node,
                              r.m_matchClauses.size(), r.m_emitClauses.size());
        }
    }
}

void InfoDumper::dumpSymbols(const SymbolTable &symbolTable, OutputFormat format, std::ostream &os)
{
    const auto &symbols = symbolTable.getSymbols();

    if (format == OutputFormat::Json)
    {
        os << "{\n";
        os << "  \"symbols\": [\n";
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

            switch (sym->getType())
            {
                case SymbolType::Type:
                {
                    os << "      \"type\": \"Type\",\n";
                    if (const auto *td = sym->getIf<Symbols::TypeSymbol>())
                    {
                        os << std::format("      \"kind\": \"{}\",\n",
                                          typeKindToString(static_cast<int>(td->m_kind)));
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
                    os << "      \"type\": \"IrInstruction\",\n";
                    if (const auto *id = sym->getIf<Symbols::IrInstructionSymbol>())
                    {
                        os << std::format("      \"category\": \"{}\",\n",
                                          irCategoryToString(static_cast<int>(id->m_category)));
                        os << std::format("      \"tier\": \"{}\",\n",
                                          irTierToString(static_cast<int>(id->m_tier)));
                        os << std::format("      \"operand_count\": {}\n", id->m_operands.size());
                    }
                    else
                    {
                        os << "      \"details\": null\n";
                    }
                    break;
                }
                case SymbolType::IrInstructionOperand:
                {
                    os << "      \"type\": \"IrInstructionOperand\",\n";
                    os << "      \"details\": null\n";
                    break;
                }
                case SymbolType::LegalizeAction:
                {
                    os << "      \"type\": \"LegalizeAction\",\n";
                    os << "      \"details\": null\n";
                    break;
                }
                case SymbolType::LegalizeRule:
                {
                    os << "      \"type\": \"LegalizeRule\",\n";
                    os << "      \"details\": null\n";
                    break;
                }
                default:
                {
                    os << "      \"type\": \"Other\",\n";
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

            switch (sym->getType())
            {
                case SymbolType::Type:
                {
                    if (const auto *td = sym->getIf<Symbols::TypeSymbol>())
                    {
                        os << std::format("  [Type] ID: {:<3} Scope: {:<2} Name: {:<12} Kind: {:<12} Width: {:<4} Align: {:<4} CompactID: {}\n",
                                          sym->getId(), sym->getDefiningScopeId(), sym->getName(),
                                          typeKindToString(static_cast<int>(td->m_kind)), td->m_bitWidth,
                                          td->m_alignment, td->m_compactId);
                    }
                    break;
                }
                case SymbolType::IrInstruction:
                {
                    if (const auto *id = sym->getIf<Symbols::IrInstructionSymbol>())
                    {
                        os << std::format("  [Instruction] ID: {:<3} Scope: {:<2} Name: {:<16} Category: {:<14} Tier: {:<12} Operands: {}\n",
                                          sym->getId(), sym->getDefiningScopeId(), sym->getName(),
                                          irCategoryToString(static_cast<int>(id->m_category)),
                                          irTierToString(static_cast<int>(id->m_tier)), id->m_operands.size());
                    }
                    break;
                }
                case SymbolType::LegalizeAction:
                {
                    os << std::format("  [LegalizeAction] ID: {:<3} Scope: {:<2} Name: {}\n", sym->getId(),
                                      sym->getDefiningScopeId(), sym->getName());
                    break;
                }
                case SymbolType::LegalizeRule:
                {
                    os << std::format("  [LegalizeRule] ID: {:<3} Scope: {:<2} Name: {}\n", sym->getId(),
                                      sym->getDefiningScopeId(), sym->getName());
                    break;
                }
                default:
                {
                    os << std::format("  [Symbol] ID: {:<3} Scope: {:<2} Name: {}\n", sym->getId(),
                                      sym->getDefiningScopeId(), sym->getName());
                    break;
                }
            }
        }
        os << "======================================================================\n";
    }
}

} // namespace Cli
