#include "Cli/CommandLineOptions.h"
#include "Cli/Driver.h"

#include <iostream>

int main(int argc, char *argv[])
{
    try
    {
        Cli::CommandLineParser parser;
        std::string errorMessage;
        auto options = parser.parse(argc, argv, errorMessage);

        if (!options.has_value())
        {
            if (!errorMessage.empty())
            {
                std::cerr << "Error: " << errorMessage << "\n\n";
                std::cerr << parser.getHelp() << "\n";
                return 1;
            }
            // Help or version was requested and displayed
            return 0;
        }

        Cli::Driver driver(std::move(*options));
        auto result = driver.run();

        if (!result.success)
        {
            if (!result.errorMessage.empty())
            {
                std::cerr << "Error: " << result.errorMessage << "\n";
            }
            return 1;
        }

        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal Exception: " << ex.what() << "\n";
        return 2;
    }
}
