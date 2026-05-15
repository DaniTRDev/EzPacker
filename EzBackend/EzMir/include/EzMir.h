/**
 * @file EzMir.h
 * @brief Umbrella header for the EzMir library (Mid-level Intermediate Representation).
 *
 * Including this header gives access to the full public EzMir API. The module
 * models a compiler IR that sits between higher-level semantic analysis and
 * later lowering/code-generation stages.
 *
 * Main building blocks:
 *   - `MirType`: self-contained MIR type descriptors identified by MIR IDs.
 *   - `MirOperand`: variant payloads used as instruction arguments.
 *   - `MirInstruction`: opcode + operand slice, with metadata derived from the
 *     instruction catalogue.
 *   - `MirBlock`: ordered instruction list representing one basic block.
 *   - `MirFunction`: entry block plus block/parameter lists for one callable
 *     unit.
 *   - `MirEmitterContext`: owning arena context that allocates and links MIR
 *     objects.
 *   - `MirEmitter`: convenience API for emitting instructions into the bound
 *     block.
 *   - `MirGlobalDataEmitter`: convenience API for emitting context-owned
 *     global data blobs and literals.
 */
#ifndef EZPACKER_EZMIR_H
#define EZPACKER_EZMIR_H

#include "EzMirCommon.h"

// ── Emitters (block/instruction builders & global data) ─────────────────────
#include "Emitter/MirEmitter.h"
#include "Emitter/MirEmitterContext.h"

// ── Function container ──────────────────────────────────────────────────────
#include "Function/MirFunction.h"

// ── Instructions & opcode catalogue ─────────────────────────────────────────
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionDefs.h"
#include "Instruction/MirInstructionSet.h"

// ── Generic passes ──────────────────────────────────────────────────────────
#include "MirPass/IMirAnalysisPass.h"
#include "MirPass/IMirPass.h"
#include "MirPass/IMirTransformPass.h"
#include "MirPass/MirPassManager.h"
#include "MirPass/Passes/CodeFlowAnalysis.h"
#include "MirPass/Passes/LivenessAnalysis.h"

// ── Basic block ─────────────────────────────────────────────────────────────
#include "MirBlock.h"

// ── Operand variant (registers, immediates, memory, references) ─────────────
#include "Operand/MirOperand.h"
#include "Operand/MirOperands.h"

// ── Type system primitives ──────────────────────────────────────────────────
#include "Type/MirType.h"
#include "Type/MirTypes.h"

#endif // EZPACKER_EZMIR_H
