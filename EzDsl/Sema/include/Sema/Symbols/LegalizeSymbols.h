#ifndef EZDSLSEMA_LEGALIZE_SYMBOLS_H
#define EZDSLSEMA_LEGALIZE_SYMBOLS_H

#include "SymbolCommon.h"

namespace DSL::Ast::LegalizeActionDef
{
enum class LegalizeActionKind : uint8_t;
}; // namespace DSL::Ast::LegalizeActionDef

namespace DSL::Ast::LegalizeRuleDef
{
enum class RuleOperandKind : uint8_t;
}; // namespace DSL::Ast::LegalizeRuleDef

namespace DSL::Ast::IrInstDef
{
enum class IrInstCategory : uint8_t;
enum class IrInstFlag : uint32_t;
enum class IrInstTier : uint8_t;
enum class IrOperandDir : uint8_t;
enum class IrOperandType : uint16_t;
}; // namespace DSL::Ast::IrInstDef

namespace Symbols
{
/**
 * Semantic symbol representing a type constraint on a legalized operand slot (.lad).
 */
struct LegalizeActionConstraintSymbol
{
    SymbolId m_typeId{ InvalidSymbolId };   // Resolved type ID (e.g. i32)
    std::optional<uint32_t> m_operandIndex; // Operand slot index (0, 1, etc.)
};

/**
 * Semantic symbol for a legalization action clause (.lad).
 */
struct LegalizeActionClauseSymbol
{
    DSL::Ast::LegalizeActionDef::LegalizeActionKind m_kind;
    std::pmr::vector<LegalizeActionConstraintSymbol> m_types;
    std::optional<SymbolId> m_targetTypeId; // Widen/Narrow/Bitcast target
    std::optional<std::string_view> m_libcallSymbol;
    std::optional<std::pmr::vector<SymbolId>> m_customRules;
};

/**
 * Semantic symbol grouping all legalization actions for a generic IR opcode (.lad).
 */
struct LegalizeActionSymbol
{
    std::string_view m_genericOpcode;
    size_t m_maxOperandIndex; // The maximum index found. Eg: i32:0 -> maxIndex = 0, i32:0, i32:1 -> maxIndex = 1. Used
                              // to calculate query table dimension.
    std::pmr::vector<LegalizeActionClauseSymbol> m_clauses;
};

struct LegalizeRuleOperandSymbol
{
    DSL::Ast::LegalizeRuleDef::RuleOperandKind m_kind;
    std::string_view m_name;
    std::optional<SymbolId> m_typeOrClassId;
    std::optional<int64_t> m_immLiteral;
};

/**
 * Semantic symbol for an instruction in a rewrite rule pattern or template (.lrd).
 */
struct LegalizeRuleInstructionSymbol
{
    std::string_view m_opcode;
    std::pmr::vector<LegalizeRuleOperandSymbol> m_operands;
};

/**
 * Semantic symbol for an IR-to-IR legalization rewrite rule (.lrd).
 */
struct LegalizeRuleSymbol
{
    std::string_view m_ruleName;
    std::pmr::vector<LegalizeRuleInstructionSymbol> m_matchPatterns;
    std::pmr::vector<LegalizeRuleInstructionSymbol> m_expansionSequence;
};

}; // namespace Symbols

#endif // EZDSLSEMA_LEGALIZE_SYMBOLS_H