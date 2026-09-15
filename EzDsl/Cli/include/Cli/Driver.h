#ifndef EZDSL_CLI_DRIVER_H
#define EZDSL_CLI_DRIVER_H

#include "Cli/CommandLineOptions.h"
#include "Cli/InfoDumper.h"
#include "EzDslCliCommon.h"

namespace Cli
{

struct DriverResult
{
    bool success{ false };
    std::string errorMessage;
    std::vector<OutputFileInfo> generatedFiles;
    std::vector<OutputFileInfo> unchangedFiles;
};

class Driver
{
  public:
    explicit Driver(CliOptions options);

    DriverResult run();

  private:
    LanguageDialect detectDialect(const std::filesystem::path &filePath) const;
    GeneratorKind resolveGeneratorKind(LanguageDialect dialect) const;
    std::vector<OutputFileInfo> computeExpectedOutputs(GeneratorKind genKind, const std::filesystem::path &outDir) const;

  private:
    CliOptions m_options;
};

} // namespace Cli

#endif // EZDSL_CLI_DRIVER_H
