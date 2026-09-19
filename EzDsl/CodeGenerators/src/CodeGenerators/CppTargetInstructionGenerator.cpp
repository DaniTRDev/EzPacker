#include "CodeGenerators/CppTargetInstructionGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetSymbols.h"
#include <algorithm>
#include <cctype>

namespace CodeGenerators
{

namespace
{

std::string ToUpper(std::string_view s)
{
    std::string res(s);
    for (char &c : res)
    {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return res;
}

std::string DirectionToFlag(DSL::Ast::TargetInstDef::OperandDirection dir)
{
    switch (dir)
    {
        case DSL::Ast::TargetInstDef::OperandDirection::Out:
            return "MirOperandFlag::Write";
        case DSL::Ast::TargetInstDef::OperandDirection::InOut:
            return "MirOperandFlag::ReadWrite";
        case DSL::Ast::TargetInstDef::OperandDirection::In:
        default:
            return "MirOperandFlag::Read";
    }
}

std::string FlagsToCpp(const std::pmr::vector<std::string_view> &flags)
{
    if (flags.empty())
    {
        return "MirInstructionFlags::None";
    }

    std::string res;
    for (size_t i = 0; i < flags.size(); ++i)
    {
        if (i > 0)
        {
            res += " | ";
        }
        res += "MirInstructionFlags::" + std::string(flags[i]);
    }
    return res;
}

} // namespace

CppTargetInstructionGenerator::CppTargetInstructionGenerator(DiagnosticCollector *collector,
                                                             SymbolTable *table,
                                                             std::filesystem::path outPath,
                                                             std::string targetName) :
    CodeGenerator("CodeGenerators::TargetInstructions", collector, table, std::move(outPath)),
    m_targetName(std::move(targetName))
{
    if (m_targetName.empty())
    {
        m_targetName = "Target";
    }
}

std::vector<const Symbol *> CppTargetInstructionGenerator::collectInstructionSymbols() const
{
    std::vector<const Symbol *> results;
    if (!m_table)
    {
        return results;
    }

    for (const Symbol *sym : m_table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::TargetInstruction && sym->hasData<Symbols::TargetInstructionSymbol>())
        {
            results.push_back(sym);
        }
    }

    return results;
}

bool CppTargetInstructionGenerator::run()
{
    if (!validate())
    {
        return false;
    }

    std::string defaultBaseName = std::format("{}TargetInstructionTable", m_targetName);
    auto [headerPath, sourcePath] = resolveHeaderAndSourcePaths(defaultBaseName);

    CppSourceEmitter headerEmitter;
    CppSourceEmitter sourceEmitter;

    emitHeader(headerEmitter);
    emitSource(sourceEmitter);

    bool headerOk = writeOutput(headerPath, headerEmitter.str());
    bool sourceOk = writeOutput(sourcePath, sourceEmitter.str());

    return headerOk && sourceOk;
}

void CppTargetInstructionGenerator::emitHeader(CppSourceEmitter &emitter) const
{
    std::string guard = std::format("EZTRIPLE_{}_TARGET_INSTRUCTION_TABLE_H", ToUpper(m_targetName));
    emitter.emitIncludeGuardStart(guard);
    emitter.emitBlankLine();
    emitter.emitBanner("CppTargetInstructionGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude("Instruction/MirTargetInstructionDesc.h");
    emitter.emitInclude("Descriptors/TargetDesc.h");
    emitter.emitLine("#include <cstdint>");
    emitter.emitLine("#include <cstddef>");
    emitter.emitBlankLine();

    std::string ns = std::format("EzTriple::{}TargetInst", m_targetName);
    {
        auto nsScope = emitter.enterNamespace(ns);

        auto instSymbols = collectInstructionSymbols();

        emitter.emitLine("enum OpCode : size_t");
        {
            auto enumScope = emitter.enterBlock();
            for (size_t i = 0; i < instSymbols.size(); ++i)
            {
                const auto *sym = instSymbols[i];
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();
                emitter.emitLine("{} = {},", data->m_name, i + 1);
            }
            emitter.emitLine("OPCODE_COUNT = {}", instSymbols.size() + 1);
        }
        emitter.emitLine(";");
        emitter.emitBlankLine();

        emitter.emitLine("const MirTargetInstructionDesc *getTargetDesc(OpCode op);");
        emitter.emitLine("void initializeTargetInstructionTable(::TargetDesc *target);");
    }

    emitter.emitBlankLine();
    emitter.emitIncludeGuardEnd(guard);
}

void CppTargetInstructionGenerator::emitSource(CppSourceEmitter &emitter) const
{
    emitter.emitBanner("CppTargetInstructionGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude(std::format("{}TargetInstructionTable.h", m_targetName));
    emitter.emitInclude("Descriptors/TargetDesc.h");
    emitter.emitInclude("Operand/MirRegisterClass.h");
    emitter.emitInclude("Operand/MirRegisterBank.h");
    emitter.emitLine("#include <string_view>");
    emitter.emitBlankLine();

    std::string ns = std::format("EzTriple::{}TargetInst", m_targetName);
    {
        auto nsScope = emitter.enterNamespace(ns);

        auto instSymbols = collectInstructionSymbols();

        emitter.emitLine("static MirTargetInstructionDesc s_descs[] =");
        {
            auto arrayScope = emitter.enterBlock();
            for (size_t i = 0; i < instSymbols.size(); ++i)
            {
                const auto *sym = instSymbols[i];
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();

                // Build operand flags string
                std::string flagsList = "{ ";
                for (size_t opIdx = 0; opIdx < data->m_operands.size(); ++opIdx)
                {
                    if (opIdx > 0) flagsList += ", ";
                    flagsList += DirectionToFlag(data->m_operands[opIdx].m_direction);
                }
                flagsList += " }";

                // Build dummy operand classes string (nullptrs)
                std::string classList = "{ ";
                for (size_t opIdx = 0; opIdx < data->m_operands.size(); ++opIdx)
                {
                    if (opIdx > 0) classList += ", ";
                    classList += "nullptr";
                }
                classList += " }";

                std::string targetFlagsStr = FlagsToCpp(data->m_flags);

                emitter.emitLine("MirTargetInstructionDesc(");
                emitter.indent();
                emitter.emitLine("\"{}\",", data->m_name);
                emitter.emitLine("static_cast<size_t>({}),", data->m_name);
                emitter.emitLine("{},", flagsList);
                emitter.emitLine("{},", classList);
                emitter.emitLine("{},", "{ /* implicit defs */ }");
                emitter.emitLine("{},", "{ /* implicit uses */ }");
                emitter.emitLine("{}", targetFlagsStr);
                emitter.dedent();
                emitter.emitLine("){},", (i + 1 == instSymbols.size()) ? "" : "");
            }
        }
        emitter.emitLine(";");
        emitter.emitBlankLine();

        // getTargetDesc
        emitter.emitLine("const MirTargetInstructionDesc *getTargetDesc(OpCode op)");
        {
            auto fnScope = emitter.enterBlock();
            emitter.emitLine("if (op == 0 || static_cast<size_t>(op) >= static_cast<size_t>(OPCODE_COUNT))");
            {
                auto ifScope = emitter.enterBlock();
                emitter.emitLine("return nullptr;");
            }
            emitter.emitLine("return &s_descs[static_cast<size_t>(op) - 1];");
        }
        emitter.emitBlankLine();

        // initializeTargetInstructionTable
        emitter.emitLine("void initializeTargetInstructionTable(::TargetDesc *target)");
        {
            auto fnScope = emitter.enterBlock();
            emitter.emitLine("if (!target) return;");
            emitter.emitBlankLine();
            emitter.emitLine("for (size_t i = 0; i < static_cast<size_t>(OPCODE_COUNT) - 1; ++i)");
            {
                auto loopScope = emitter.enterBlock();
                emitter.emitLine("s_descs[i].setEncodingId(i + 1);");
            }
            emitter.emitBlankLine();
            emitter.emitLine("auto findClass = [&](std::string_view name) -> MirRegisterClass * {");
            emitter.indent();
            emitter.emitLine("for (auto *bank : target->getAvailableRegisterBanks())");
            emitter.emitLine("{");
            emitter.indent();
            emitter.emitLine("if (!bank) continue;");
            emitter.emitLine("if (auto *rc = bank->getClass(name)) return rc;");
            emitter.dedent();
            emitter.emitLine("}");
            emitter.emitLine("return nullptr;");
            emitter.dedent();
            emitter.emitLine("};");
            emitter.emitBlankLine();

            for (size_t i = 0; i < instSymbols.size(); ++i)
            {
                const auto *sym = instSymbols[i];
                const auto *data = sym->getIf<Symbols::TargetInstructionSymbol>();
                for (size_t opIdx = 0; opIdx < data->m_operands.size(); ++opIdx)
                {
                    const auto &op = data->m_operands[opIdx];
                    if (!op.m_regClassOrType.empty())
                    {
                        emitter.emitLine("s_descs[{}].setOperandClass({}, findClass(\"{}\"));",
                                         i, opIdx, op.m_regClassOrType);
                    }
                }
            }
        }
    }
}

bool GenerateTargetInstructionTable(DiagnosticCollector *collector,
                                    SymbolTable *table,
                                    std::filesystem::path outPath,
                                    std::string targetName)
{
    CppTargetInstructionGenerator generator(collector, table, std::move(outPath), std::move(targetName));
    return generator.run();
}

} // namespace CodeGenerators
