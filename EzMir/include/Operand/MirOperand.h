#ifndef EZMIR_MIR_OPERAND_H
#define EZMIR_MIR_OPERAND_H

#include "EzMirCommon.h"

enum class MirOperandType : uint8_t
{
    Invalid = 0,
    FloatingPoint, // Immediate floating point value.
    Integer,       // Immediate integer.
    Reference,     // A reference to a block, a function or data.
    Register,      // A physical or virtual register.
    RuntimeSymbol, // A symbol that's defined in the runtime library.
    Memory,        // A memory address of the form: base+displacement.
    MaxOperandType
};

inline std::unordered_map<MirOperandType, std::string> g_MirOperandType2Str = {
    { MirOperandType::Invalid, "Invalid" },   { MirOperandType::FloatingPoint, "FloatingPoint" },
    { MirOperandType::Integer, "Integer" },   { MirOperandType::Reference, "Reference" },
    { MirOperandType::Register, "Register" }, { MirOperandType::RuntimeSymbol, "RuntimeSymbol" },
    { MirOperandType::Memory, "Memory" },     { MirOperandType::MaxOperandType, "MaxOperandType" }
};

class MirOperand
{
  public:
    virtual ~MirOperand() = default;

    /**
     * Creates the operand with the given type.
     */
    explicit MirOperand(class MirType *type, class SourceReference *sourceRef);

    /**
     * Returns the type of this operand.
     */
    virtual MirOperandType getType() const = 0;

    /**
     * Returns the MIR type associated with this operand.
     */
    class MirType *getMirType() const;

    /**
     * Gets the size of the operand using its inner MirType.
     */
    virtual size_t getSizeInBytes() const;

    /**
     * Returns the source reference of the operand. It MAY BE nullptr if the operand does not have a source reference.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Sets the MirType of the operand.
     */
    void setMirType(class MirType *type);

    /**
     * Returns a string representation of the operand.
     */
    virtual std::string toString() const = 0;

    /**
     * Returns true if the operand is of the given type.
     */
    template <typename OperandType> bool isOfType() const { return getType() == OperandType::OpKind; }

    /**
     * Returns a casted pointer to the operand if it is of the given type.
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
     * Returns a const-casted pointer to the operand if it is of the given type.
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
    class MirType *m_type{ nullptr };
    class SourceReference *m_sourceRef{ nullptr };
};

#endif // EZMIR_MIR_OPERAND_H