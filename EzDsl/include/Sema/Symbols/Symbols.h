#ifndef EZDSL_SYMBOLS_H
#define EZDSL_SYMBOLS_H

#include "EzDslCommon.h"

using SymbolId = size_t;
inline constexpr SymbolId InvalidSymbolId = UINT64_MAX;

// Forward declarations of shared AST enums to avoid heavy AST includes
namespace DSL::Ast
{
namespace CallingConvDef
{
enum class StackDirection : uint8_t;
enum class StackCleaner : uint8_t;
enum class AllocPolicy : uint8_t;
enum class RegAssignKind : uint8_t;
enum class AggregatePredicateKind : uint8_t;
enum class LoweringActionKind : uint8_t;
} // namespace CallingConvDef

namespace IrInstDef
{
enum class IrInstCategory : uint8_t;
enum class IrInstTier : uint8_t;
enum class IrInstFlag : uint32_t;
enum class IrOperandType : uint16_t;
enum class IrOperandDir : uint8_t;
} // namespace IrInstDef

namespace InstDef
{
enum class InstOperandKind : uint8_t;
enum class InstOperandDir : uint8_t;
enum class InstFlag : uint8_t;
} // namespace InstDef

namespace LegalizeActionDef
{
enum class LegalizeActionKind : uint8_t;
}

namespace LegalizeRuleDef
{
enum class OperandKind : uint8_t;
}

namespace TypeDef
{
enum class TypeKind : uint8_t;
}
} // namespace DSL::Ast

namespace Sema::Symbols
{
struct TypeSymbol
{
    std::string_view m_name;
    DSL::Ast::TypeDef::TypeKind m_kind;
    uint32_t m_bitWidth{ 0 };
};

struct RegisterSymbol
{
    std::string_view m_name;
    std::optional<SymbolId> m_parentId; // Resolved parent register ID (e.g. rax for eax)
    uint32_t m_bitSize{ 0 };
    uint32_t m_bitOffset{ 0 };
    SymbolId m_primaryClassId{ InvalidSymbolId };
};

struct RegisterClassSymbol
{
    std::string_view m_name;
    SymbolId m_bankId{ InvalidSymbolId };
    std::pmr::vector<SymbolId> m_registers; // Resolved RegisterSymbol IDs
};

struct RegisterBankSymbol
{
    std::string_view m_name;
    std::pmr::vector<SymbolId> m_classes; // Resolved RegisterClassSymbol IDs
};

struct BitSlice
{
    uint16_t m_from{ 0 };
    uint16_t m_to{ 0 };
};

struct FormatFieldSymbol
{
    std::string_view m_name;
    BitSlice m_slice;
    std::optional<uint64_t> m_defaultValue;
};

struct InstructionFormatSymbol
{
    std::string_view m_name;
    uint32_t m_bitWidth{ 32 };
    std::pmr::vector<FormatFieldSymbol> m_fields;
};

struct TargetOperandSymbol
{
    DSL::Ast::InstDef::InstOperandKind m_kind;
    SymbolId m_typeOrClassId{ InvalidSymbolId }; // RegisterClass ID or Type ID
    std::string_view m_name;
    DSL::Ast::InstDef::InstOperandDir m_dir;
};

struct TargetInstructionSymbol
{
    std::string_view m_name;
    SymbolId m_formatId{ InvalidSymbolId };
    std::pmr::vector<TargetOperandSymbol> m_args;
    std::pmr::vector<TargetOperandSymbol> m_implicitArgs;
    std::string_view m_asmTemplate;
    uint32_t m_latency{ 1 };
    uint32_t m_flagsMask{ 0 }; // Bitmask of DSL::Ast::InstDef::InstFlag

    bool hasFlag(DSL::Ast::InstDef::InstFlag flag) const noexcept
    {
        return (m_flagsMask & (1u << static_cast<uint32_t>(flag))) != 0;
    }
};

struct IrOperandSymbol
{
    DSL::Ast::IrInstDef::IrOperandType m_typeMask;
    std::string_view m_name;
    DSL::Ast::IrInstDef::IrOperandDir m_dir;
};

struct IrInstructionSymbol
{
    std::string_view m_name;
    DSL::Ast::IrInstDef::IrInstCategory m_category;
    DSL::Ast::IrInstDef::IrInstTier m_tier;
    DSL::Ast::IrInstDef::IrInstFlag m_flagsMask;
    std::pmr::vector<IrOperandSymbol> m_operands;

    bool hasFlag(DSL::Ast::IrInstDef::IrInstFlag flagMask) const noexcept
    {
        return (static_cast<uint32_t>(m_flagsMask) & static_cast<uint32_t>(flagMask)) != 0;
    }
};

struct LegalizeConstraintSymbol
{
    SymbolId m_typeId{ InvalidSymbolId }; // Resolved type ID (e.g. i32)
    std::optional<uint32_t> m_typeIndex;  // Operand slot index (0, 1, etc.)
};

struct LegalizeClauseSymbol
{
    DSL::Ast::LegalizeActionDef::LegalizeActionKind m_kind;
    std::pmr::vector<LegalizeConstraintSymbol> m_types;
    std::optional<SymbolId> m_targetTypeId; // Widen/Narrow/Bitcast target
    std::optional<std::string_view> m_libcallSymbol;
};

struct LegalizeActionSymbol
{
    std::string_view m_genericOpcode;
    std::pmr::vector<LegalizeClauseSymbol> m_clauses;
};

struct RuleOperandSymbol
{
    DSL::Ast::LegalizeRuleDef::OperandKind m_kind;
    std::string_view m_name;
    std::optional<SymbolId> m_typeOrClassId;
    std::optional<int64_t> m_immLiteral;
};

struct RuleInstructionSymbol
{
    std::string_view m_opcode;
    std::pmr::vector<RuleOperandSymbol> m_operands;
};

struct LegalizeRewriteRuleSymbol
{
    std::string_view m_ruleName;
    std::pmr::vector<RuleInstructionSymbol> m_matchPatterns;
    std::pmr::vector<RuleInstructionSymbol> m_expansionSequence;
};

struct AddrModeVariantSymbol
{
    std::string_view m_name;
    std::pmr::vector<RuleInstructionSymbol> m_matchPatterns;
};

struct AddrModeSymbol
{
    std::string_view m_name;
    std::pmr::vector<TargetOperandSymbol> m_parameters;
    std::pmr::vector<AddrModeVariantSymbol> m_variants;
};

struct ISelPatternSymbol
{
    std::string_view m_patternName;
    std::pmr::vector<RuleInstructionSymbol> m_matchPatterns;
    std::pmr::vector<RuleInstructionSymbol> m_emitSequence;
    uint32_t m_cost{ 1 };
};

struct RegisterRefSymbol
{
    SymbolId m_classId{ InvalidSymbolId };
    SymbolId m_registerId{ InvalidSymbolId };
};

struct AggregatePredicateSymbol
{
    DSL::Ast::CallingConvDef::AggregatePredicateKind m_kind;
    std::optional<int64_t> m_size;
    std::pmr::vector<int64_t> m_sizes;
    std::optional<std::string_view> m_homogeneousClass;
    std::optional<int64_t> m_maxElements;
    std::string_view m_resultClass;
};

struct AggregateClassifySymbol
{
    std::pmr::vector<AggregatePredicateSymbol> m_predicates;
    std::optional<int64_t> m_chunkSize;
    std::pmr::vector<std::string_view> m_mergePrecedence;
    DSL::Ast::CallingConvDef::AllocPolicy m_allocPolicy;
};

struct LoweringActionSymbol
{
    DSL::Ast::CallingConvDef::LoweringActionKind m_kind;
    DSL::Ast::CallingConvDef::RegAssignKind m_regAssignKind;
    std::pmr::vector<RegisterRefSymbol> m_registers;
    std::optional<std::string_view> m_targetClass;
    std::optional<uint32_t> m_stackFallbackAlign;
};

struct DispatchRuleSymbol
{
    std::string_view m_abiClass;
    LoweringActionSymbol m_action;
};

struct SretConfigSymbol
{
    RegisterRefSymbol m_passInReg;
    bool m_consumesArgSlot{ true };
    std::optional<RegisterRefSymbol> m_returnReg;
};

struct CallingConvSymbol
{
    std::string_view m_name;

    // Stack configuration
    uint32_t m_stackAlign{ 16 };
    DSL::Ast::CallingConvDef::StackDirection m_stackDirection;
    DSL::Ast::CallingConvDef::StackCleaner m_stackCleanup;
    uint32_t m_shadowSpace{ 0 };
    uint32_t m_redZone{ 0 };

    RegisterRefSymbol m_stackPointer;
    RegisterRefSymbol m_framePointer;

    std::pmr::vector<RegisterRefSymbol> m_calleeSaved;
    std::pmr::vector<RegisterRefSymbol> m_callerSaved;

    // Classification & Rules
    std::pmr::vector<std::pair<SymbolId, std::string_view>> m_primitiveRules; // TypeId -> ABI Class
    std::optional<AggregateClassifySymbol> m_aggregateDef;
    std::pmr::vector<DispatchRuleSymbol> m_passRules;
    std::pmr::vector<DispatchRuleSymbol> m_returnRules;
    std::optional<SretConfigSymbol> m_sretConfig;
};

struct TargetSymbol
{
    std::string_view m_name;
    std::pmr::vector<SymbolId> m_banks;
    std::pmr::vector<SymbolId> m_instructions;
    std::pmr::vector<SymbolId> m_callingConvs;
    std::pmr::vector<SymbolId> m_legalizeActions;
    std::pmr::vector<SymbolId> m_iselPatterns;
};
}; // namespace Sema::Symbols

#endif // EZDSL_SYMBOLS_H