/**
 * @file SemanticAnalysis.h
 * @brief Compilation phase for semantic analysis.
 *
 * The SemanticAnalysisPhase performs type checking and other semantic validations
 * on the AST. It ensures that the code adheres to the language's type system
 * and semantic rules before MIR generation.
 */
#ifndef EZPACKER_SEMANTICANALYSIS_H
#define EZPACKER_SEMANTICANALYSIS_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

#include "Semantic/SymbolAndTypeResolver.h"
#include "Semantic/SymbolDefinition.h"
#include "Semantic/TypeCheck.h"

/**
 * @class SemanticAnalysisPhase
 * @brief Validates the semantics of the AST.
 *
 * This phase runs a series of semantic checks on the AST, including symbol
 * resolution, type checking, and other validations. It uses the `EzSemantics`
 * library to perform these checks.
 *
 * If semantic errors are found (e.g., type mismatches, undefined symbols),
 * this phase reports them and returns `false`.
 */
class SemanticAnalysisPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * @brief Executes the semantic analysis phase.
     *
     * Runs the semantic analyzer on the unit's AST to validate its correctness.
     *
     * @param unit Pointer to the compilation unit to analyze.
     * @return `true` if semantic analysis succeeded; `false` otherwise.
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * @brief Gets the phase name.
     *
     * @return "SemanticAnalysisPhase"
     */
    const char *getName() override;
};

#endif // EZPACKER_SEMANTICANALYSIS_H
