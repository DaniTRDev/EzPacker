#ifndef EZDSL_SYMBOLS_H
#define EZDSL_SYMBOLS_H

#include "EzDslCommon.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

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
enum class BitExprOp;
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
/**
 * Semantic symbol for a parsed primitive or special IR type declaration (.tyf).
 */
struct TypeSymbol
{
    std::string_view m_name;
    DSL::Ast::TypeDef::TypeKind m_kind;
    uint32_t m_bitWidth{ 0 };
    uint32_t m_alignment{ 0 };
    uint8_t m_compactId{ 0 }; // Fast O(1) index for table driven legalizer/selector.
};

/**
 * Semantic symbol for an architecture hardware register definition (.tdf).
 */
struct RegisterSymbol
{
    std::string_view m_name;
    std::optional<SymbolId> m_parentId; // Resolved parent register ID (e.g. rax for eax)
    uint32_t m_bitSize{ 0 };
    uint32_t m_bitOffset{ 0 };
    SymbolId m_primaryClassId{ InvalidSymbolId };
};

/**
 * Semantic symbol for a register class grouping (.tdf).
 */
struct RegisterClassSymbol
{
    std::string_view m_name;
    SymbolId m_bankId{ InvalidSymbolId };
    std::pmr::vector<SymbolId> m_registers; // Resolved RegisterSymbol IDs
};

/**
 * Semantic symbol for a register bank container (.tdf).
 */
struct RegisterBankSymbol
{
    std::string_view m_name;
    std::pmr::vector<SymbolId> m_classes; // Resolved RegisterClassSymbol IDs
};

/**
 * Resolved bit slice range [from, to].
 */
struct BitSlice
{
    uint16_t m_from{ 0 };
    uint16_t m_to{ 0 };
};

struct ResolvedBitExpr;

/**
 * Resolved reference to an instruction operand with optional bit slicing.
 */
struct SlicedOperandRef
{
    std::string_view m_operandName;
    SymbolId m_operandTypeId{ InvalidSymbolId }; // Resolved TypeId/ClassId
    BitSlice m_slice;
};

using ResolvedBitExprValue =
        std::variant<uint64_t,                        // Folded compile-time constant (e.g. 0x33, 1 << 2)
                     SlicedOperandRef,                // Direct or sliced instruction operand (e.g. rd, imm12[0:4])
                     std::shared_ptr<ResolvedBitExpr> // Composite binary/unary expression tree
                     >;

/**
 * Resolved bit expression tree with operator and evaluated child operands.
 */
struct ResolvedBitExpr
{
    ResolvedBitExprValue m_lhs;
    DSL::Ast::InstDef::BitExprOp m_op;
    std::optional<ResolvedBitExprValue> m_rhs;
};

/**
 * Semantic symbol for a named bitfield in an instruction encoding format (.idf).
 */
struct FormatFieldSymbol
{
    std::string_view m_name;
    BitSlice m_slice;
    std::optional<ResolvedBitExprValue> m_defaultValue; // Folded constant or default bit expression
};

/**
 * Semantic symbol for an instruction binary encoding format (.idf).
 */
struct InstructionFormatSymbol
{
    std::string_view m_name;
    uint32_t m_bitWidth{ 32 };
    std::pmr::vector<FormatFieldSymbol> m_fields;
};

/**
 * Semantic symbol for a formal argument operand of a target instruction (.idf).
 */
struct TargetOperandSymbol
{
    DSL::Ast::InstDef::InstOperandKind m_kind;
    SymbolId m_typeOrClassId{ InvalidSymbolId }; // RegisterClass ID or Type ID
    std::string_view m_name;
    DSL::Ast::InstDef::InstOperandDir m_dir;
};

/**
 * Semantic symbol for a format field assignment in an instruction declaration (.idf).
 */
struct FieldAssignmentSymbol
{
    std::string_view m_fieldName;
    BitSlice m_fieldSlice;        // Slice within target field ([0, width-1] if unsliced)
    ResolvedBitExprValue m_value; // Constant, operand reference, or expression tree
};

/**
 * Semantic symbol representing a fully resolved target architecture instruction (.idf).
 */
struct TargetInstructionSymbol
{
    std::string_view m_name;
    SymbolId m_formatId{ InvalidSymbolId };
    std::pmr::vector<FieldAssignmentSymbol> m_fieldAssignments; // Resolved overrides
    std::pmr::vector<TargetOperandSymbol> m_args;
    std::pmr::vector<TargetOperandSymbol> m_implicitArgs;
    std::string_view m_asmTemplate;
    uint32_t m_latency{ 1 };
    uint32_t m_flagsMask{ 0 }; // Bitmask of DSL::Ast::InstDef::InstFlag

    /**
     * Checks if the instruction has the specified behavioral flag set.
     */
    [[nodiscard]] bool hasFlag(DSL::Ast::InstDef::InstFlag flag) const noexcept
    {
        return (m_flagsMask & (1u << static_cast<uint32_t>(flag))) != 0;
    }
};

/**
 * Semantic symbol for an IR instruction operand slot (.irdf).
 */
struct IrOperandSymbol
{
    DSL::Ast::IrInstDef::IrOperandType m_typeMask;
    std::string_view m_name;
    DSL::Ast::IrInstDef::IrOperandDir m_dir;
};

/**
 * Semantic symbol for an intermediate representation (IR) instruction opcode (.irdf).
 */
struct IrInstructionSymbol
{
    std::string_view m_name;
    DSL::Ast::IrInstDef::IrInstCategory m_category;
    DSL::Ast::IrInstDef::IrInstTier m_tier;
    DSL::Ast::IrInstDef::IrInstFlag m_flagsMask;
    std::pmr::vector<IrOperandSymbol> m_operands;

    /**
     * Checks if the IR instruction has the specified verification or behavioral flag set.
     */
    [[nodiscard]] bool hasFlag(DSL::Ast::IrInstDef::IrInstFlag flagMask) const noexcept
    {
        return (static_cast<uint32_t>(m_flagsMask) & static_cast<uint32_t>(flagMask)) != 0;
    }
};

/**
 * Semantic symbol representing a type constraint on a legalized operand slot (.lad).
 */
struct LegalizeConstraintSymbol
{
    SymbolId m_typeId{ InvalidSymbolId };   // Resolved type ID (e.g. i32)
    std::optional<uint32_t> m_operandIndex; // Operand slot index (0, 1, etc.)
};

/**
 * Semantic symbol for a legalization directive clause (.lad).
 */
struct LegalizeClauseSymbol
{
    DSL::Ast::LegalizeActionDef::LegalizeActionKind m_kind;
    std::pmr::vector<LegalizeConstraintSymbol> m_types;
    std::optional<SymbolId> m_targetTypeId; // Widen/Narrow/Bitcast target
    std::optional<std::string_view> m_libcallSymbol;
    std::optional<std::pmr::vector<SymbolId>> m_customRules;
};

/**
 * Semantic symbol grouping all legalization rules for a generic IR opcode (.lad).
 */
struct LegalizeActionSymbol
{
    std::string_view m_genericOpcode;
    size_t m_maxOperandIndex; // The maximum index found. Eg: i32:0 -> maxIndex = 0, i32:0, i32:1 -> maxIndex = 1. Used
                              // to calculate query table dimension.
    std::pmr::vector<LegalizeClauseSymbol> m_clauses;
};

/**
 * Semantic symbol for an operand in a rewrite rule pattern or template (.lrd).
 */
struct RuleOperandSymbol
{
    DSL::Ast::LegalizeRuleDef::OperandKind m_kind;
    std::string_view m_name;
    std::optional<SymbolId> m_typeOrClassId;
    std::optional<int64_t> m_immLiteral;
};

/**
 * Semantic symbol for an instruction in a rewrite rule pattern or template (.lrd).
 */
struct RuleInstructionSymbol
{
    std::string_view m_opcode;
    std::pmr::vector<RuleOperandSymbol> m_operands;
};

/**
 * Semantic symbol for an IR-to-IR legalization rewrite rule (.lrd).
 */
struct LegalizeRewriteRuleSymbol
{
    std::string_view m_ruleName;
    std::pmr::vector<RuleInstructionSymbol> m_matchPatterns;
    std::pmr::vector<RuleInstructionSymbol> m_expansionSequence;
};

/**
 * Semantic symbol for an addressing mode matching variant (.isf).
 */
struct AddrModeVariantSymbol
{
    std::string_view m_name;
    std::pmr::vector<RuleInstructionSymbol> m_matchPatterns;
};

/**
 * Semantic symbol for an addressing mode aggregate definition (.isf).
 */
struct AddrModeSymbol
{
    std::string_view m_name;
    std::pmr::vector<TargetOperandSymbol> m_parameters;
    std::pmr::vector<AddrModeVariantSymbol> m_variants;
};

/**
 * Semantic symbol for an instruction selection (ISel) pattern rule (.isf).
 */
struct ISelPatternSymbol
{
    std::string_view m_patternName;
    std::pmr::vector<RuleInstructionSymbol> m_matchPatterns;
    std::pmr::vector<RuleInstructionSymbol> m_emitSequence;
    uint32_t m_cost{ 1 };
};

/**
 * Semantic symbol referencing a target register with its class ID and register ID.
 */
struct RegisterRefSymbol
{
    SymbolId m_classId{ InvalidSymbolId };
    SymbolId m_registerId{ InvalidSymbolId };
};

/**
 * Semantic symbol for an aggregate classification predicate in calling conventions (.cdf).
 */
struct AggregatePredicateSymbol
{
    DSL::Ast::CallingConvDef::AggregatePredicateKind m_kind;
    std::optional<int64_t> m_size;
    std::pmr::vector<int64_t> m_sizes;
    std::optional<std::string_view> m_homogeneousClass;
    std::optional<int64_t> m_maxElements;
    std::string_view m_resultClass;
};

/**
 * Semantic symbol for an aggregate classification configuration block (.cdf).
 */
struct AggregateClassifySymbol
{
    std::pmr::vector<AggregatePredicateSymbol> m_predicates;
    std::optional<int64_t> m_chunkSize;
    std::pmr::vector<std::string_view> m_mergePrecedence;
    DSL::Ast::CallingConvDef::AllocPolicy m_allocPolicy;
};

/**
 * Semantic symbol for an argument/return lowering action (.cdf).
 */
struct LoweringActionSymbol
{
    DSL::Ast::CallingConvDef::LoweringActionKind m_kind;
    DSL::Ast::CallingConvDef::RegAssignKind m_regAssignKind;
    std::pmr::vector<RegisterRefSymbol> m_registers;
    std::optional<std::string_view> m_targetClass;
    std::optional<uint32_t> m_stackFallbackAlign;
};

/**
 * Semantic symbol mapping an ABI class to a lowering action (.cdf).
 */
struct DispatchRuleSymbol
{
    std::string_view m_abiClass;
    LoweringActionSymbol m_action;
};

/**
 * Semantic symbol for struct-return (SRET) convention configuration (.cdf).
 */
struct SretConfigSymbol
{
    RegisterRefSymbol m_passInReg;
    bool m_consumesArgSlot{ true };
    std::optional<RegisterRefSymbol> m_returnReg;
};

/**
 * Semantic symbol representing a complete target calling convention definition (.cdf).
 */
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

/**
 * Root semantic symbol encapsulating all banks, instructions, calling conventions, and rules for a target architecture.
 */
struct TargetSymbol
{
    std::string_view m_name;
    std::pmr::vector<SymbolId> m_banks;
    std::pmr::vector<SymbolId> m_instructions;
    std::pmr::vector<SymbolId> m_callingConvs;
    std::pmr::vector<SymbolId> m_legalizeActions;
    std::pmr::vector<SymbolId> m_iselPatterns;
};

} // namespace Sema::Symbols

#endif // EZDSL_SYMBOLS_H