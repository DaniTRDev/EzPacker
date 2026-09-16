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
    static constexpr size_t OPCODE_COUNT = static_cast<size_t>(MirInstructionOpCode::OPCODE_COUNT) + 1;
    static constexpr size_t MAX_COMPACT_TYPES = 32;

    using LegalizeHandler = LegalizationResult (*)(LegalizeCtx &ctx);
    using RuleMatcher = std::function<LegalityResponse(const LegalityQuery &)>;

  public:
    LegalizerInfo()
    {
        for (size_t op = 0; op < OPCODE_COUNT; ++op)
        {
            m_wildcardActions[op] = LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
            for (size_t t = 0; t < MAX_COMPACT_TYPES; ++t)
            {
                m_primaryMatrix[op][t] = LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
            }
        }
    }

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
     * Fast Tier 1 direct query for homogeneous primary type.
     */
    inline LegalityResponse queryFast(MirInstructionOpCode op, uint8_t typeId) const
    {
        size_t opIdx = static_cast<size_t>(op);
        if (opIdx < OPCODE_COUNT && typeId < MAX_COMPACT_TYPES)
        {
            return m_primaryMatrix[opIdx][typeId];
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

    ActionDefinitionBuilder getActionDefinitions(MirInstructionOpCode op);
    ActionDefinitionBuilder getActionDefinitions(std::initializer_list<MirInstructionOpCode> ops);

    void setPrimaryMatrix(MirInstructionOpCode op, uint8_t typeId, LegalityResponse resp)
    {
        size_t opIdx = static_cast<size_t>(op);
        if (opIdx < OPCODE_COUNT && typeId < MAX_COMPACT_TYPES)
        {
            m_primaryMatrix[opIdx][typeId] = resp;
        }
    }

    void setWildcardAction(MirInstructionOpCode op, LegalityResponse resp)
    {
        size_t opIdx = static_cast<size_t>(op);
        if (opIdx < OPCODE_COUNT)
        {
            m_wildcardActions[opIdx] = resp;
        }
    }

    void addRuleMatcher(MirInstructionOpCode op, RuleMatcher matcher)
    {
        m_ruleMatchers[op].push_back(std::move(matcher));
    }

    uint16_t registerHandler(LegalizeHandler handler)
    {
        m_handlers.push_back(handler);
        return static_cast<uint16_t>(m_handlers.size() - 1);
    }

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

    std::string_view getLibcallSymbol(uint16_t id) const
    {
        if (id < m_libcallSymbols.size())
        {
            return m_libcallSymbols[id];
        }
        return {};
    }

  protected:
    LegalityResponse m_primaryMatrix[OPCODE_COUNT][MAX_COMPACT_TYPES];
    LegalityResponse m_wildcardActions[OPCODE_COUNT];
    std::unordered_map<MirInstructionOpCode, std::vector<RuleMatcher>> m_ruleMatchers;
    std::vector<LegalizeHandler> m_handlers;
    std::vector<std::string> m_libcallSymbols;
};

/**
 * Fluent builder for declaring target instruction legality rules.
 */
class ActionDefinitionBuilder
{
  public:
    ActionDefinitionBuilder(LegalizerInfo *info, std::vector<MirInstructionOpCode> opcodes) :
        m_info(info), m_opcodes(std::move(opcodes))
    {
    }

    ActionDefinitionBuilder &legalFor(std::initializer_list<MirType *> types)
    {
        for (auto op : m_opcodes)
        {
            for (auto *t : types)
            {
                if (t && t->getCompactId() < LegalizerInfo::MAX_COMPACT_TYPES)
                {
                    m_info->setPrimaryMatrix(op, t->getCompactId(),
                        LegalityResponse{ .m_action = LegalizeActionKind::Legal,
                                          .m_slot = 0,
                                          .m_targetCompactId = t->getCompactId() });
                }
            }
        }
        return *this;
    }

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

            m_info->addRuleMatcher(op, [sigs](const LegalityQuery &q) -> LegalityResponse {
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

    ActionDefinitionBuilder &widenScalarTo(size_t slot, std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        uint8_t toId = toType ? toType->getCompactId() : 0;
        for (auto op : m_opcodes)
        {
            if (slot == 0)
            {
                for (auto *t : fromTypes)
                {
                    if (t && t->getCompactId() < LegalizerInfo::MAX_COMPACT_TYPES)
                    {
                        m_info->setPrimaryMatrix(op, t->getCompactId(),
                            LegalityResponse{ .m_action = LegalizeActionKind::WidenScalar,
                                              .m_slot = 0,
                                              .m_targetCompactId = toId });
                    }
                }
            }
            else
            {
                std::vector<uint8_t> fromIds;
                for (auto *t : fromTypes)
                {
                    if (t) fromIds.push_back(t->getCompactId());
                }
                m_info->addRuleMatcher(op, [slot, fromIds, toId](const LegalityQuery &q) -> LegalityResponse {
                    if (q.m_operandCount > slot)
                    {
                        for (uint8_t fid : fromIds)
                        {
                            if (q.m_compactIds[slot] == fid)
                            {
                                return LegalityResponse{ .m_action = LegalizeActionKind::WidenScalar,
                                                          .m_slot = static_cast<uint8_t>(slot),
                                                          .m_targetCompactId = toId };
                            }
                        }
                    }
                    return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
                });
            }
        }
        return *this;
    }

    ActionDefinitionBuilder &narrowScalarTo(size_t slot, std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        uint8_t toId = toType ? toType->getCompactId() : 0;
        for (auto op : m_opcodes)
        {
            if (slot == 0)
            {
                for (auto *t : fromTypes)
                {
                    if (t && t->getCompactId() < LegalizerInfo::MAX_COMPACT_TYPES)
                    {
                        m_info->setPrimaryMatrix(op, t->getCompactId(),
                            LegalityResponse{ .m_action = LegalizeActionKind::NarrowScalar,
                                              .m_slot = 0,
                                              .m_targetCompactId = toId });
                    }
                }
            }
            else
            {
                std::vector<uint8_t> fromIds;
                for (auto *t : fromTypes)
                {
                    if (t) fromIds.push_back(t->getCompactId());
                }
                m_info->addRuleMatcher(op, [slot, fromIds, toId](const LegalityQuery &q) -> LegalityResponse {
                    if (q.m_operandCount > slot)
                    {
                        for (uint8_t fid : fromIds)
                        {
                            if (q.m_compactIds[slot] == fid)
                            {
                                return LegalityResponse{ .m_action = LegalizeActionKind::NarrowScalar,
                                                          .m_slot = static_cast<uint8_t>(slot),
                                                          .m_targetCompactId = toId };
                            }
                        }
                    }
                    return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
                });
            }
        }
        return *this;
    }

    ActionDefinitionBuilder &libcallFor(MirType *type, std::string_view symbol)
    {
        uint16_t strId = m_info->registerLibcallSymbol(symbol);
        for (auto op : m_opcodes)
        {
            if (type && type->getCompactId() < LegalizerInfo::MAX_COMPACT_TYPES)
            {
                m_info->setPrimaryMatrix(op, type->getCompactId(),
                    LegalityResponse{ .m_action = LegalizeActionKind::Libcall,
                                      .m_slot = 0,
                                      .m_targetCompactId = type->getCompactId(),
                                      .m_handlerOrStringId = strId });
            }
        }
        return *this;
    }

    ActionDefinitionBuilder &lowerWith(LegalizerInfo::LegalizeHandler handler)
    {
        uint16_t hId = m_info->registerHandler(handler);
        for (auto op : m_opcodes)
        {
            m_info->setWildcardAction(op,
                LegalityResponse{ .m_action = LegalizeActionKind::Lower,
                                  .m_slot = 0,
                                  .m_handlerOrStringId = hId });
        }
        return *this;
    }

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

    ActionDefinitionBuilder &legalForTypesWithSource(std::initializer_list<MirType *> sourceTypes)
    {
        std::vector<uint8_t> sIds;
        for (auto *t : sourceTypes)
        {
            if (t) sIds.push_back(t->getCompactId());
        }
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(op, [sIds](const LegalityQuery &q) -> LegalityResponse {
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

    ActionDefinitionBuilder &widenScalarSourceTo(std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        uint8_t toId = toType ? toType->getCompactId() : 0;
        std::vector<uint8_t> fromIds;
        for (auto *t : fromTypes)
        {
            if (t) fromIds.push_back(t->getCompactId());
        }
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(op, [fromIds, toId](const LegalityQuery &q) -> LegalityResponse {
                if (q.m_operandCount > 1)
                {
                    for (uint8_t fid : fromIds)
                    {
                        if (q.m_compactIds[1] == fid)
                        {
                            return LegalityResponse{ .m_action = LegalizeActionKind::WidenScalar,
                                                      .m_slot = 0,
                                                      .m_targetCompactId = toId };
                        }
                    }
                }
                return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
            });
        }
        return *this;
    }

    ActionDefinitionBuilder &narrowScalarSourceTo(std::initializer_list<MirType *> fromTypes, MirType *toType)
    {
        uint8_t toId = toType ? toType->getCompactId() : 0;
        std::vector<uint8_t> fromIds;
        for (auto *t : fromTypes)
        {
            if (t) fromIds.push_back(t->getCompactId());
        }
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(op, [fromIds, toId](const LegalityQuery &q) -> LegalityResponse {
                if (q.m_operandCount > 1)
                {
                    for (uint8_t fid : fromIds)
                    {
                        if (q.m_compactIds[1] == fid)
                        {
                            return LegalityResponse{ .m_action = LegalizeActionKind::NarrowScalar,
                                                      .m_slot = 0,
                                                      .m_targetCompactId = toId };
                        }
                    }
                }
                return LegalityResponse{ .m_action = LegalizeActionKind::Unsupported };
            });
        }
        return *this;
    }

    ActionDefinitionBuilder &legalIfSameType()
    {
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(op, [](const LegalityQuery &q) -> LegalityResponse {
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

    ActionDefinitionBuilder &bitcastBetween(MirType *typeA, MirType *typeB)
    {
        uint8_t idA = typeA ? typeA->getCompactId() : 0;
        uint8_t idB = typeB ? typeB->getCompactId() : 0;
        for (auto op : m_opcodes)
        {
            m_info->addRuleMatcher(op, [idA, idB](const LegalityQuery &q) -> LegalityResponse {
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
    LegalizerInfo *m_info;
    std::vector<MirInstructionOpCode> m_opcodes;
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
