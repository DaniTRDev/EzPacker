#ifndef EZPACKER_MIREXPANSIONRULEREGISTRY_H
#define EZPACKER_MIREXPANSIONRULEREGISTRY_H

#include "EzTripleCommon.h"

/**
 * This enum represents an abstraction over the operands that are going to be used when expanding an instruction.
 *
 * It is important to note that since the ISA is a 2-operand-destination-left, dest = operand 0 and src = operand 1.
 */
enum class ExpansionOperandType
{
    DestLow,
    DestHigh,
    SrcLow,
    SrcHigh,
    Temporal, // Dynamic index based temporal register
    IntImm,
    FloatImm,
    MemoryHalfOffset
};

/**
 * Struct used to contain information about how a memory operand should be expanded.
 *
 * If displacement != 0, it will be used as is: base+displ.
 * If scale factor != 0, the displacement used will be: base + displ + (origSize / 2^factor)
 */
struct ExpansionMemoryOperand
{
    ExpansionOperandType m_base{ ExpansionOperandType::DestLow };
    int64_t m_displ{ 0 };
    int64_t m_scaleHalfFactor{ 0 };
};

/**
 * Struct used to contain the information about a temporal register operand used for the expansion.
 */
struct ExpansionTemporalOperand
{
    bool m_high{ false };
    size_t m_id{ 0 };
};

/**
 * Structure used to represent a high level version of the resulting expanded instruction that will be interpreted by
 * the expand action.
 */
struct ExpansionOperand
{
    ExpansionOperandType m_type;
    FlexFloat floatVal{ 0.0 };
    FlexInt intVal{ 0 };
    ExpansionMemoryOperand m_memOperand{};
    ExpansionTemporalOperand m_tempOperand{};

    // Fluent Constructors & Helpers
    static ExpansionOperand dstLo() { return { ExpansionOperandType::DestLow }; }
    static ExpansionOperand dstHi() { return { ExpansionOperandType::DestHigh }; }
    static ExpansionOperand srcLo() { return { ExpansionOperandType::SrcLow }; }
    static ExpansionOperand scrHi() { return { ExpansionOperandType::SrcHigh }; }

    /**
     * Creates an expansion operand that acts as a temporal LOW register.
     */
    static ExpansionOperand TempLo(size_t index)
    {
        ExpansionOperand op{ ExpansionOperandType::Temporal };
        op.m_tempOperand = ExpansionTemporalOperand{ .m_high = false, .m_id = index };
        return op;
    }

    /**
     * Creates an expansion operand that acts as a temporal HIGH regisger.
     */
    static ExpansionOperand TempHi(size_t index)
    {
        ExpansionOperand op{ ExpansionOperandType::Temporal };
        op.m_tempOperand = ExpansionTemporalOperand{ .m_high = true, .m_id = index };
        return op;
    }

    /**
     * Creates an expansion operand that acts as an integer.
     */
    static ExpansionOperand Imm(const FlexInt &val)
    {
        ExpansionOperand op{ ExpansionOperandType::IntImm };
        op.intVal = val;
        return op;
    }

    /**
     * Creates an expansion operand that acts as a floating point.
     */
    static ExpansionOperand Imm(const FlexFloat &val)
    {
        ExpansionOperand op{ ExpansionOperandType::FloatImm };
        op.floatVal = val;
        return op;
    }

    /**
     * Creates an expansion operand that acts as an abstract memory operand. This operand receives a base, a
     * displacement and a factor. The final formula for the memory address is: base + displ + (origSize / 2^factor).
     */
    static ExpansionOperand MemHalf(ExpansionOperandType base, int64_t factor, int64_t displ = 0)
    {
        ExpansionOperand op{ ExpansionOperandType::MemoryHalfOffset };
        op.m_memOperand = ExpansionMemoryOperand{ .m_base = base, .m_displ = displ, .m_scaleHalfFactor = factor };
        return op;
    }
};

/**
 * Struct used to contain the abstract representation of an expanded instruction.
 */
struct ExpansionInstruction
{
    MirInstructionOpCode m_opcode;
    std::pmr::string m_rtLibraryCall;
    std::pmr::vector<ExpansionOperand> m_operands;
};

/**
 * Context given to expansion predicates so they have control of whenever an expansion should, or not, occur and how.
 */
struct ExpansionContext
{
    MirType *m_expandedType;
    MirType *m_fullType;
    std::pmr::list<MirInstruction *>::iterator m_it;
};

/**
 * Type used for the predicates of an expansion rule, if the predicate returns true, the following expansion sentences
 * will be executed. This has NOTHING to do with the legalization predicates that dictate when an expansion SHOULD
 * occur.
 *
 * This is a mechanism that can be used to apply optimisations.
 */
using ExpansionPredicate = std::function<bool(ExpansionContext &ctx)>;

/**
 * Structure used to contain the information of an expansion rule.
 */
struct ExpansionRule
{
    ExpansionPredicate m_pred{ nullptr };                  // Nullptr means it is alawys true.
    std::pmr::vector<ExpansionInstruction> m_instructions; // Result of the expansion rule.

    ExpansionRule(std::pmr::memory_resource *alloc) : m_pred(nullptr), m_instructions(alloc) {}
};

class MirExpansionRuleRegistry
{
  public:
    /**
     * Creates the registry with the given context.
     */
    MirExpansionRuleRegistry(MirBuilderContext *ctx);

    /**
     * Traverses the rule map and returns the first rule whose predicate returns true (or is nullptr) for the given
     * instruction.
     */
    ExpansionRule *getRule(ExpansionContext &ctx);

    /**
     * Adds a new expansion rule for the given opcode.
     */
    void addRule(MirInstructionOpCode opcode, ExpansionRule rule);

    /**
     * Returns the allocator of this registry.
     */
    std::pmr::memory_resource *getAllocator();

  private:
    std::pmr::memory_resource *m_allocator;
    std::pmr::unordered_map<MirInstructionOpCode, std::pmr::vector<ExpansionRule *>> m_rules;
};

#endif // EZPACKER_MIREXPANSIONRULEREGISTRY_H