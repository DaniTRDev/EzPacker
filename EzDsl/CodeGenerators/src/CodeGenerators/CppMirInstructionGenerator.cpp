#include "Ast/IrInstructionDefLangAst.h"
#include "CodeGenerators/CppMirInstructionGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"

namespace CodeGenerators
{

namespace
{

std::string CategoryToString(DSL::Ast::IrInstDef::IrInstCategory category)
{
    using namespace DSL::Ast::IrInstDef;
    switch (category)
    {
        case IrInstCategory::DataMovement:
            return "MirCat_DataMovement";
        case IrInstCategory::Memory:
            return "MirCat_Memory";
        case IrInstCategory::Arithmetic:
            return "MirCat_Arithmetic";
        case IrInstCategory::Bitwise:
            return "MirCat_Bitwise";
        case IrInstCategory::Compare:
            return "MirCat_Compare";
        case IrInstCategory::ControlFlow:
            return "MirCat_ControlFlow";
        case IrInstCategory::Casting:
            return "MirCat_Casting";
        case IrInstCategory::System:
            return "MirCat_System";
        case IrInstCategory::Invalid:
        default:
            return "MirCat_Invalid";
    }
}

std::string TierToString(DSL::Ast::IrInstDef::IrInstTier tier)
{
    using namespace DSL::Ast::IrInstDef;
    switch (tier)
    {
        case IrInstTier::HighLevel:
            return "T(HighLevel)";
        case IrInstTier::PassInternal:
            return "T(PassInternal)";
        case IrInstTier::TargetLow:
            return "T(TargetLow)";
    }
    return "T(HighLevel)";
}

std::string OperandTypeToString(DSL::Ast::IrInstDef::IrOperandType type)
{
    using namespace DSL::Ast::IrInstDef;
    switch (type)
    {
        case IrOperandType::Register:
            return "ExpectedOperandType::Register";
        case IrOperandType::Integer:
            return "ExpectedOperandType::Integer";
        case IrOperandType::FloatingPoint:
            return "ExpectedOperandType::FloatingPoint";
        case IrOperandType::Memory:
            return "ExpectedOperandType::Memory";
        case IrOperandType::Reference:
            return "ExpectedOperandType::Reference";
        case IrOperandType::RuntimeSymbol:
            return "ExpectedOperandType::RuntimeSymbol";
        case IrOperandType::VariadicArgs:
            return "ExpectedOperandType::VariadicArgs";
        case IrOperandType::Immediate:
            return "ExpectedOperandType::Immediate";
        case IrOperandType::RegIntImm:
            return "ExpectedOperandType::RegIntImm";
        case IrOperandType::RegFloatImm:
            return "ExpectedOperandType::RegFloatImm";
        case IrOperandType::RegImm:
            return "ExpectedOperandType::RegImm";
        case IrOperandType::AddressSource:
            return "ExpectedOperandType::AddressSource";
        case IrOperandType::Any:
            return "ExpectedOperandType::Any";
        case IrOperandType::None:
        default:
            return "ExpectedOperandType::None";
    }
}

std::string OperandDirToString(DSL::Ast::IrInstDef::IrOperandDir dir)
{
    using namespace DSL::Ast::IrInstDef;
    switch (dir)
    {
        case IrOperandDir::ArgIn:
            return "MirOperandFlag::Read";
        case IrOperandDir::ArgOut:
            return "MirOperandFlag::Write";
        case IrOperandDir::ArgInOut:
            return "MirOperandFlag::ReadWrite";
    }
    return "MirOperandFlag::Read";
}

std::string FlagsToString(DSL::Ast::IrInstDef::IrInstFlag flags)
{
    using namespace DSL::Ast::IrInstDef;

    if (flags == IrInstFlag::None)
    {
        return "F(None)";
    }

    std::vector<std::string> flagNames;
    auto checkFlag = [&](IrInstFlag flag, std::string_view name)
    {
        if ((static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0)
        {
            flagNames.push_back(std::format("F({})", name));
        }
    };

    checkFlag(IrInstFlag::SizeMatch, "SizeMatch");
    checkFlag(IrInstFlag::DestLarger, "DestLarger");
    checkFlag(IrInstFlag::DestSmaller, "DestSmaller");
    checkFlag(IrInstFlag::ReadsMemory, "ReadsMemory");
    checkFlag(IrInstFlag::WritesMemory, "WritesMemory");
    checkFlag(IrInstFlag::IsTerminator, "IsTerminator");
    checkFlag(IrInstFlag::IsBranch, "IsBranch");
    checkFlag(IrInstFlag::IsCall, "IsCall");
    checkFlag(IrInstFlag::IsReturn, "IsReturn");
    checkFlag(IrInstFlag::HasSideEffect, "HasSideEffect");
    checkFlag(IrInstFlag::IsCommutative, "IsCommutative");
    checkFlag(IrInstFlag::ReadsCPUFlags, "ReadsCPUFlags");
    checkFlag(IrInstFlag::WritesCPUFlags, "WritesCPUFlags");
    checkFlag(IrInstFlag::TreatAsSigned, "TreatAsSigned");
    checkFlag(IrInstFlag::VariadicArgs, "VariadicArgs");

    if (flagNames.empty())
    {
        return "F(None)";
    }

    std::string result;
    for (size_t i = 0; i < flagNames.size(); ++i)
    {
        if (i > 0)
        {
            result += " | ";
        }
        result += flagNames[i];
    }
    return result;
}

} // namespace

CppMirInstructionGenerator::CppMirInstructionGenerator(DiagnosticCollector *collector,
                                                       SymbolTable *table,
                                                       std::filesystem::path outPath) :
    CodeGenerator("CodeGenerators::MirIrInstructionDefs", collector, table, std::move(outPath))
{
}

std::vector<const Symbol *> CppMirInstructionGenerator::collectInstructionSymbols() const
{
    std::vector<const Symbol *> instSymbols;
    if (!m_table)
    {
        return instSymbols;
    }

    for (const Symbol *sym : m_table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::IrInstruction)
        {
            instSymbols.push_back(sym);
        }
    }
    return instSymbols;
}

void CppMirInstructionGenerator::emitInstructionDefs(CppSourceEmitter &emitter,
                                                    const std::vector<const Symbol *> &instSymbols) const
{
    emitter.emitBanner("CppMirInstructionGenerator");
    emitter.emitBlankLine();

    emitter.emitLine("#ifdef INSTRUCTION");
    emitter.emitBlankLine();

    emitter.emitComment("This code is needed not to collide with gtest's TEST macro.");
    emitter.emitLine("#ifdef TEST");
    emitter.emitLine("#define BG_TEST_WAS_DEFINED");
    emitter.emitLine("#pragma push_macro(\"TEST\")");
    emitter.emitLine("#undef TEST");
    emitter.emitLine("#endif");
    emitter.emitBlankLine();

    emitter.emitLine("#define OPERAND_CONSTRAINTS(...) { __VA_ARGS__ }");
    emitter.emitComment("Helper to keep the flags readable without polluting the global namespace");
    emitter.emitLine("#define F(x) MirInstructionFlags::x");
    emitter.emitLine("#define T(x) MirInstructionTier::x");
    emitter.emitBlankLine();

    emitter.emitLine("INSTRUCTION(INVALID, T(HighLevel), MirCat_Invalid, OPERAND_CONSTRAINTS(), F(None))");

    for (const Symbol *sym : instSymbols)
    {
        const auto *data = sym->getIf<Symbols::IrInstructionSymbol>();
        if (!data)
        {
            continue;
        }

        std::string name(sym->getName());
        std::string tier = TierToString(data->m_tier);
        std::string category = CategoryToString(data->m_category);
        std::string flags = FlagsToString(data->m_flags);

        if (data->m_operands.empty())
        {
            emitter.emitBlankLine();
            emitter.emitLine("INSTRUCTION({}, {}, {}, OPERAND_CONSTRAINTS(), {})", name, tier, category, flags);
        }
        else
        {
            std::vector<std::string> constraints;
            for (const auto &op : data->m_operands)
            {
                constraints.push_back(
                        std::format("{{ {}, {} }}", OperandTypeToString(op.m_type), OperandDirToString(op.m_dir)));
            }

            emitter.emitBlankLine();
            emitter.emitLine("INSTRUCTION({},", name);
            emitter.indent();
            emitter.emitLine("{},", tier);
            emitter.emitLine("{},", category);

            if (constraints.size() == 1)
            {
                emitter.emitLine("OPERAND_CONSTRAINTS({}),", constraints[0]);
            }
            else
            {
                emitter.emitLine("OPERAND_CONSTRAINTS(");
                emitter.indent();
                for (size_t i = 0; i < constraints.size(); ++i)
                {
                    if (i + 1 < constraints.size())
                    {
                        emitter.emitLine("{},", constraints[i]);
                    }
                    else
                    {
                        emitter.emitLine("{}),", constraints[i]);
                    }
                }
                emitter.dedent();
            }

            emitter.emitLine("{})", flags);
            emitter.dedent();
        }
    }

    emitter.emitBlankLine();
    emitter.emitLine("#undef T");
    emitter.emitLine("#undef F");
    emitter.emitLine("#undef OPERAND_CONSTRAINTS");
    emitter.emitBlankLine();

    emitter.emitLine("#ifdef BG_TEST_WAS_DEFINED");
    emitter.emitLine("#pragma pop_macro(\"TEST\")");
    emitter.emitLine("#undef BG_TEST_WAS_DEFINED");
    emitter.emitLine("#endif");
    emitter.emitBlankLine();

    emitter.emitLine("#endif // INSTRUCTION");
}

bool CppMirInstructionGenerator::run()
{
    if (!validate())
    {
        return false;
    }

    trace("Generating IR instruction definitions in {}", m_outputPath.string());

    auto instSymbols = collectInstructionSymbols();
    auto targetFilePath = resolveSingleFilePath("MirInstructionSetDefs.h");

    CppSourceEmitter emitter;
    emitInstructionDefs(emitter, instSymbols);

    if (!writeOutput(targetFilePath, emitter.view()))
    {
        return false;
    }

    trace("Successfully synthesized {} IR instructions into {}",
          instSymbols.size(),
          targetFilePath.filename().string());

    return true;
}

bool GenerateMirIrInstructionDefs(DiagnosticCollector *collector,
                                 SymbolTable *table,
                                 std::filesystem::path outPath)
{
    CppMirInstructionGenerator generator(collector, table, std::move(outPath));
    return generator.run();
}

} // namespace CodeGenerators