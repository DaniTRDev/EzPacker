/**
 * @file AstLowering.h
 * @brief Compilation phase for lowering AST to MIR.
 *
 * The AstLoweringPhase is the final step in the frontend compilation pipeline.
 * It takes the semantically analyzed AST and converts it into the Mid-level
 * Intermediate Representation (MIR) using the `MirEmitter` from EzMir.
 */
#ifndef EZPACKER_ASTLOWERING_H
#define EZPACKER_ASTLOWERING_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * @class AstLoweringPhase
 * @brief Converts the AST into MIR.
 *
 * This phase initializes the MIR emitter and processes the AST to generate
 * the intermediate representation. The resulting MIR is stored in the
 * `FrontendCompilationUnit` and can be used for optimization and code generation.
 *
 * If lowering fails (e.g., unsupported constructs), this phase reports errors
 * and returns `false`.
 */
class AstLoweringPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * @brief Executes the AST lowering phase.
     *
     * Runs the MIR emitter on the unit's AST to produce MIR.
     *
     * @param unit Pointer to the compilation unit to lower.
     * @return `true` if lowering succeeded; `false` otherwise.
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * @brief Gets the phase name.
     *
     * @return "AstLoweringPhase"
     */
    const char *getName() override;
};

#endif // EZPACKER_ASTLOWERING_H
