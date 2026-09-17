#ifndef EZTRIPLE_LEGALITY_QUERY_H
#define EZTRIPLE_LEGALITY_QUERY_H

#include "EzTripleCommon.h"
#include "Instruction/MirInstructionMetadata.h"
#include "Instruction/MirInstructionSet.h"
#include "Type/MirType.h"

#include <array>
#include <cstdint>

/**
 * Predefined legalization action kinds for target machine legalization.
 */
enum class LegalizeActionKind : uint8_t
{
    Legal = 0,     // Instruction is natively supported for given types/operands.
    WidenScalar,   // Promote scalar operand(s) to a larger legal scalar type (e.g. i8 -> i32).
    NarrowScalar,  // Split scalar operand(s) into smaller legal types (e.g. i128 -> 2x i64).
    Bitcast,       // Reinterpret bit pattern into a legal type of equal bit-width (e.g. f32 -> i32).
    Libcall,       // Lower instruction into a standard ABI runtime library call (e.g. __divdi3).
    Lower,         // Decompose complex high-level instruction into standard target MIR primitives
                   // (Used for CALL -> PUSH_ARG/POP_RET, RET -> PUSH_RET/RET, ALLOC -> frame layout).
    Custom,        // Delegate legalization to target-defined rewrite rule or C++ member function.
    Unsupported    // Explicitly rejected combination. Emits a compiler diagnostic error.
};

/**
 * Query descriptor capturing all relevant properties of a MIR instruction to determine its legality.
 */
struct LegalityQuery
{
    MirInstructionOpCode m_opcode{ MirInstructionOpCode::INVALID };
    uint32_t m_flags{ 0 };                     // MirInstructionFlags (IsCall, IsReturn, IsSigned, etc.)
    size_t m_operandCount{ 0 };
    std::array<MirType *, 6> m_types{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };     // Concrete MirType pointers for operands
    std::array<uint8_t, 6> m_compactIds{ 0, 0, 0, 0, 0, 0 };  // Compact type IDs for fast indexing
    std::array<ExpectedOperandType, 6> m_operandKinds{
        ExpectedOperandType::None, ExpectedOperandType::None, ExpectedOperandType::None,
        ExpectedOperandType::None, ExpectedOperandType::None, ExpectedOperandType::None
    }; // Register, Immediate, Memory, etc.
    int64_t m_immValue{ 0 };              // Immediate constant value if second operand is imm
    bool m_hasImm{ false };
};

/**
 * Decision result returned by LegalizerInfo for a given LegalityQuery.
 */
struct alignas(8) LegalityResponse
{
    LegalizeActionKind m_action{ LegalizeActionKind::Unsupported };
    uint8_t m_slot{ 0 };                 // Primary operand slot targeted by action (0, 1, 2)
    uint8_t m_targetCompactId{ 0 };      // Destination type compact ID for Widen/Narrow/Bitcast
    uint16_t m_handlerOrStringId{ 0 };   // Libcall string index or Custom/Lower callback index

    constexpr bool isLegal() const noexcept { return m_action == LegalizeActionKind::Legal; }
    constexpr bool isUnsupported() const noexcept { return m_action == LegalizeActionKind::Unsupported; }
};

#endif // EZTRIPLE_LEGALITY_QUERY_H
