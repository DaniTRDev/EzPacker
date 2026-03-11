/**
 * @file EzFrontendCompiler.h
 * @brief Umbrella header for the EzFrontendCompiler library.
 *
 * EzFrontendCompiler is the high-level driver that orchestrates the entire
 * front-end compilation process for the EzPacker toolchain. It is responsible
 * for managing source files, coordinating various compilation phases, and
 * bridging the gap between different frontend libraries.
 *
 * Its main responsibilities include:
 *
 *   1. **Source Management** – Accepting source code from files or in-memory
 *      strings and managing them as individual `FrontendCompilationUnit`s.
 *
 *   2. **Phase Execution** – Driving each compilation unit through a series of
 *      well-defined phases:
 *      - Tokenization & Parsing (using EzLexer)
 *      - Include Resolution
 *      - Semantic Analysis (using EzSemantics)
 *      - AST Lowering to prepare for MIR generation (for EzMir)
 *
 *   3. **Orchestration** – The `FrontendCompilerDriver` class acts as the central
 *      coordinator, initializing and running the phases in the correct order.
 *
 * Including this single header gives you access to the main driver, compilation
 * unit management, and all related components of the frontend compiler.
 */
#ifndef EZPACKER_EZFRONTENDCOMPILER_H
#define EZPACKER_EZFRONTENDCOMPILER_H

#include "EzFrontendCompilerCommon.h"

// ── Core Components ─────────────────────────────────────────────────────────
#include "FrontendCompilerDriver.h"
#include "FrontendCompilationUnit.h"
#include "FrontendCompilationUnitPhase.h"

// ── Compilation Phases ──────────────────────────────────────────────────────
#include "CompilationPhases/AstLowering.h"
#include "CompilationPhases/IncludePhase.h"
#include "CompilationPhases/Parsing.h"
#include "CompilationPhases/SemanticAnalysis.h"
#include "CompilationPhases/Tokenization.h"

// ── Visitors ────────────────────────────────────────────────────────────────
#include "IncludeVisitor/IncludeVisitor.h"


#endif // EZPACKER_EZFRONTENDCOMPILER_H
