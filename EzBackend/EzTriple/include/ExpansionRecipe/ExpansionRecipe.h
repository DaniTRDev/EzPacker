#ifndef EZPACKER_EXPANSIONRECIPE_H
#define EZPACKER_EXPANSIONRECIPE_H

#include "EzTripleCommon.h"

enum class ExpansionOperandKind : uint8_t
{
    DestLow = 0, // Destination operand (first operand) low part.
    DestHigh,    // Destination operand (first operand) high part.

    Src0Low,  // Source operand 0 (second operand) low part.
    Src0High, // Source operand 0 (second operand) high part.

    Src1Low,  // Source operand 1 (third operand) low part.
    Src1High, // Source operand 1 (third operand) high part.

    // These temporal registers are used to contain temporal values to help expanding instructions.
    TemporalLow0,  // Temporal 0 register's low part.
    TemporalHigh0, // Temporal 0 register's high part.

    TemporalLow1,  // Temporal 1 register's low part.
    TemporalHigh1, // Temporal 1 register's high part.

    TemporalLow2,  // Temporal 2 register's low part.
    TemporalHigh2, // Temporal 2 register's high part.

    IntImm,   // An integer immediate.
    FloatImm, // A floating-point immediate.

    Memory,          // A memory operand referencing base and m_displ slices.
    MemoryHalfOffset /*
                      * A memory operand with an offset following this formula: addr + (offset * sizeof(HalfChunk)).
                      * This is needed to be able to expand any given size, ex: 128bit (16 bytes) ->
                      * sizeof(HalfChunk) = 8
                      *     MEM_HALF(base, 0) -> Points to addr + (0 * 8)
                      *     MEM_HALF(base, 1) -> Points to addr + (1 * 8)
                      */
};

/**
 * Encapsulates static parameters for a memory layout recipe.
 */
struct RecipeMemoryPayload
{
    ExpansionOperandKind m_baseKind;

    int32_t m_scaleHalfSizeFactor; /*
                                    * Multiplier for the current recursive slice size:
                                    * addr + (m_scaleHalfSizeFactor * sizeof(HalfChunk))
                                    */

    int64_t m_displ; // Static m_displ.
};

/**
 * A uniform variant-like token capable of being declared cleanly inside static tables.
 */
struct RecipeOperand
{
    ExpansionOperandKind kind;

    // Inline data representations for values
    FlexInt intVal{ 0 };
    FlexFloat floatVal{ 0.0 };
    RecipeMemoryPayload memVal = { ExpansionOperandKind::DestLow, 0, 0 }; // Fixed payload initialization

    // Standard initializers for implicit conversion inside recipes
    RecipeOperand(ExpansionOperandKind k) : kind(k) {}
    RecipeOperand(const FlexInt &val) : kind(ExpansionOperandKind::IntImm), intVal(val) {}
    RecipeOperand(const FlexFloat &val) : kind(ExpansionOperandKind::FloatImm), floatVal(val) {}

    // Explicit constructor used by the memory helper macros
    RecipeOperand(ExpansionOperandKind k, RecipeMemoryPayload payload) : kind(k), memVal(payload) {}
};

// Define common tokens that help creating recipes cleanly.
namespace Expansion
{
inline constexpr ExpansionOperandKind D_LO = ExpansionOperandKind::DestLow;
inline constexpr ExpansionOperandKind D_HI = ExpansionOperandKind::DestHigh;

inline constexpr ExpansionOperandKind S0_LO = ExpansionOperandKind::Src0Low;
inline constexpr ExpansionOperandKind S0_HI = ExpansionOperandKind::Src0High;

inline constexpr ExpansionOperandKind S1_LO = ExpansionOperandKind::Src1Low;
inline constexpr ExpansionOperandKind S1_HI = ExpansionOperandKind::Src1High;

inline constexpr ExpansionOperandKind T0_LO = ExpansionOperandKind::TemporalLow0;
inline constexpr ExpansionOperandKind T0_HI = ExpansionOperandKind::TemporalHigh0;

inline constexpr ExpansionOperandKind T1_LO = ExpansionOperandKind::TemporalLow1;
inline constexpr ExpansionOperandKind T1_HI = ExpansionOperandKind::TemporalHigh1;

inline constexpr ExpansionOperandKind T2_LO = ExpansionOperandKind::TemporalLow2;
inline constexpr ExpansionOperandKind T2_HI = ExpansionOperandKind::TemporalHigh2;
} // namespace Expansion

/**
 * Structure to contain a part of the recipe to expand an instruction.
 */
struct ExpansionInstruction
{
    MirInstructionOpCode m_instr;
    std::string m_rtLibraryCall; /*
                                  * Used to tell the legalizer that this instruction must be expanded using a runtime
                                  * library call.
                                  */
    std::vector<RecipeOperand> m_operands;
};

/**
 * A recipe that tells how an instruction must be expanded.
 */
struct ExpansionRecipe
{
    MirInstructionOpCode m_target;
    std::vector<ExpansionInstruction> m_expandSequence;
};

#define INT_IMM(value) RecipeOperand(FlexInt(value))
#define FLOAT_IMM(value) RecipeOperand(FlexFloat(value))

#define MEM(base, disp)                                                                                                \
    RecipeOperand(ExpansionOperandKind::Memory, RecipeMemoryPayload{ Expansion::base, 0, static_cast<int64_t>(disp) })

#define MEM_HALF(base, halfFactor)                                                                                     \
    RecipeOperand(ExpansionOperandKind::MemoryHalfOffset,                                                              \
                  RecipeMemoryPayload{ Expansion::base, static_cast<int32_t>(halfFactor), 0 })

#define DEFINE_EXPANSION_RECIPES(TargetName) inline const ExpansionRecipe g_##TargetName_MirExpansionRecipes[] = {
#define GET_EXPANSION_RECIPES(TargetName) g_##TargetName_MirExpansionRecipes
#define GET_EXPANSION_RECIPES_SIZE(TargetName) sizeof(g_##TargetName_MirExpansionRecipes)

#define END_EXPANSION_RECIPES                                                                                          \
    }                                                                                                                  \
    ;

#define RECIPE_FOR(targetOp)                                                                                           \
    {                                                                                                                  \
        MirInstructionOpCode::targetOp,                                                                                \
        {

#define END_RECIPE                                                                                                     \
    }                                                                                                                  \
    }                                                                                                                  \
    ,

#define EMIT_INST(op, ...) { MirInstructionOpCode::op, "", { __VA_ARGS__ } },
#define EMIT_RT_CALL(name, ...) { MirInstructionOpCode::CALL, name, { __VA_ARGS__ } },

#endif // EZPACKER_EXPANSIONRECIPE_H