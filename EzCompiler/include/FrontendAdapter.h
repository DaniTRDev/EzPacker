#ifndef EZPACKER_FRONTEND_ADAPTER_H
#define EZPACKER_FRONTEND_ADAPTER_H

#include "EzCompilerCommon.h"

class MirBuilderContext;
class MirFunction;

namespace EzCompiler
{

class DriverContext;

/**
 * Abstract interface for front-end language compilers translating source code into generic SSA MIR.
 * Designed to decouple driver orchestration from concrete frontend versions.
 */
class IFrontendAdapter
{
  public:
    virtual ~IFrontendAdapter() = default;

    /**
     * Translates the specified source file into generic MIR functions in outMirCtx.
     */
    virtual bool compileSourceToMir(DriverContext &ctx, std::string_view sourcePath, MirBuilderContext &outMirCtx) = 0;
};

/**
 * Programmatic module loader providing MIR module creation for testing, verification,
 * and intermediate compilation runs.
 */
class MirModuleLoader : public IFrontendAdapter
{
  public:
    MirModuleLoader() = default;
    ~MirModuleLoader() override = default;

    /**
     * Loads a .mir file, or synthesizes a default entrypoint when sourcePath is empty.
     */
    bool compileSourceToMir(DriverContext &ctx, std::string_view sourcePath, MirBuilderContext &outMirCtx) override;

    /**
     * Loads and parses a textual .mir file into outMirCtx using MirParser.
     */
    static bool loadMirFile(DriverContext &ctx, std::string_view mirPath, MirBuilderContext &outMirCtx);

    /**
     * Creates a synthetic test function returning an integer constant (e.g. return 42;).
     */
    static MirFunction *createReturnConstFunction(DriverContext &ctx, std::string_view funcName, int64_t retVal);

    /**
     * Creates a synthetic test function performing arithmetic on two 64-bit integers.
     */
    static MirFunction *createArithmeticFunction(DriverContext &ctx, std::string_view funcName);
};

} // namespace EzCompiler

#endif // EZPACKER_FRONTEND_ADAPTER_H
