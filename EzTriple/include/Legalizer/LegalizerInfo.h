#ifndef EZTRIPLE_LEGALIZER_INFO_H
#define EZTRIPLE_LEGALIZER_INFO_H

#include "EzTripleCommon.h"
#include "Actions/LegalizeActionCommon.h"
#include "Instruction/MirInstructionSet.h"
#include "Legalizer/LegalityQuery.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

#include <array>
#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class LegalizeCtx;
class ActionDefinitionBuilder;

/**
 * Modern, fluent, 3-tier target machine legalizer information interface.
 * Combines an O(1) dense 2D primary matrix for homogeneous scalar operations with
 * fast signature matchers for heterogeneous multi-slot operations and target lowering dispatchers.
 */
class LegalizerInfo
{
  public:
    static constexpr size_t OPCODE_COUNT = static_cast<size_t>(MirInstructionOpCode::OPCODE_COUNT) +
            1;                                      ///< Number of opcode rows in the primary matrix.
    static constexpr size_t MAX_COMPACT_TYPES = 32; ///< Number of compact type columns in the primary matrix.

    using LegalizeHandler =
            LegalizationResult (*)(LegalizeCtx &ctx); ///< Callable that performs a Custom/Lower rewrite.
    using RuleMatcher = std::function<LegalityResponse(const LegalityQuery &)>; ///< Predicate mapping a query to a
                                                                                ///< legality response.

  public:
    /**
     * Builds an empty legality table. The primary matrix and wildcard tables default to
     * Unsupported through LegalityResponse's default member initializers, so unregistered
     * combinations are rejected without an explicit per-element initialization pass. Generated
     * targets override query() and read their static tables directly, so they do not pay for a
     * base-class table copy either.
     */
    LegalizerInfo() = default;

    virtual ~LegalizerInfo() = default;

    /**
     * Queries the legalizer across all 3 tiers for a given instruction query.
     */
    virtual LegalityResponse query(const LegalityQuery &q) const
    {
        size_t opIdx = static_cast<size_t>(q.m_opcode);
        if (opIdx >= OPCODE_COUNT)
        {
            return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
        }

        // Tier 3 / Wildcard check: unconditional actions (e.g. CALL, RET lowering)
        if (m_wildcardActions[opIdx].m_action != LegalizeActionKind::Unsupported)
        {
            return m_wildcardActions[opIdx];
        }

        // Tier 2: Heterogeneous Rule Matchers
        auto it = m_ruleMatchers.find(q.m_opcode);
        if (it != m_ruleMatchers.end())
        {
            for (const auto &matcher : it->second)
            {
                LegalityResponse resp = matcher(q);
                if (resp.m_action != LegalizeActionKind::Unsupported)
                {
                    return resp;
                }
            }
        }

        // Tier 1: Dense 2D Primary Matrix (O(1))
        if (q.m_operandCount > 0 && q.m_compactIds[0] < MAX_COMPACT_TYPES)
        {
            LegalityResponse fastResp = m_primaryMatrix[opIdx][q.m_compactIds[0]];
            if (fastResp.m_action != LegalizeActionKind::Unsupported)
            {
                return fastResp;
            }
        }

        return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
    }

    /**
     * Executes custom or lower action callback registered with given handler ID.
     */
    virtual LegalizationResult executeCustom(LegalizeCtx &ctx, uint16_t handlerId)
    {
        if (handlerId < m_handlers.size() && m_handlers[handlerId])
        {
            return m_handlers[handlerId](ctx);
        }
        return LegalizationResult::Failed;
    }

    // --- Fluent API Helpers ---

    /// Starts a fluent rule definition for a single opcode.
    ActionDefinitionBuilder getActionDefinitions(MirInstructionOpCode op);

    /// Starts a fluent rule definition applying the same rules to several opcodes at once.
    ActionDefinitionBuilder getActionDefinitions(std::initializer_list<MirInstructionOpCode> ops);

    /// Stores a Tier 1 dense-matrix response for the given opcode and primary type.
    void setPrimaryMatrix(MirInstructionOpCode op, uint8_t typeId, LegalityResponse resp)
    {
        size_t opIdx = static_cast<size_t>(op);
        if (opIdx < OPCODE_COUNT && typeId < MAX_COMPACT_TYPES)
        {
            m_primaryMatrix[opIdx][typeId] = resp;
        }
    }

    /// Stores a Tier 3 wildcard response that applies to every operand shape of an opcode.
    void setWildcardAction(MirInstructionOpCode op, LegalityResponse resp)
    {
        size_t opIdx = static_cast<size_t>(op);
        if (opIdx < OPCODE_COUNT)
        {
            m_wildcardActions[opIdx] = resp;
        }
    }

    /// Appends a Tier 2 signature matcher consulted in registration order.
    void addRuleMatcher(MirInstructionOpCode op, RuleMatcher matcher)
    {
        m_ruleMatchers[op].push_back(std::move(matcher));
    }

    /// Registers a Custom/Lower callback and returns the id stored in LegalityResponse.
    uint16_t registerHandler(LegalizeHandler handler)
    {
        m_handlers.push_back(handler);
        return static_cast<uint16_t>(m_handlers.size() - 1);
    }

    /// Interns a libcall symbol name, returning an existing id when already present.
    uint16_t registerLibcallSymbol(std::string_view sym)
    {
        for (size_t i = 0; i < m_libcallSymbols.size(); ++i)
        {
            if (m_libcallSymbols[i] == sym)
                return static_cast<uint16_t>(i);
        }
        m_libcallSymbols.emplace_back(sym);
        return static_cast<uint16_t>(m_libcallSymbols.size() - 1);
    }

    /// Resolves a libcall id previously returned by registerLibcallSymbol.
    std::string_view getLibcallSymbol(uint16_t id) const
    {
        if (id < m_libcallSymbols.size())
        {
            return m_libcallSymbols[id];
        }
        return {};
    }

  protected:
    LegalityResponse m_primaryMatrix[OPCODE_COUNT][MAX_COMPACT_TYPES]; ///< Tier 1 opcode x type legality table.
    LegalityResponse m_wildcardActions[OPCODE_COUNT];                  ///< Tier 3 unconditional per-opcode actions.
    std::unordered_map<MirInstructionOpCode, std::vector<RuleMatcher>>
            m_ruleMatchers;                    ///< Tier 2 per-opcode matcher chains.
    std::vector<LegalizeHandler> m_handlers;   ///< Registered Custom/Lower callbacks.
    std::vector<std::string> m_libcallSymbols; ///< Interned libcall symbol names.
};

/**
 * Fluent builder for declaring target instruction legality rules.
 */
class ActionDefinitionBuilder
{
  public:
    /**
     * Creates a builder that applies its rules to every opcode in opcodes on the given LegalizerInfo.
     */
    ActionDefinitionBuilder(LegalizerInfo *info, std::vector<MirInstructionOpCode> opcodes) :
        m_info(info), m_opcodes(std::move(opcodes))
    {
    }

    /**
     * Marks operations whose primary operand has one of the listed types as Legal (Tier 1).
     */
    ActionDefinitionBuilder &legalFor(std::initializer_list<MirType *> types)
    {
        for (auto op : m_opcodes)
        {
            for (auto *t : types)
            {
                if (t && t->getCompactId() < LegalizerInfo::MAX_COMPACT_TYPES)
                {
                    m_info->setPrimaryMatrix(op,
                                             t->getCompactId(),
                                             LegalityResponse{ .m_action = LegalizeActionKind::Legal,
                                                               .m_slot = 0,
                                                               .m_targetCompactId = t->getCompactId() });
                }
            }
        }
        return *this;
    }

    /**
     * Marks operations Legal only when the full multi-operand type signature matches one of
     * the supplied tuples, installed as a Tier 2 matcher.
     */
    ActionDefinitionBuilder &legalFor(std::initializer_list<std::vector<MirType *>> multiSlotTypes)
    {
        for (auto op : m_opcodes)
        {
            std::vector<std::vector<uint8_t>> sigs;
            for (const auto &tuple : multiSlotTypes)
            {
                std::vector<uint8_t> ids;
                for (auto *t : tuple)
                {
                    ids.push_back(t ? t->getCompactId() : 0);
                }
                sigs.push_back(std::move(ids));
            }

            m_info->addRuleMatcher(
                    op,
                    [sigs](const LegalityQuery &q) -> LegalityResponse
                    {
                        for (const auto &sig : sigs)
                        {
                            if (q.m_operandCount >= sig.size())
                            {
                                bool match = true;
                                for (size_t i = 0; i < sig.size(); ++i)
                                {
                                    if (q.m_compactIds[i] != sig[i])
                                    {
                                        match = false;
                                        break;
                                    }
                                }
                                if (match)
                                {
                                    return LegalityResponse{ .m_action = LegalizeActionKind::Legal,
                                                             .m_slot = 0,
                                                             .m_targetCompactId = sig.empty() ? uint8_t(0) : sig[0] };
                                }
                            }
                        }
                        return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
                    });
        }
        return *this;
    }

    /**
     * Requests that values of fromTypes be promoted to toType before the operation runs.
     * Slot 0 uses the dense primary matrix; other slots install a Tier 2 matcher on that operand.
     */
    ActionDefinitionBuilder &widenScalarTo(size_t slot, std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        installScalarAction(slot, slot, fromTypes, toType, LegalizeActionKind::WidenScalar);
        return *this;
    }

    /**
     * Requests that values of fromTypes be split into toType before the operation runs.
     * Slot 0 uses the dense primary matrix; other slots install a Tier 2 matcher on that operand.
     */
    ActionDefinitionBuilder &narrowScalarTo(size_t slot, std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        installScalarAction(slot, slot, fromTypes, toType, LegalizeActionKind::NarrowScalar);
        return *this;
    }

    /**
     * Rewrites operations on the given type into a call to the named runtime library function.
     */
    ActionDefinitionBuilder &libcallFor(MirType *type, std::string_view symbol)
    {
        uint16_t strId = m_info->registerLibcallSymbol(symbol);
        for (auto op : m_opcodes)
        {
            if (type && type->getCompactId() < LegalizerInfo::MAX_COMPACT_TYPES)
            {
                m_info->setPrimaryMatrix(op,
                                         type->getCompactId(),
                                         LegalityResponse{ .m_action = LegalizeActionKind::Libcall,
                                                           .m_slot = 0,
                                                           .m_targetCompactId = type->getCompactId(),
                                                           .m_handlerOrStringId = strId });
            }
        }
        return *this;
    }

    /**
     * Registers a wildcard Lower handler that decomposes the operation into target MIR primitives.
     */
    ActionDefinitionBuilder &lowerWith(LegalizerInfo::LegalizeHandler handler)
    {
        uint16_t hId = m_info->registerHandler(handler);
        for (auto op : m_opcodes)
        {
            m_info->setWildcardAction(
                    op,
                    LegalityResponse{ .m_action = LegalizeActionKind::Lower, .m_slot = 0, .m_handlerOrStringId = hId });
        }
        return *this;
    }

    /**
     * Registers a wildcard Custom handler that performs a target-defined rewrite of the operation.
     */
    ActionDefinitionBuilder &customWith(LegalizerInfo::LegalizeHandler handler)
    {
        uint16_t hId = m_info->registerHandler(handler);
        for (auto op : m_opcodes)
        {
            m_info->setWildcardAction(op,
                                      LegalityResponse{ .m_action = LegalizeActionKind::Custom,
                                                        .m_slot = 0,
                                                        .m_handlerOrStringId = hId });
        }
        return *this;
    }

    /**
     * Marks operations Legal when their second operand (the source) has one of sourceTypes.
     */
    ActionDefinitionBuilder &legalForTypesWithSource(std::initializer_list<MirType *> sourceTypes)
    {
        std::vector<uint8_t> sIds;
        for (auto *t : sourceTypes)
        {
            if (t)
                sIds.push_back(t->getCompactId());
        }
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(op,
                                   [sIds](const LegalityQuery &q) -> LegalityResponse
                                   {
                                       if (q.m_operandCount > 1)
                                       {
                                           for (uint8_t sid : sIds)
                                           {
                                               if (q.m_compactIds[1] == sid)
                                               {
                                                   return LegalityResponse{ .m_action = LegalizeActionKind::Legal,
                                                                            .m_slot = 0,
                                                                            .m_targetCompactId = sid };
                                               }
                                           }
                                       }
                                       return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
                                   });
        }
        return *this;
    }

    /**
     * Widens the second (source) operand from any of fromTypes to toType, matching on that slot.
     */
    ActionDefinitionBuilder &widenScalarSourceTo(std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        installScalarAction(1, 0, fromTypes, toType, LegalizeActionKind::WidenScalar);
        return *this;
    }

    /**
     * Narrows the second (source) operand from any of fromTypes to toType, matching on that slot.
     */
    ActionDefinitionBuilder &narrowScalarSourceTo(std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        installScalarAction(1, 0, fromTypes, toType, LegalizeActionKind::NarrowScalar);
        return *this;
    }

    /**
     * Marks an operation Legal when its first two operands share the same non-zero compact type.
     */
    ActionDefinitionBuilder &legalIfSameType()
    {
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(
                    op,
                    [](const LegalityQuery &q) -> LegalityResponse
                    {
                        if (q.m_operandCount >= 2 && q.m_compactIds[0] != 0 && q.m_compactIds[0] == q.m_compactIds[1])
                        {
                            return LegalityResponse{ .m_action = LegalizeActionKind::Legal,
                                                     .m_slot = 0,
                                                     .m_targetCompactId = q.m_compactIds[0] };
                        }
                        return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
                    });
        }
        return *this;
    }

    /**
     * Allows an operation mixing typeA and typeB by bitcasting the mismatching operand to the other type.
     */
    ActionDefinitionBuilder &bitcastBetween(MirType *typeA, MirType *typeB)
    {
        uint8_t idA = typeA ? typeA->getCompactId() : 0;
        uint8_t idB = typeB ? typeB->getCompactId() : 0;
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(op,
                                   [idA, idB](const LegalityQuery &q) -> LegalityResponse
                                   {
                                       if (q.m_operandCount >= 2)
                                       {
                                           if (q.m_compactIds[0] == idA && q.m_compactIds[1] == idB)
                                           {
                                               return LegalityResponse{ .m_action = LegalizeActionKind::Bitcast,
                                                                        .m_slot = 1,
                                                                        .m_targetCompactId = idA };
                                           }
                                           if (q.m_compactIds[0] == idB && q.m_compactIds[1] == idA)
                                           {
                                               return LegalityResponse{ .m_action = LegalizeActionKind::Bitcast,
                                                                        .m_slot = 1,
                                                                        .m_targetCompactId = idB };
                                           }
                                       }
                                       return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
                                   });
        }
        return *this;
    }

    /**
     * Convenience rule clamping scalar support to the [minType, maxType] range: smaller types are
     * widened up to minType, the range itself is legal, and larger types are narrowed down to maxType.
     */
    ActionDefinitionBuilder &clampScalar(MirType *minType, MirType *maxType, MirTypeTable *tt)
    {
        if (tt)
        {
            widenScalarTo(0, { tt->i1(), tt->i8(), tt->i16() }, minType);
            legalFor({ minType, maxType });
            narrowScalarTo(0, { tt->i128(), tt->i256() }, maxType);
        }
        return *this;
    }

  private:
    /**
     * Installs one Widen/Narrow scalar action for every opcode in the builder.
     *
     * @param matchSlot  Operand slot consulted when matching (0 uses the dense primary matrix).
     * @param actionSlot Operand slot the action rewrites (may differ from matchSlot for source rules).
     * @param fromTypes  Types that trigger the action.
     * @param toType     Type the matched value is widened/narrowed to.
     * @param action     WidenScalar or NarrowScalar.
     */
    void installScalarAction(size_t matchSlot,
                             size_t actionSlot,
                             std::initializer_list<MirType *> fromTypes,
                             MirType *toType,
                             LegalizeActionKind action)
    {
        uint8_t toId = toType ? toType->getCompactId() : 0;
        for (auto op : m_opcodes)
        {
            if (matchSlot == 0 && actionSlot == 0)
            {
                for (auto *t : fromTypes)
                {
                    if (t && t->getCompactId() < LegalizerInfo::MAX_COMPACT_TYPES)
                    {
                        m_info->setPrimaryMatrix(op,
                                                 t->getCompactId(),
                                                 LegalityResponse{ .m_action = action,
                                                                   .m_slot = 0,
                                                                   .m_targetCompactId = toId });
                    }
                }
                continue;
            }

            std::vector<uint8_t> fromIds;
            for (auto *t : fromTypes)
            {
                if (t)
                    fromIds.push_back(t->getCompactId());
            }
            m_info->addRuleMatcher(
                    op,
                    [matchSlot, actionSlot, fromIds, toId, action](const LegalityQuery &q) -> LegalityResponse
                    {
                        if (q.m_operandCount > matchSlot)
                        {
                            for (uint8_t fid : fromIds)
                            {
                                if (q.m_compactIds[matchSlot] == fid)
                                {
                                    return LegalityResponse{ .m_action = action,
                                                             .m_slot = static_cast<uint8_t>(actionSlot),
                                                             .m_targetCompactId = toId };
                                }
                            }
                        }
                        return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
                    });
        }
    }

    LegalizerInfo *m_info;                       ///< LegalizerInfo receiving the rules being defined.
    std::vector<MirInstructionOpCode> m_opcodes; ///< Opcodes the builder applies each rule to.
};

inline ActionDefinitionBuilder LegalizerInfo::getActionDefinitions(MirInstructionOpCode op)
{
    return ActionDefinitionBuilder(this, { op });
}

inline ActionDefinitionBuilder LegalizerInfo::getActionDefinitions(std::initializer_list<MirInstructionOpCode> ops)
{
    return ActionDefinitionBuilder(this, std::vector<MirInstructionOpCode>(ops));
}

#endif // EZTRIPLE_LEGALIZER_INFO_H
