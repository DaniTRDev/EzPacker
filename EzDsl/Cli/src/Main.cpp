#include "Cli/CommandLineOptions.h"
#include "Cli/Driver.h"
#include "CliExitCode.h"
#include "EzTargetsX86_64Dsl.h"

#include <iostream>

// Process entry point: parse arguments, run the driver, and map failures to shared exit codes.
int main(int argc, char *argv[])
{
    try
    {
        // Register the target DSL plugins available to this tool before any generation.
        EzTargets::X86_64::registerDsl();

        Cli::CommandLineParser parser;
        std::string errorMessage;
        auto options = parser.parse(argc, argv, errorMessage);

        // A missing value signals either a parse error (message set) or an explicit help/version exit.
        if (!options.has_value())
        {
            if (!errorMessage.empty())
            {
                std::cerr << "Error: " << errorMessage << "\n\n";
                std::cerr << parser.getHelp() << "\n";
                return EzCli::kError;
            }
            // Help or version was requested and displayed
            return EzCli::kSuccess;
        }

        Cli::Driver driver(std::move(*options));
        auto result = driver.run();

        if (!result.success)
        {
            if (!result.errorMessage.empty())
            {
                std::cerr << "Error: " << result.errorMessage << "\n";
            }
            return EzCli::kError;
        }

        return EzCli::kSuccess;
    }
    catch (const std::exception &ex)
    {
        // Unexpected failures surface as a fatal error and a distinct exit code.
        std::cerr << "Fatal Exception: " << ex.what() << "\n";
        return EzCli::kFatalException;
    }
}
