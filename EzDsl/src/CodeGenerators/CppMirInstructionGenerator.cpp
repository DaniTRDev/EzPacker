#include "CodeGenerators/CppMirInstructionGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/IrInstructionSymbol.h"

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

void WriteFileIfChanged(const std::filesystem::path &filePath, const std::string &newContent)
{
    if (std::filesystem::exists(filePath))
    {
        std::ifstream currentFile(filePath, std::ios::in | std::ios::binary);
        if (currentFile.is_open())
        {
            std::ostringstream ss;
            ss << currentFile.rdbuf();
            if (ss.str() == newContent)
            {
                return;
            }
        }
    }

    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc | std::ios::binary);
    if (outFile.is_open())
    {
        outFile << newContent;
    }
}

void EmitInstructionDefs(std::ostream &out, const std::vector<const Symbol *> &instSymbols)
{
    out << R"(#ifdef INSTRUCTION

// This code is needed not to collide with gtest's TEST macro.
#ifdef TEST
#define BG_TEST_WAS_DEFINED
#pragma push_macro("TEST")
#undef TEST
#endif

#define OPERAND_CONSTRAINTS(...)                                                                                       \
    {                                                                                                                  \
        __VA_ARGS__                                                                                                    \
    }
// Helper to keep the flags readable without polluting the global namespace
#define F(x) MirInstructionFlags::x
#define T(x) MirInstructionTier::x

INSTRUCTION(INVALID, T(HighLevel), MirCat_Invalid, OPERAND_CONSTRAINTS(), F(None))
)";

    for (const Symbol *sym : instSymbols)
    {
        const auto *data = sym->getIf<Sema::Symbols::IrInstructionSymbol>();
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
            out << std::format("\nINSTRUCTION({}, {}, {}, OPERAND_CONSTRAINTS(), {})\n", name, tier, category, flags);
        }
        else
        {
            std::vector<std::string> constraints;
            for (const auto &op : data->m_operands)
            {
                constraints.push_back(
                        std::format("{{ {}, {} }}", OperandTypeToString(op.m_type), OperandDirToString(op.m_dir)));
            }

            out << std::format("\nINSTRUCTION({},\n"
                               "            {},\n"
                               "            {},\n",
                               name,
                               tier,
                               category);

            if (constraints.size() == 1)
            {
                out << std::format("            OPERAND_CONSTRAINTS({}),\n", constraints[0]);
            }
            else
            {
                out << "            OPERAND_CONSTRAINTS(";
                for (size_t i = 0; i < constraints.size(); ++i)
                {
                    if (i == 0)
                    {
                        out << constraints[i] << ",\n";
                    }
                    else if (i + 1 < constraints.size())
                    {
                        out << "                                " << constraints[i] << ",\n";
                    }
                    else
                    {
                        out << "                                " << constraints[i] << "),\n";
                    }
                }
            }

            out << std::format("            {})\n", flags);
        }
    }

    out << R"(
#undef T
#undef F
#undef OPERAND_CONSTRAINTS

#ifdef BG_TEST_WAS_DEFINED
#pragma pop_macro("TEST")
#undef BG_TEST_WAS_DEFINED
#endif

#endif // INSTRUCTION
)";
}

} // namespace

bool GenerateMirIrInstructionDefs(DiagnosticCollector *collector, SymbolTable *table, std::filesystem::path outPath)
{
    constexpr auto genName = "CodeGenerators::MirIrInstructionDefs";

    if (!collector)
    {
        return false;
    }

    if (!table)
    {
        collector->error(genName, "Cannot generate IR instruction definitions with a null SymbolTable.");
        return false;
    }

    collector->trace(genName, "Generating IR instruction definitions in {}", outPath.string());

    std::vector<const Symbol *> instSymbols;
    for (const Symbol *sym : table->getSymbols())
    {
        if (sym && sym->getType() == SymbolType::IrInstruction)
        {
            instSymbols.push_back(sym);
        }
    }

    std::filesystem::path targetFilePath;
    if (std::filesystem::is_directory(outPath) || !outPath.has_extension())
    {
        std::error_code ec;
        std::filesystem::create_directories(outPath, ec);
        if (ec)
        {
            collector->error(genName, "Failed to create directory {}: {}", outPath.string(), ec.message());
            return false;
        }
        targetFilePath = outPath / "MirInstructionSetDefs.h";
    }
    else
    {
        if (outPath.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(outPath.parent_path(), ec);
            if (ec)
            {
                collector->error(genName,
                                 "Failed to create directory {}: {}",
                                 outPath.parent_path().string(),
                                 ec.message());
                return false;
            }
        }
        targetFilePath = outPath;
    }

    std::ostringstream outStream;
    EmitInstructionDefs(outStream, instSymbols);
    WriteFileIfChanged(targetFilePath, outStream.str());

    collector->trace(genName,
                     "Successfully synthesized {} IR instructions into {}",
                     instSymbols.size(),
                     targetFilePath.filename().string());

    return true;
}

} // namespace CodeGenerators