#ifndef EZDSL_CALLING_CONV_DEF_LANG_AST_H
#define EZDSL_CALLING_CONV_DEF_LANG_AST_H

#include "EzDslCommon.h"
#include "CommonAstNodes.h"
#include "Ast/InstructionDefLangAst.h"

namespace DSL::Ast::CallingConvDef
{
/**
 * Determines the direction of growth of the stack:
 *  - Down: Stack pointer is decremented.
 *  - Up:   Stack pointer is incremented.
 */
enum class StackDirection : uint8_t
{
    Down = 0,
    Up
};

/**
 * Determines who is responsible for cleaning up stack arguments.
 */
enum class StackCleaner : uint8_t
{
    Caller = 0,
    Callee
};

/**
 * Register allocation strategy for multi-register / chunk arguments.
 */
enum class AllocPolicy : uint8_t
{
    AllOrNothing = 0, // If any chunk/register is unavailable, the entire argument spills
    Independent       // Assign available registers and spill only the remaining parts
};

/**
 * Register assignment mechanism.
 */
enum class RegAssignKind : uint8_t
{
    Sequence, // Sequential register pool consumption (e.g., SysV: RDI, RSI, RDX...)
    Slots     // Paired argument slot consumption (e.g., Win64: Slot 0 = RCX/XMM0...)
};

/**
 * Predicate kind used in aggregate classification.
 */
enum class AggregatePredicateKind : uint8_t
{
    SizeGt,      // IF_SIZE_GT(N)
    SizeLe,      // IF_SIZE_LE(N)
    SizeIn,      // IF_SIZE_IN(1, 2, 4, 8)
    Homogeneous, // IF_HOMOGENEOUS(FLOAT, MAX: 4)
    Unaligned,   // IF_UNALIGNED
    NonTrivial,  // IF_NON_TRIVIAL
    Default      // DEFAULT
};

/**
 * Action to perform when lowering an argument or return value.
 */
enum class LoweringActionKind : uint8_t
{
    RegisterAssign, // REG_SEQ(...) or REG_SLOTS(...)
    ExpandTo,       // EXPAND_TO(targetClass) (e.g. HFA unpacking)
    PassAsPointer,  // PASS_AS_POINTER >> targetClass (e.g. ByRef)
    Stack,          // Directly allocate to stack
    Sret            // Lower to hidden struct-return pointer
};

/**
 * A reference to a register (e.g. GPR:rdi, FPR:xmm0).
 */
struct RegisterRef
{
    Common::Identifier m_className;
    Common::Identifier m_regName;
};

/**
 * Stack placement configuration and alignment constraints.
 */
struct StackPlacement
{
    std::optional<Common::IntegerLiteral> m_alignment;
};

/**
 * Scalar primitive classification rule:
 * TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;
 */
struct PrimitiveClassifyRule
{
    std::pmr::vector<Common::Identifier> m_types;
    Common::Identifier m_targetClass;
};

/**
 * Aggregate classification predicate:
 * IF_SIZE_GT(16) >> MEMORY;
 * IF_HOMOGENEOUS(FLOAT, MAX: 4) >> HFA;
 */
struct AggregatePredicate
{
    AggregatePredicateKind m_kind;
    std::optional<Common::IntegerLiteral> m_size;         // For SizeGt, SizeLe
    std::pmr::vector<Common::IntegerLiteral> m_sizes;     // For SizeIn
    std::optional<Common::Identifier> m_homogeneousClass; // For Homogeneous (e.g. FLOAT)
    std::optional<Common::IntegerLiteral> m_maxElements;  // For Homogeneous MAX
    Common::Identifier m_resultClass;                     // Target ABI class (e.g. MEMORY, HFA, BY_REF)
};

/**
 * Aggregate slicing, precedence, and policy definition:
 * AGGREGATE { IF_SIZE_GT(16) >> MEMORY; CHUNK_SIZE(8); MERGE_PRECEDENCE >> ... };
 */
struct AggregateClassifyDef
{
    std::pmr::vector<AggregatePredicate> m_predicates;
    std::optional<Common::IntegerLiteral> m_chunkSize;      // Slicing chunk size (e.g., 8 bytes)
    std::pmr::vector<Common::Identifier> m_mergePrecedence; // Precedence chain: MEMORY > INTEGER > FLOAT
    AllocPolicy m_allocPolicy{ AllocPolicy::AllOrNothing };
};

/**
 * The CLASSIFY block grouping all type-to-class rules.
 */
struct ClassifyBlock
{
    std::pmr::vector<PrimitiveClassifyRule> m_primitiveRules;
    std::optional<AggregateClassifyDef> m_aggregateDef;
};

/**
 * Action descriptor detailing how an ABI class is passed or returned.
 */
struct LoweringAction
{
    LoweringActionKind m_kind;
    RegAssignKind m_regAssignKind{ RegAssignKind::Sequence };
    std::pmr::vector<RegisterRef> m_registers;       // Candidate registers
    std::optional<Common::Identifier> m_targetClass; // Target for ExpandTo / PassAsPointer
    std::optional<StackPlacement> m_stackFallback;   // Fallback: >> STACK(ALIGN: 8)
};

/**
 * Mapping rule from an ABI class to a lowering action:
 * INTEGER >> REG_SEQ(GPR:rdi, GPR:rsi, ...) >> STACK(ALIGN: 8);
 */
struct DispatchRule
{
    Common::Identifier m_abiClass;
    LoweringAction m_action;
};

/**
 * Struct Return (SRET) specific ABI behavior configuration.
 */
struct SretConfig
{
    RegisterRef m_passInReg;                // Register carrying pointer (e.g., GPR:rdi or GPR:x8)
    bool m_consumesArgSlot{ true };         // True if it steals the first argument register
    std::optional<RegisterRef> m_returnReg; // Echoed return register (e.g., GPR:rax or std::nullopt)
};

struct CallingConvDefFile
{
    Common::Identifier m_name;

    // Stack & Frame Layout
    Common::IntegerLiteral m_stackAlign;
    StackDirection m_stackDirection{ StackDirection::Down };
    StackCleaner m_stackCleanup{ StackCleaner::Caller };
    Common::IntegerLiteral m_shadowSpace{ 0 };
    Common::IntegerLiteral m_redZone{ 0 };

    RegisterRef m_stackPointer;
    RegisterRef m_framePointer;

    // Register Preservation
    std::pmr::vector<RegisterRef> m_calleeSaved;
    std::pmr::vector<RegisterRef> m_callerSaved;

    // 1. Classification
    ClassifyBlock m_classify;

    // 2. Argument Dispatch (PASS)
    std::pmr::vector<DispatchRule> m_passRules;

    // 3. Return Dispatch & SRET (RETURN)
    std::pmr::vector<DispatchRule> m_returnRules;
    std::optional<SretConfig> m_sretConfig;
};

} // namespace DSL::Ast::CallingConvDef

#endif // EZDSL_CALLING_CONV_DEF_LANG_AST_H