#ifndef EZMIR_MIR_OPERAND_H
#define EZMIR_MIR_OPERAND_H

#include "EzMirCommon.h"

/**
 * Enumeration specifying the concrete kind of a MIR operand.
 */
enum class MirOperandType : uint8_t
{
    Invalid = 0,   // Uninitialized / invalid operand kind sentinel
    FloatingPoint, // Immediate floating-point constant value (MirFloat)
    Integer,       // Immediate integer constant value (MirInteger)
    Reference,     // Symbolic reference to a block, function, global variable, or stack slot (MirReference)
    Register,      // Virtual or physical register identifier (MirRegister)
    RuntimeSymbol, // Named runtime library symbol (MirRuntimeSymbol)
    Memory,        // Base-plus-displacement memory addressing mode [base + displacement] (MirMemory)
    MaxOperandType // Operand count sentinel
};

/**
 * Abstract base class for all operands attached to MIR instructions.
 * Encapsulates the associated MirType and optional source location reference.
 */
class MirOperand
{
  public:
    /**
     * Virtual destructor for operand polymorphism.
     */
    virtual ~MirOperand() = default;

    /**
     * Constructs a base operand with an associated MIR type and source code reference.
     */
    explicit MirOperand(class MirType *type, class SourceReference *sourceRef);

    /**
     * Returns the concrete MirOperandType discriminant.
     */
    virtual MirOperandType getType() const = 0;

    /**
     * Returns the MIR type descriptor associated with this operand.
     */
    class MirType *getMirType() const;

    /**
     * Returns the size in bytes of the operand derived from its underlying MirType.
     */
    virtual size_t getSizeInBytes() const;

    /**
     * Returns the source location reference for diagnostics, or nullptr if unavailable.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Updates the MIR type descriptor of this operand.
     */
    void setMirType(class MirType *type);

    /**
     * Formats the operand into a diagnostic and printable string representation.
     */
    virtual std::string toString() const = 0;

    /**
     * Checks if this operand matches the concrete derived operand type OperandType.
     */
    template <typename OperandType> bool isOfType() const { return getType() == OperandType::OpKind; }

    /**
     * Casts this operand to the derived OperandType pointer, or returns nullptr if type mismatch.
     */
    template <typename OperandType> OperandType *get()
    {
        if (isOfType<OperandType>())
        {
            return static_cast<OperandType *>(this);
        }
        return nullptr;
    }

    /**
     * Const-qualified cast to the derived OperandType pointer, or returns nullptr if type mismatch.
     */
    template <typename OperandType> const OperandType *get() const
    {
        if (isOfType<OperandType>())
        {
            return static_cast<const OperandType *>(this);
        }
        return nullptr;
    }

  private:
    /**
     * MIR type associated with this operand value.
     */
    class MirType *m_type{ nullptr };

    /**
     * Source location reference for diagnostic tracing.
     */
    class SourceReference *m_sourceRef{ nullptr };
};

#endif // EZMIR_MIR_OPERAND_H