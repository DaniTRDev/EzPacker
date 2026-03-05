/**
 * @file EzMir.h
 * @brief Umbrella header for the EzMir library (Mid-level Intermediate Representation).
 *
 * EzMir defines the compiler's intermediate representation that sits between
 * the high-level AST (produced by EzLexer) and any final code-generation or
 * optimization backend.  Its main building blocks are:
 *
 *   - **MirBlock** – A basic block: a straight-line sequence of instructions
 *     with a single entry point and a single terminating control-flow edge.
 *
 *   - **MirInstruction / MirInstructionSet** – A typed, opcode-driven
 *     instruction (MOV, ADD, CMP, JMP, …) together with the full catalogue
 *     of supported opcodes and their metadata (operand count, flags, etc.).
 *
 *   - **MirOperand** – A variant value that an instruction operates on.
 *     Operands can be virtual registers, integer/float immediates, memory
 *     references (base + index*scale + displacement), or block references.
 *
 *   - **MirFunction** – A function-level container that owns an ordered list
 *     of basic blocks, an entry point, a return type, and parameter operands.
 *
 *   - **MirType** – Representation of primitive types and their sizes used
 *     during MIR emission and later lowering stages.
 *
 *   - **Emitters** – Builder APIs that make constructing well-formed MIR
 *     convenient:
 *       • MirEmitterContext – manages block/instruction/operand pools and
 *         tracks the currently bound block and function.
 *       • MirEmitter – high-level helpers to emit typed instructions
 *         (emitMOV, emitADD, emitJMP, …) into the current block.
 *       • MirGlobalDataEmitter – emits initialized and uninitialized global
 *         data entries (the .data / .rdata / .bss equivalent).
 *
 * Including this single header gives you access to every public type in EzMir.
 */
#ifndef EZPACKER_EZMIR_H
#define EZPACKER_EZMIR_H

#include "EzMirCommon.h"

// ── Emitters (block/instruction builders & global data) ─────────────────────
#include "Emitter/MirEmitter.h"
#include "Emitter/MirEmitterContext.h"
#include "Emitter/MirGlobalDataEmitter.h"

// ── Function container ──────────────────────────────────────────────────────
#include "Function/MirFunction.h"

// ── Instructions & opcode catalogue ─────────────────────────────────────────
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionDefs.h"
#include "Instruction/MirInstructionSet.h"

// ── Basic block ─────────────────────────────────────────────────────────────
#include "MirBlock.h"

// ── Operand variant (registers, immediates, memory, references) ─────────────
#include "Operand/MirOperand.h"

// ── Type system primitives ──────────────────────────────────────────────────
#include "Type/MirType.h"

#endif // EZPACKER_EZMIR_H
