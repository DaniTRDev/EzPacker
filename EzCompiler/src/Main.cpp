#include "EzCompilerCommon.h"
#include "CommandLineOptions.h"
#include "DriverContext.h"
#include "FrontendAdapter.h"
#include "CompilationPipeline.h"
#include "EmissionEngine.h"

int main(int argc, char **argv)
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
            return 1;
        }
        return 0;
    }

    // Set up allocators, diagnostics and target descriptors for the chosen triple.
    EzCompiler::DriverContext ctx(options);
    if (!ctx.initialize())
    {
        std::cerr << "error: failed to initialize compiler target for " << options.target.toString() << "\n";
        return 1;
    }

    // Translate the input into generic MIR.
    EzCompiler::MirModuleLoader loader;
    if (!loader.compileSourceToMir(ctx, options.inputFilePath, *ctx.getBuilderContext()))
    {
        return 1;
    }

    // Run the middle-end and backend pass pipeline.
    EzCompiler::CompilationPipeline pipeline(ctx);
    if (!pipeline.runPipeline())
    {
        return 1;
    }

    // Inspection gates
    if (options.emissionStage == EzCompiler::EmissionStage::GenericMir ||
        options.emissionStage == EzCompiler::EmissionStage::LegalizedMir ||
        options.emissionStage == EzCompiler::EmissionStage::LoweredMir)
    {
        std::cout << pipeline.dumpCurrentMir();
        return 0;
    }

    if (options.emissionStage == EzCompiler::EmissionStage::Assembly)
    {
        std::cout << pipeline.dumpAssembly();
        return 0;
    }

    // Default: emit object file
    EzCompiler::EmissionEngine emitter(ctx);
    if (!emitter.emitModule(*ctx.getBuilderContext(), options.outputFilePath))
    {
        return 1;
    }

    return 0;
}
