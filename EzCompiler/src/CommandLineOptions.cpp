#include "CommandLineOptions.h"
#include <iostream>

namespace EzCompiler
{

CommandLineParser::CommandLineParser()
{
    setupArguments();
}

void CommandLineParser::setupArguments()
{
    m_program = std::make_unique<argparse::ArgumentParser>("ezc", "1.0.0", argparse::default_arguments::help);
    m_program->add_description("EzPacker Ahead-Of-Time Compiler Driver");

    m_program->add_argument("input")
        .help("Input source file (.ez) or intermediate representation (.mir)")
        .default_value(std::string(""))
        .nargs(argparse::nargs_pattern::optional);

    m_program->add_argument("-o", "--output")
        .help("Output file path (default: a.out or a.obj depending on target)")
        .metavar("<path>")
        .default_value(std::string(""));

    m_program->add_argument("-c")
        .help("Compile and assemble, but do not link")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("-S")
        .help("Stop after compilation; emit assembly text")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("--target")
        .help("Target architecture and OS triple (e.g. x86_64-linux-gnu, x86_64-windows-msvc)")
        .metavar("<triple>")
        .default_value(std::string(""));

    m_program->add_argument("--emit-mir")
        .help("Dump generic SSA MIR after frontend lowering")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("--emit-legalized-mir")
        .help("Dump MIR after type and opcode legalization")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("--emit-lowered-mir")
        .help("Dump target-lowered MIR post register allocation and frame lowering")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("--emit-obj")
        .help("Emit native object format (ELF64 or PE-COFF)")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("-O0")
        .help("Disable optimizations")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("-O1")
        .help("Basic optimizations")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("-O2")
        .help("Full optimizations (SSA, SIB folding, coloring)")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("-Os")
        .help("Optimize for code size")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("-v", "--verbose")
        .help("Enable verbose compiler diagnostic logging")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("--print-passes")
        .help("Print compiler pass names in execution order")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("--time-passes")
        .help("Report execution time per compiler pass")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("-fPIC")
        .help("Generate position-independent code")
        .default_value(false)
        .implicit_value(true);

    m_program->add_argument("--diag-level")
        .help("Minimum diagnostic severity threshold (error, warning, info, trace, debug)")
        .metavar("<level>")
        .default_value(std::string("warning"));

    m_program->add_argument("-V", "--version")
        .help("Print version information")
        .default_value(false)
        .implicit_value(true);
}

bool CommandLineParser::parse(int argc, const char *const *argv, CommandLineOptions &outOptions, std::string &outError)
{
    std::vector<std::string> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
    {
        args.emplace_back(argv[i]);
    }
    return parse(args, outOptions, outError);
}

bool CommandLineParser::parse(const std::vector<std::string> &args, CommandLineOptions &outOptions, std::string &outError)
{
    setupArguments();
    outOptions = CommandLineOptions();

    try
    {
        m_program->parse_args(args);
    }
    catch (const std::exception &err)
    {
        outError = err.what();
        return false;
    }

    if (m_program->get<bool>("-V"))
    {
        printVersion();
        return false;
    }

    // Input file
    outOptions.inputFilePath = m_program->get<std::string>("input");

    // Target triple
    std::string targetStr = m_program->get<std::string>("--target");
    if (!targetStr.empty())
    {
        outOptions.target = TargetTriple::parse(targetStr);
    }
    else
    {
        outOptions.target = TargetTriple::getHostTriple();
    }

    // Output path
    outOptions.outputFilePath = m_program->get<std::string>("-o");
    if (outOptions.outputFilePath.empty())
    {
        if (outOptions.target.isWindows())
        {
            outOptions.outputFilePath = "a.obj";
        }
        else
        {
            outOptions.outputFilePath = "a.o";
        }
    }

    // Emission stage
    if (m_program->get<bool>("--emit-mir"))
    {
        outOptions.emissionStage = EmissionStage::GenericMir;
    }
    else if (m_program->get<bool>("--emit-legalized-mir"))
    {
        outOptions.emissionStage = EmissionStage::LegalizedMir;
    }
    else if (m_program->get<bool>("--emit-lowered-mir"))
    {
        outOptions.emissionStage = EmissionStage::LoweredMir;
    }
    else if (m_program->get<bool>("-S"))
    {
        outOptions.emissionStage = EmissionStage::Assembly;
    }
    else
    {
        outOptions.emissionStage = EmissionStage::Object;
    }

    // Optimizations
    if (m_program->get<bool>("-O2"))
    {
        outOptions.optLevel = OptimizationLevel::O2;
    }
    else if (m_program->get<bool>("-O1"))
    {
        outOptions.optLevel = OptimizationLevel::O1;
    }
    else if (m_program->get<bool>("-Os"))
    {
        outOptions.optLevel = OptimizationLevel::Os;
    }
    else
    {
        outOptions.optLevel = OptimizationLevel::O0;
    }

    outOptions.verbose = m_program->get<bool>("-v");
    outOptions.printPasses = m_program->get<bool>("--print-passes");
    outOptions.timePasses = m_program->get<bool>("--time-passes");
    outOptions.isPositionIndependent = m_program->get<bool>("-fPIC");
    outOptions.compileOnly = m_program->get<bool>("-c");

    // Diag level
    std::string diagLvl = m_program->get<std::string>("--diag-level");
    if (diagLvl == "error")
    {
        outOptions.diagThreshold = DiagnosticMessageType::Diag_Error;
    }
    else if (diagLvl == "warning")
    {
        outOptions.diagThreshold = DiagnosticMessageType::Diag_Warning;
    }
    else if (diagLvl == "trace")
    {
        outOptions.diagThreshold = DiagnosticMessageType::Diag_Trace;
    }
    else if (diagLvl == "debug")
    {
        outOptions.diagThreshold = DiagnosticMessageType::Diag_Debug;
    }

    return true;
}

void CommandLineParser::printHelp() const
{
    std::cout << m_program->help().str() << "\n";
}

void CommandLineParser::printVersion() const
{
    std::cout << "ezc (EzPacker Compiler Suite) 1.0.0\n";
    std::cout << "Copyright (C) 2026 DaniTRDev / EzPacker Project\n";
}

} // namespace EzCompiler
