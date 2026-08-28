#ifndef EZTRIPLE_MIR_LEGALIZE_ACTION_TABLE_H
#define EZTRIPLE_MIR_LEGALIZE_ACTION_TABLE_H

#include "EzTripleCommon.h"
#include "Actions/LegalizeActionCommon.h"
#include "Instruction/MirInstructionSet.h"
#include "Type/MirTypeTable.h"

struct alignas(8) LegalizeQueryResult
{
    LegalizeAction m_action;
    uint8_t m_compactId;       // Compact Id used for fast LookUp Tables in O(1) time.
    uint8_t m_slot;            // Target operand slot (0, 1, or 2)
    uint8_t m_libcallOffset;   // Offset in target's static runtime-functions string table
    uint16_t m_customActionId; // Offset in the custom action table to executethe function.
};

/**
 * A no-context callback that returns the action for a specific opcode given its operand MIR types.
 *
 * It allows up to 3 operands. Important note, 0 here means DEFAULT VALUE (unset).
 */
using LegalizeActionQueryFunc = LegalizeQueryResult (*)(size_t op1Type, size_t op2Type, size_t op3Type);

/**
 * This is a custom action function callback. It uses the legalization context. Invoked by MirLegalizer.
 */
using LegalizeCustomActionFunc = LegalizationResult (*)(class LegalizeCtx *ctx);

/**
 * This is made a struct and not a class for direct queries without vTable access to maximize throughput and cache
 * locality.
 */
struct MirLegalizeActionTable
{
    LegalizeActionQueryFunc m_queryTable[static_cast<uint16_t>(MirInstructionOpCode::OPCODE_COUNT) + 1];
    LegalizeCustomActionFunc m_customActionTable[static_cast<uint8_t>(UINT8_MAX)];

    /**
     * This function just uses the query table. It's marked as inline to ensure the compiler inlines this out instead of
     * generating a call.
     */
    inline LegalizeQueryResult query(MirInstructionOpCode op, uint8_t t0, uint8_t t1 = 0, uint8_t t2 = 0) const
    {
        return m_queryTable[static_cast<uint16_t>(op)](t0, t1, t2);
    }

    /**
     * Executes a given action function. It's marked as inline to ensure the compiler inlines this out instead of
     * generating a call.
     */
    inline LegalizationResult executeCustomAction(class LegalizeCtx *ctx, uint8_t actionIndex) const
    {
        return m_customActionTable[actionIndex](ctx);
    }
};

#endif // EZTRIPLE_MIR_LEGALIZE_ACTION_TABLE_H