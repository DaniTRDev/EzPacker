#include "EzLinkerCommon.h"
#include "EzLinkerDriver.h"
#include "SystemLinkerDetector.h"
#include <argparse/argparse.hpp>
#include <iostream>

namespace
{

int runLinker(int argc, char **argv)
{
    argparse::ArgumentParser program("ez-ld", "1.0.0");
    program.add_description("EzPacker automated system linker driver and runtime library linker");

    program.add_argument("inputs")
        .help("Input object files (.o, .obj) or archives (.a, .lib)")
        .remaining();

    program.add_argument("-o", "--output")
        .help("Destination binary path")
        .default_value(std::string(""));

    program.add_argument("--linker")
        .help("Explicit linker executable override (e.g. lld-link, link.exe, ld.lld)")
        .default_value(std::string(""));

    program.add_argument("--target")
        .help("Target architecture/platform triple (e.g. x86_64-pc-windows-msvc, x86_64-unknown-linux-gnu)")
        .default_value(std::string(""));

    program.add_argument("-L")
        .help("Additional library search path")
        .append();

    program.add_argument("-l")
        .help("Additional library to link")
        .append();

    program.add_argument("--entry")
        .help("Custom entry point symbol name")
        .default_value(std::string(""));

    program.add_argument("--subsystem")
        .help("Subsystem (console, windows, etc.)")
        .default_value(std::string(""));

    program.add_argument("--shared")
        .help("Build a shared library or dynamic link library")
        .flag();

    program.add_argument("--static")
        .help("Link statically against runtime libraries")
        .flag();

    program.add_argument("-v", "--verbose")
        .help("Print verbose linker diagnostic logs and synthesized command line")
        .flag();

    program.add_argument("--dry-run")
        .help("Synthesize and display the linker command line without executing")
        .flag();

    program.add_argument("--nodefaultlibs")
        .help("Do not automatically link C standard libraries")
        .flag();

    program.add_argument("--no-compiler-rt")
        .help("Do not automatically link compiler-rt / legalizer helpers")
        .flag();

    program.add_argument("--no-exception-rt")
        .help("Do not automatically link EzExceptionRuntime")
        .flag();

    program.add_argument("-Xlinker")
        .help("Pass raw argument to the system linker")
        .append();

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        std::cerr << "ez-ld: error: " << err.what() << "\n";
        std::cerr << program;
        return 1;
    }

    EzLinker::LinkerOptions opts;

    opts.outputFile = program.get<std::string>("-o");
    opts.customLinker = program.get<std::string>("--linker");
    opts.targetTriple = program.get<std::string>("--target");
    opts.entryPoint = program.get<std::string>("--entry");
    opts.subsystem = program.get<std::string>("--subsystem");
    opts.isShared = program.get<bool>("--shared");
    opts.isStatic = program.get<bool>("--static");
    opts.verbose = program.get<bool>("-v");
    opts.dryRun = program.get<bool>("--dry-run");
    opts.noDefaultLibs = program.get<bool>("--nodefaultlibs");
    opts.noCompilerRt = program.get<bool>("--no-compiler-rt");
    opts.noExceptionRuntime = program.get<bool>("--no-exception-rt");

    try
    {
        opts.inputFiles = program.get<std::vector<std::string>>("inputs");
        for (size_t i = 0; i < opts.inputFiles.size();)
        {
            if (opts.inputFiles[i] == "-o" && i + 1 < opts.inputFiles.size())
            {
                if (opts.outputFile.empty())
                {
                    opts.outputFile = opts.inputFiles[i + 1];
                }
                opts.inputFiles.erase(opts.inputFiles.begin() + i, opts.inputFiles.begin() + i + 2);
            }
            else
            {
                ++i;
            }
        }
    }
    catch (...)
    {
        // No inputs supplied
    }

    if (opts.inputFiles.empty() && !opts.dryRun)
    {
        std::cerr << "ez-ld: error: no input files provided\n";
        return 1;
    }

    try
    {
        opts.libSearchPaths = program.get<std::vector<std::string>>("-L");
    }
    catch (...) {}

    try
    {
        opts.libraries = program.get<std::vector<std::string>>("-l");
    }
    catch (...) {}

    try
    {
        opts.rawLinkerArgs = program.get<std::vector<std::string>>("-Xlinker");
    }
    catch (...) {}

    EzLinker::EzLinkerDriver driver;
    EzLinker::LinkResult result = driver.link(opts);

    if (opts.dryRun)
    {
        std::cout << result.commandLine << "\n";
        return 0;
    }

    if (!result.output.empty())
    {
        std::cout << result.output;
    }

    if (!result.success)
    {
        std::cerr << "ez-ld: linking failed with exit code " << result.exitCode << "\n";
        return result.exitCode != 0 ? result.exitCode : 1;
    }

    return 0;
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        return runLinker(argc, argv);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "ez-ld: fatal error: " << ex.what() << "\n";
        return 2;
    }
}
