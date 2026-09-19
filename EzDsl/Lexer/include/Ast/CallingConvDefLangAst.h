#ifndef EZDSLLEXER_CALLING_CONV_DEF_LANG_AST_H
#define EZDSLLEXER_CALLING_CONV_DEF_LANG_AST_H

#include "CommonAstNodes.h"
#include "EzDslLexerCommon.h"

namespace DSL::Ast::CallingConvDef
{

/**
 * Direction in which the ABI stack grows as call frames are pushed.
 */
enum class StackGrowth : uint8_t
{
    Down, // Stack grows toward lower addresses on push (e.g. x86).
    Up    // Stack grows toward higher addresses on push.
};

/**
 * Identifies which side restores the stack pointer once a callee returns.
 */
enum class StackCleanup : uint8_t
{
    Caller, // The caller pops the arguments after the call.
    Callee  // The callee tears down its own frame and pops arguments.
};

/**
 * Governs whether a by-value aggregate may be split between registers and memory or must be
 * passed entirely in one place.
 */
enum class AllocPolicy : uint8_t
{
    AllOrNothing,    // Either the whole aggregate fits in registers or the whole value goes to the stack.
    SplitRegAndStack // Remaining fields may continue on the stack after registers are exhausted.
};

/**
 * Stack layout parameters of a calling convention: alignment, growth direction, cleanup
 * responsibility, optional shadow space/red zone, and the SP/FP/LR register names.
 */
struct StackDef
{
    Common::IntegerLiteral m_alignment;               // Required stack alignment in bytes (power of two).
    StackGrowth m_growth;                             // Direction the stack grows.
    StackCleanup m_cleanup;                           // Side responsible for stack cleanup.
    Common::IntegerLiteral m_shadowSpace;             // 0 if unused
    Common::Identifier m_stackPointer;                // Register acting as the stack pointer.
    Common::Identifier m_framePointer;                // Register acting as the frame pointer.
    std::optional<Common::IntegerLiteral> m_redZone;  // Optional area below SP usable without probing.
    std::optional<Common::Identifier> m_linkRegister; // Optional link register holding the return address.
};

/**
 * A single predicate evaluated over an aggregate argument together with the ABI class
 * assigned when the predicate matches.
 */
struct AggregateCondition
{
    enum class Kind : uint8_t
    {
        NonTrivial,           // Aggregate has a non-trivial copy/destructor.
        Unaligned,            // Aggregate's alignment is not naturally satisfied.
        SizeGreaterThan,      // Aggregate size exceeds m_sizeLimit.
        SizeLessThanOrEqual,  // Aggregate size is at most m_sizeLimit.
        SizeIn,               // Aggregate size is one of m_sizeSet.
        HomogeneousAggregate, // e.g. HFA/HVA
        Default               // Fallback applied when no earlier condition matched.
    } m_kind;

    std::optional<Common::IntegerLiteral> m_sizeLimit;    // Threshold used by the size comparison kinds.
    std::pmr::vector<Common::IntegerLiteral> m_sizeSet;   // Explicit accepted sizes for SizeIn.
    std::optional<Common::Identifier> m_homoBaseType;     // e.g. "float"
    std::optional<Common::IntegerLiteral> m_homoMaxCount; // e.g. 4

    Common::Identifier m_resultClass; // e.g. "integer", "by_ref", "memory"
    bool m_implicitCopy{ false };     // True for Win64-style by_ref stack copy
};

/**
 * Ordered aggregate classification pipeline: conditions are evaluated top to bottom and the
 * first match wins. Slice chunking splits large aggregates before re-classification, and
 * merge precedence resolves conflicts when several classifications apply.
 */
struct AggregatePipeline
{
    std::pmr::vector<AggregateCondition> m_conditions;      // Ordered condition/result pairs.
    std::optional<Common::IntegerLiteral> m_sliceChunkSize; // e.g. 8 bytes for SysV eightbytes
    std::pmr::vector<Common::Identifier> m_mergePrecedence; // Class order used to break ties.
    AllocPolicy m_policy{ AllocPolicy::AllOrNothing };      // Whether register/stack splitting is allowed.
};

/**
 * Maps a set of primitive source types to the ABI class used to pass them
 * (e.g. [float, double] => sse).
 */
struct PrimitiveRule
{
    std::pmr::vector<Common::Identifier> m_types; // Source type names covered by this rule.
    Common::Identifier m_targetClass;             // ABI class assigned to those types.
};

/**
 * Full classification descriptor, combining direct primitive-type rules with an optional
 * aggregate classification pipeline.
 */
struct ClassificationDef
{
    std::pmr::vector<PrimitiveRule> m_primitives; // Rules for scalar/primitive arguments.
    std::optional<AggregatePipeline> m_aggregate; // Optional pipeline for aggregate arguments.
};

/**
 * Stack slot used when a value cannot be passed in registers: mandatory slot size and an
 * optional explicit alignment override.
 */
struct StackFallback
{
    Common::IntegerLiteral m_slotSize;                 // Bytes reserved on the stack for the value.
    std::optional<Common::IntegerLiteral> m_alignment; // Optional alignment override for the slot.
};

/**
 * Ordered register pool from which argument registers are drawn.
 */
struct RegisterSequence
{
    enum class Kind : uint8_t
    {
        Sequential,      // Take the next free register from the pool.
        ConsecutiveBlock // Require the whole register range to be contiguous.
    } m_kind{ Kind::Sequential };
    std::pmr::vector<Common::Identifier> m_registers; // Candidate registers in allocation order.
};

/**
 * Binds a single ABI class to the physical register that carries it inside a unified slot.
 */
struct SlotBinding
{
    Common::Identifier m_abiClass; // ABI class occupying this part of the slot.
    Common::Identifier m_register; // Register that carries the class.
};

/**
 * One argument slot for slot-model ABIs (e.g. Win64) that may hold several ABI classes bound
 * to different registers.
 */
struct UnifiedSlot
{
    std::pmr::vector<SlotBinding> m_bindings; // e.g. [{ "integer", rcx }, { "float", xmm0 }]
};

/**
 * Rule describing how a single ABI class is passed: an explicit register sequence, an alias to
 * another class, or pure stack (monostate), plus an optional stack fallback.
 */
struct PassRule
{
    Common::Identifier m_abiClass; // ABI class this rule applies to.

    // Direct register pool, alias/forward (e.g. by_ref => integer), or pure stack (monostate)
    std::variant<RegisterSequence, Common::Identifier, std::monostate> m_source;
    std::optional<StackFallback> m_fallback; // Slot used when the rule cannot be satisfied in registers.
};

/**
 * Complete argument-passing description for a calling convention, supporting both unified-slot
 * ABIs and per-class register-bank rules, with a default stack fallback.
 */
struct ArgumentPassingDef
{
    std::pmr::vector<UnifiedSlot> m_unifiedSlots;        // Slot-based ABIs (e.g. Win64)
    std::pmr::vector<PassRule> m_rules;                  // Bank-based ABIs (e.g. SysV, AAPCS)
    std::optional<StackFallback> m_defaultStackFallback; // Fallback used when no rule matches.
};

/**
 * Small-struct return convention: the hidden pointer register, whether it consumes a normal
 * argument slot, and an optional register echoing the returned pointer.
 */
struct StructReturnDef
{
    Common::Identifier m_pointerRegister;               // Register receiving the hidden return-address pointer.
    bool m_consumesArgSlot;                             // Whether the hidden pointer occupies an argument slot.
    std::optional<Common::Identifier> m_returnRegister; // e.g. RAX echoed return
};

/**
 * Return-value description: optional structure-return convention plus per-class return rules.
 */
struct ReturnDef
{
    std::optional<StructReturnDef> m_sret; // Present when large structs are returned indirectly.
    std::pmr::vector<PassRule> m_rules;    // Per-ABI-class rules for returning values.
};

/**
 * Variadic-call ABI details, such as the register holding the vector argument count and any
 * target-specific argument duplication or alignment requirements.
 */
struct VarargsDef
{
    std::optional<Common::Identifier> m_vectorCountReg; // Register carrying the vector arg count (e.g. AL on SysV).
    bool m_duplicateFloatsToGpr{ false };               // Whether float varargs are also placed in GPRs.
    std::optional<Common::IntegerLiteral> m_stackAlign; // Optional alignment required for the varargs area.
};

/**
 * Complete parsed calling convention declaration, aggregating stack layout, preserved register
 * sets, classification, argument/return rules, and optional varargs handling.
 */
struct CallingConventionDecl
{
    Common::Identifier m_name; // Convention name used as its symbol.
    StackDef m_stack;          // Stack layout parameters.

    std::pmr::vector<Common::Identifier> m_calleeSaved; // Registers the callee must preserve.
    std::pmr::vector<Common::Identifier> m_callerSaved; // Registers the caller must preserve.

    ClassificationDef m_classification;  // Type-to-ABI-class classification rules.
    ArgumentPassingDef m_arguments;      // Argument passing rules.
    ReturnDef m_returns;                 // Return value rules.
    std::optional<VarargsDef> m_varargs; // Variadic-call handling, if declared.
};

/**
 * Root AST node for a parsed calling convention file: mirrors the first convention as the
 * primary declaration and retains the full list of conventions declared in the file.
 */
struct CallingConventionDefFile : public CallingConventionDecl
{
    std::pmr::vector<CallingConventionDecl> m_conventions; // All conventions in declaration order.
};

} // namespace DSL::Ast::CallingConvDef

#endif // EZDSLLEXER_CALLING_CONV_DEF_LANG_AST_H