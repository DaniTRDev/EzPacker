#ifndef EZPACKER_HIGHLEVELMIRINSTRUCTIONOPERAND_H
#define EZPACKER_HIGHLEVELMIRINSTRUCTIONOPERAND_H

#include "EzSemanticsCommon.h"
#include "Scope/TypeTable.h"

/**
 * This essentially is a pseudo copy of the MemoryOperandAstNode class. It's actually violating DRY
 * (Don't-Repeat-Yourself) principles but it's needed in order to decouple MIR from AST so changes can be made
 * to the AST without really needing to change MIR and vice versa.
 */

enum class HighLevelMirOperandType
{
    Immediate, // 0xFF, 42
    Memory,    // (%rax + 0x10)
    None,      // For unused operands
    Reference, // Is this operand a reference to a label or a module?
    Register   // %eax, %r1, virtual_reg_5
};

static std::map<HighLevelMirOperandType, std::string_view> g_HighLevelMirOperandType2Str = {
    { HighLevelMirOperandType::Immediate, "Immediate" },
    { HighLevelMirOperandType::Memory, "Memory" },
    { HighLevelMirOperandType::None, "None" },
    { HighLevelMirOperandType::Reference, "Reference" },
    { HighLevelMirOperandType::Register, "Register" }
};

enum class HighLevelMirImmediateOperandType
{
    Float,      // Value is stored as-is
    BigInteger, // The value stored is an index to an entry in a constant pool.
    Integer     // The value is stored as-is.
};

struct HighLevelMirImmediateOperand
{
    HighLevelMirImmediateOperandType m_type;
    double _double;
    size_t _constantPoolId;
    uint64_t m_integer;
    std::shared_ptr<mp_int> bigInteger; // TODO: Update when the new TypePool class is created.
};

struct HighLevelMirMemoryOperand
{
    size_t m_baseRegId;
    size_t m_indexRegId;
    int8_t m_scale;
    int64_t m_offset;
};

struct HighLevelReferenceOperand
{
    size_t m_referencedItemId;
};

struct HighLevelRegisterOperand
{
    size_t m_registerId; // ID of the register.
};

class HighLevelMirInstructionOperand
{
  public:
    /**
     * Creates a BigInteger immediate.
     * @param data
     * @param bitSize
     * @return HighLevelMirInstructionOperand
     */
    static HighLevelMirInstructionOperand createBigIntegerImm(std::shared_ptr<mp_int> data, size_t bitSize);

    /**
     * Creates a FloatImmediate operand out of the given data.
     * @param immediateType
     * @param data
     * @param bitSize
     * @return HighLevelMirInstructionOperand
     */
    static HighLevelMirInstructionOperand createFloatImm(double data);

    /**
     * Creates an IntegerImmediate.
     * @param data
     * @return HighLevelMirInstructionOperand
     */
    static HighLevelMirInstructionOperand createIntegerImm(uint64_t data);

    /**
     * Creates a memory operand out of the given data.
     * @param bitSize
     * @param baseRegId
     * @param indexRegId
     * @param scale
     * @param offset
     * @return HighLevelMirInstructionOperand
     */
    static HighLevelMirInstructionOperand
    createMem(uint16_t bitSize, size_t baseRegId, size_t indexRegId, int8_t scale, int64_t offset);

    /**
     * Creates a reference operand with the given data.
     * @param referencedItemId
     * @return HighLevelMirInstructionOperand
     */
    static HighLevelMirInstructionOperand createRef(size_t referencedItemId);

    /**
     * Creates a register operand with the given data.
     * @param registerId
     * @param bitSize
     * @return HighLevelMirInstructionOperand
     */
    static HighLevelMirInstructionOperand createReg(size_t registerId, uint16_t bitSize);

    /**
     * Returns the immediate stored in this operand. If the operand is not an immediate, nullptr is returned.
     * @return HighLevelMirImmediateOperand *
     */
    const HighLevelMirImmediateOperand *getImm() const;

    /**
     * Returns the memory operand stored in this operand. If the operand is not a memory operand, nullptr is returned.
     * @return HighLevelMirMemoryOperand *
     */
    const HighLevelMirMemoryOperand *getMem() const;

    /**
     * Returns the reference stored in this operand. If the operand is not a reference, nullptr is returned.
     * @return HighLevelReferenceOperand *
     */
    const HighLevelReferenceOperand *getRef() const;

    /**
     * Returns the register stored in this operand. If the operand is not a register, nullptr is returned.
     * @return HighLevelRegisterOperand *
     */
    const HighLevelRegisterOperand *getReg() const;

    /**
     * Returns the type of the operand.
     * @return HighLevelMirOperandType
     */
    HighLevelMirOperandType getType() const;

    /**
     * Returns the bit size of the element. Returns 0 if its not known at the current time.
     * @return size_t
     */
    size_t getBitSize() const;

    /**
     * Adds a source reference to this instruction operand.
     * @param ref
     */
    void addSourceRef(const std::shared_ptr<SourceReference> &ref);

    /**
     * Sets the source references of this instruction operand. Will OVERWRITE any existing reference.
     * @param refs
     */
    void setSourceRefs(const std::vector<std::shared_ptr<SourceReference>> &refs);

    /**
     * Returns the source reference of this instruction operand.
     * @return const std::vector<std::shared_ptr<SourceReference>> &
     */
    const std::vector<std::shared_ptr<SourceReference>> &getSourceRefs() const;

  private:
    HighLevelMirOperandType m_type;
    uint16_t m_bitsize;
    std::variant<std::monostate, // Represents "None"
                 HighLevelMirImmediateOperand,
                 HighLevelMirMemoryOperand,
                 HighLevelReferenceOperand,
                 HighLevelRegisterOperand>
            m_data;
    std::vector<std::shared_ptr<SourceReference>> m_sourceReferences;
};

#endif // EZPACKER_HIGHLEVELMIRINSTRUCTIONOPERAND_H
