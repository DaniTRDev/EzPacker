#ifndef EZDSL_CALLING_CONV_DEF_LANG_AST_H
#define EZDSL_CALLING_CONV_DEF_LANG_AST_H

#include "EzDslCommon.h"
#include "CommonAstNodes.h"
#include "Ast/InstructionDefLangAst.h"

namespace DSL::Ast::CallingConvDef
{
/**
 * Determines the direction of growth of the stack.
 *
 * Valid keywords:
 *   'DOWN' -> Stack pointer decrements on allocation
 *   'UP'   -> Stack pointer increments on allocation
 */
enum class StackDirection : uint8_t
{
    Down = 0,
    Up
};

/**
 * Determines who is responsible for cleaning up stack arguments.
 *
 * Valid keywords:
 *   'CALLER', 'CALLEE'
 */
enum class StackCleaner : uint8_t
{
    Caller = 0,
    Callee
};

/**
 * Register allocation strategy for multi-register / chunk arguments.
 *
 * Valid keywords:
 *   'ALL_OR_NOTHING', 'INDEPENDENT'
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
 * Register reference with register class qualifier.
 *
 * Syntax:
 *   RegisterRef := ClassName ':' RegName
 *   ClassName   := Identifier
 *   RegName     := Identifier
 *
 * Examples:
 *   GPR:rdi
 *   FPR:xmm0
 */
struct RegisterRef
{
    Common::Identifier m_className;
    Common::Identifier m_regName;
};

/**
 * Stack placement configuration and alignment constraints.
 *
 * Syntax:
 *   StackPlacement := 'STACK' ( '(' ( 'ALIGN' ':' )? IntegerLiteral ')' )?
 *
 * Examples:
 *   STACK
 *   STACK(8)
 *   STACK(ALIGN: 16)
 */
struct StackPlacement
{
    std::optional<Common::IntegerLiteral> m_alignment;
};

/**
 * Scalar primitive classification rule in a calling convention.
 *
 * Syntax:
 *   PrimitiveClassifyRule := 'TYPE' '(' TypeName (',' TypeName)* ')' '>>' AbiClass ';'
 *   TypeName              := Identifier
 *   AbiClass              := Identifier
 *
 * Example:
 *   TYPE(i1, i8, i16, i32, i64, ptr) >> INTEGER;
 */
struct PrimitiveClassifyRule
{
    std::pmr::vector<Common::Identifier> m_types;
    Common::Identifier m_targetClass;
};

/**
 * Aggregate classification predicate mapping aggregate properties to ABI classes.
 *
 * Syntax:
 *   AggregatePredicate := PredicateBranch ';'
 *   PredicateBranch    := ( 'IF_SIZE_GT' '(' IntegerLiteral ')'
 *                         | 'IF_SIZE_LE' '(' IntegerLiteral ')'
 *                         | 'IF_SIZE_IN' '(' IntegerLiteral (',' IntegerLiteral)* ')'
 *                         | 'IF_HOMOGENEOUS' '(' ClassName ',' ( 'MAX' ':' )? IntegerLiteral ')'
 *                         | 'IF_UNALIGNED'
 *                         | 'IF_NON_TRIVIAL'
 *                         | 'DEFAULT' ) '>>' ResultClass
 *
 * Examples:
 *   IF_SIZE_GT(16) >> MEMORY;
 *   IF_HOMOGENEOUS(FLOAT, MAX: 4) >> HFA;
 *   DEFAULT >> INTEGER;
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
 * Aggregate slicing, precedence, and policy definition block.
 *
 * Syntax:
 *   AggregateClassifyDef := 'AGGREGATE' '{' ( AggregateItem )* '}' ';'?
 *   AggregateItem        := AggregatePredicate | ChunkSizeDecl | MergePrecedenceDecl | AllocPolicyDecl
 *   ChunkSizeDecl        := 'CHUNK_SIZE' '(' IntegerLiteral ')' ';'
 *   MergePrecedenceDecl  := 'MERGE_PRECEDENCE' '>>' ClassName ( '>' ClassName )* ';'
 *   AllocPolicyDecl      := 'ALLOC_POLICY' '(' ('ALL_OR_NOTHING' | 'INDEPENDENT') ')' ';'
 */
struct AggregateClassifyDef
{
    std::pmr::vector<AggregatePredicate> m_predicates;
    std::optional<Common::IntegerLiteral> m_chunkSize;      // Slicing chunk size (e.g., 8 bytes)
    std::pmr::vector<Common::Identifier> m_mergePrecedence; // Precedence chain: MEMORY > INTEGER > FLOAT
    AllocPolicy m_allocPolicy{ AllocPolicy::AllOrNothing };
};

/**
 * CLASSIFY block grouping primitive and aggregate classification rules.
 *
 * Syntax:
 *   ClassifyBlock := 'CLASSIFY' '{' ( PrimitiveClassifyRule | AggregateClassifyDef )* '}' ';'?
 */
struct ClassifyBlock
{
    std::pmr::vector<PrimitiveClassifyRule> m_primitiveRules;
    std::optional<AggregateClassifyDef> m_aggregateDef;
};

/**
 * Action descriptor detailing how an ABI class is passed or returned.
 *
 * Syntax:
 *   LoweringAction := 'REG_SEQ' '(' RegisterRef (',' RegisterRef)* ')' ( '>>' StackPlacement )?
 *                   | 'REG_SLOTS' '(' RegisterRef (',' RegisterRef)* ')' ( '>>' StackPlacement )?
 *                   | 'EXPAND_TO' '(' ClassName ')'
 *                   | 'PASS_AS_POINTER' '>>' ClassName
 *                   | StackPlacement
 *                   | 'SRET'
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
 * Mapping rule from an ABI class to a lowering action.
 *
 * Syntax:
 *   DispatchRule := AbiClass '>>' LoweringAction ';'
 *   AbiClass     := Identifier
 *
 * Examples:
 *   INTEGER >> REG_SEQ(GPR:rdi, GPR:rsi, GPR:rdx) >> STACK(ALIGN: 8);
 *   FLOAT   >> REG_SEQ(FPR:xmm0, FPR:xmm1) >> STACK(8);
 */
struct DispatchRule
{
    Common::Identifier m_abiClass;
    LoweringAction m_action;
};

/**
 * Struct Return (SRET) specific ABI behavior configuration.
 *
 * Syntax:
 *   SretConfig := 'SRET_CONFIG' '{' ( SretItem )* '}' ';'?
 *   SretItem   := 'PASS_IN_REG' '(' RegisterRef ')' ';'
 *               | 'CONSUMES_ARG_SLOT' '(' ('true' | 'false') ')' ';'
 *               | 'RETURN_REG' '(' ( RegisterRef | 'NONE' ) ')' ';'
 */
struct SretConfig
{
    RegisterRef m_passInReg;                // Register carrying pointer (e.g., GPR:rdi or GPR:x8)
    bool m_consumesArgSlot{ true };         // True if it steals the first argument register
    std::optional<RegisterRef> m_returnReg; // Echoed return register (e.g., GPR:rax or std::nullopt)
};

/**
 * Complete top-level calling convention definition AST root in a .cdf file.
 *
 * Syntax:
 *   CallingConvDefFile := 'calling_conv' ConvName '{' ( Directive )* '}' ';'? EOF
 *   ConvName  := Identifier
 *   Directive := 'STACK_ALIGN' '(' IntegerLiteral ')' ';'
 *              | 'STACK_DIRECTION' '(' ('DOWN' | 'UP') ')' ';'
 *              | 'STACK_CLEANUP' '(' ('CALLER' | 'CALLEE') ')' ';'
 *              | 'SHADOW_SPACE' '(' IntegerLiteral ')' ';'
 *              | 'STACK_POINTER' '(' RegisterRef ')' ';'
 *              | 'FRAME_POINTER' '(' RegisterRef ')' ';'
 *              | 'CALLEE_SAVED' '(' RegisterRef (',' RegisterRef)* ')' ';'
 *              | 'CALLER_SAVED' '(' RegisterRef (',' RegisterRef)* ')' ';'
 *              | ClassifyBlock
 *              | 'PASS' '{' ( DispatchRule )* '}' ';'?
 *              | 'RETURN' '{' ( DispatchRule | SretConfig )* '}' ';'?
 */
struct CallingConvDefFile
{
    Common::Identifier m_name;

    // Stack & Frame Layout
    Common::IntegerLiteral m_stackAlign{ 0 };
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