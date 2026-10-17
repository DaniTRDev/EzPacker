#ifndef EZPACKER_C_FRONTEND_ADAPTER_H
#define EZPACKER_C_FRONTEND_ADAPTER_H

#include "EzCompilerCommon.h"
#include "FrontendAdapter.h"
#include "CFrontend/ChibiccWrapper.h"

namespace EzCompiler
{

class DriverContext;

/**
 * Concrete IFrontendAdapter implementation that consumes C11 source files (.c, .i)
 * via embedded chibicc, parses them into a C AST, and lowers the AST into EzPacker SSA MIR.
 */
class CFrontendAdapter : public IFrontendAdapter
{
  public:
    CFrontendAdapter();
    ~CFrontendAdapter() override = default;

    /**
     * Translates a C source file at sourcePath into generic MIR in outMirCtx.
     */
    bool compileSourceToMir(DriverContext &ctx, std::string_view sourcePath, MirBuilderContext &outMirCtx) override;

    /**
     * Translates in-memory C source text into generic MIR in outMirCtx.
     */
    bool compileStringToMir(DriverContext &ctx, std::string_view sourceCode, MirBuilderContext &outMirCtx, std::string_view virtualFileName = "input.c");

    /**
     * Adds an include directory for C preprocessor header lookup.
     */
    void addIncludePath(std::string_view path);

    /**
     * Adds a preprocessor macro definition (-D).
     */
    void defineMacro(std::string_view name, std::string_view value = "1");

  private:
    CFrontend::ChibiccWrapper m_wrapper;
};

} // namespace EzCompiler

#endif // EZPACKER_C_FRONTEND_ADAPTER_H
