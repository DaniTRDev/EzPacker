#ifndef EZPACKER_EZMIR_H
#define EZPACKER_EZMIR_H

#include "EzMirCommon.h"

// ── Block container ─────────────────────────────────────────────────────────
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Block/MirBlockInstructionQuery.h"

// ── Emitters (block/instruction builders & global data) ─────────────────────
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"

// ── Class container ─────────────────────────────────────────────────────────
#include "Class/MirClass.h"
#include "Class/MirClassBuilder.h"

// ── Function container ──────────────────────────────────────────────────────
#include "Function/ArgumentLocationDesc.h"
#include "Function/CallingConvDesc.h"
#include "Function/CallLoweringState.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "Function/MirFunctionStackFrame.h"

// ── Global variable container ───────────────────────────────────────────────
#include "GlobalVar/MirGlobalVar.h"
#include "GlobalVar/MirGlobalVarBuilder.h"

// ── Instructions & opcode catalogue ─────────────────────────────────────────
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstructionDefs.h"
#include "Instruction/MirInstructionSet.h"

// ── Generic passes ──────────────────────────────────────────────────────────
#include "MirPasses/IMirAnalysisPass.h"
#include "MirPasses/MirPass.h"
#include "MirPasses/IMirTransformPass.h"
#include "MirPasses/MirPassManager.h"
#include "MirPasses/Passes/ClassOffsetResolverPass.h"
#include "MirPasses/Passes/CodeFlowAnalysisPass.h"
#include "MirPasses/Passes/LivenessAnalysisPass.h"
#include "MirPasses/Passes/RelativeReferenceLowererPass.h"

// ── Basic block ─────────────────────────────────────────────────────────────
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Block/MirBlockInstructionQuery.h"

// ── Operand variant (registers, immediates, memory, references) ─────────────
#include "Operand/MirOperand.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterReference.h"

#include "Printer/MirPrinter.h"

// ── Type system primitives ──────────────────────────────────────────────────
#include "Type/IMirTargetTypeLayout.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

#endif // EZPACKER_EZMIR_H
