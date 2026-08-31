#ifndef EZDSLLEXER_CALLING_CONV_DEF_LANG_AST_H
#define EZDSLLEXER_CALLING_CONV_DEF_LANG_AST_H

#include "CommonAstNodes.h"
#include "EzDslLexerCommon.h"

namespace DSL::Ast::CallingConvDef
{

enum class StackGrowth : uint8_t
{
    Down,
    Up
};
enum class StackCleanup : uint8_t
{
    Caller,
    Callee
};
enum class AllocPolicy : uint8_t
{
    AllOrNothing,
    SplitRegAndStack
};

struct StackDef
{
    Common::IntegerLiteral m_alignment;
    StackGrowth m_growth;
    StackCleanup m_cleanup;
    Common::IntegerLiteral m_shadowSpace; // 0 if unused
    Common::Identifier m_stackPointer;
    Common::Identifier m_framePointer;
};

struct AggregateCondition
{
    enum class Kind : uint8_t
    {
        NonTrivial,
        Unaligned,
        SizeGreaterThan,
        SizeLessThanOrEqual,
        SizeIn,
        HomogeneousAggregate, // e.g. HFA/HVA
        Default
    } m_kind;

    std::optional<Common::IntegerLiteral> m_sizeLimit;
    std::pmr::vector<Common::IntegerLiteral> m_sizeSet;
    std::optional<Common::Identifier> m_homoBaseType;     // e.g. "float"
    std::optional<Common::IntegerLiteral> m_homoMaxCount; // e.g. 4

    Common::Identifier m_resultClass; // e.g. "integer", "by_ref", "memory"
    bool m_implicitCopy{ false };     // True for Win64-style by_ref stack copy
};

struct AggregatePipeline
{
    std::pmr::vector<AggregateCondition> m_conditions;
    std::optional<Common::IntegerLiteral> m_sliceChunkSize; // e.g. 8 bytes for SysV eightbytes
    std::pmr::vector<Common::Identifier> m_mergePrecedence;
    AllocPolicy m_policy{ AllocPolicy::AllOrNothing };
};

struct PrimitiveRule
{
    std::pmr::vector<Common::Identifier> m_types;
    Common::Identifier m_targetClass;
};

struct ClassificationDef
{
    std::pmr::vector<PrimitiveRule> m_primitives;
    std::optional<AggregatePipeline> m_aggregate;
};

struct StackFallback
{
    Common::IntegerLiteral m_slotSize;
    std::optional<Common::IntegerLiteral> m_alignment;
};

struct RegisterSequence
{
    enum class Kind : uint8_t
    {
        Sequential,
        ConsecutiveBlock
    } m_kind{ Kind::Sequential };
    std::pmr::vector<Common::Identifier> m_registers;
};

struct SlotBinding
{
    Common::Identifier m_abiClass;
    Common::Identifier m_register;
};

struct UnifiedSlot
{
    std::pmr::vector<SlotBinding> m_bindings; // e.g. [{ "integer", rcx }, { "float", xmm0 }]
};

struct PassRule
{
    Common::Identifier m_abiClass;

    // Direct register pool, alias/forward (e.g. by_ref => integer), or pure stack (monostate)
    std::variant<RegisterSequence, Common::Identifier, std::monostate> m_source;
    std::optional<StackFallback> m_fallback;
};

struct ArgumentPassingDef
{
    std::pmr::vector<UnifiedSlot> m_unifiedSlots; // Slot-based ABIs (e.g. Win64)
    std::pmr::vector<PassRule> m_rules;           // Bank-based ABIs (e.g. SysV, AAPCS)
    std::optional<StackFallback> m_defaultStackFallback;
};

struct StructReturnDef
{
    Common::Identifier m_pointerRegister;
    bool m_consumesArgSlot;
    std::optional<Common::Identifier> m_returnRegister; // e.g. RAX echoed return
};

struct ReturnDef
{
    std::optional<StructReturnDef> m_sret;
    std::pmr::vector<PassRule> m_rules;
};

struct CallingConventionDefFile
{
    Common::Identifier m_name;
    StackDef m_stack;

    std::pmr::vector<Common::Identifier> m_calleeSaved;
    std::pmr::vector<Common::Identifier> m_callerSaved;

    ClassificationDef m_classification;
    ArgumentPassingDef m_arguments;
    ReturnDef m_returns;
};

} // namespace DSL::Ast::CallingConvDef

#endif // EZDSLLEXER_CALLING_CONV_DEF_LANG_AST_H