#include "EzCompilerCommon.h"
#include "CommandLineOptions.h"
#include "DriverContext.h"
#include "FrontendAdapter.h"
#include "CompilationPipeline.h"
#include "EmissionEngine.h"
#include "CliExitCode.h"

#include <exception>
#include <iostream>

namespace
{

/**
 * Runs the compiler driver. Returns 0 on success, 1 for user/compilation errors and 2 for an
 * unexpected fatal exception (aligned with the EzDSL CLI entry point).
 */
int runCompiler(int argc, char **argv)
{
    EzCompiler::CommandLineParser parser;
    EzCompiler::CommandLineOptions options;
    std::string err;

    // An empty error on parse failure means help/version was handled successfully.
    if (!parser.parse(argc, argv, options, err))
    {
        if (!err.empty())
        {
            std::cerr << "error: " << err << "\n";
            return EzCli::kError;
        }
        return EzCli::kSuccess;
    }

    // Set up allocators, diagnostics and target descriptors for the chosen triple.
    EzCompiler::DriverContext ctx(options);
    if (!ctx.initialize())
    {
        std::cerr << "error: failed to initialize compiler target for " << options.target.toString() << "\n";
        return EzCli::kError;
    }

    // Translate the input into generic MIR.
    EzCompiler::MirModuleLoader loader;
    if (!loader.compileSourceToMir(ctx, options.inputFilePath, *ctx.getBuilderContext()))
    {
        return EzCli::kError;
    }

    // Run the middle-end and backend pass pipeline.
    EzCompiler::CompilationPipeline pipeline(ctx);
    if (!pipeline.runPipeline())
    {
        return EzCli::kError;
    }

    // Inspection gates
    if (options.emissionStage == EzCompiler::EmissionStage::GenericMir ||
        options.emissionStage == EzCompiler::EmissionStage::LegalizedMir ||
        options.emissionStage == EzCompiler::EmissionStage::LoweredMir)
    {
        std::cout << pipeline.dumpCurrentMir();
        return EzCli::kSuccess;
    }

    if (options.emissionStage == EzCompiler::EmissionStage::Assembly)
    {
        std::cout << pipeline.dumpAssembly();
        return EzCli::kSuccess;
    }

    // Default: emit object file
    EzCompiler::EmissionEngine emitter(ctx);
    if (!emitter.emitModule(*ctx.getBuilderContext(), options.outputFilePath))
    {
        return EzCli::kError;
    }

    return EzCli::kSuccess;
}

} // namespace

int main(int argc, char **argv)
{
    try
    {
        return runCompiler(argc, argv);
    }
    catch (const std::exception &ex)
    {
        // Contract-violating paths (e.g. an instruction the emitter cannot encode) surface here.
        std::cerr << "Fatal Exception: " << ex.what() << "\n";
        return EzCli::kFatalException;
    }
}
