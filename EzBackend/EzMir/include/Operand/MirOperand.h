/**
 * @file MirOperand.h
 * @brief Variant type representing any operand an MIR instruction can use.
 *
 * A `MirOperand` is a polymorphic base class used by `MirInstruction` to store
 * instruction arguments in a uniform way. Each instance holds exactly one of
 * the payload structs declared in `MirOperands.h`.
 *
 * Accessors such as `get<MirRegister>()` return a typed pointer to the active
 * payload, or `nullptr` if the operand currently stores a different kind,
 * utilizing a fast RTTI static_cast system.
 */
#ifndef EZPACKER_MIROPERAND_H
#define EZPACKER_MIROPERAND_H

#include "EzMirCommon.h"
#include "Type/MirType.h"

enum class MirOperandType : uint8_t
{
    Invalid = 0,
    ConstantPoolRef,
    Double,
    Integer,
    Reference,
    Register,
    FrameIndex,
    Memory,
    MaxOperandType
};

inline std::map<MirOperandType, std::string> g_MirOperandType2Str = {
    { MirOperandType::Invalid, "Invalid" },       { MirOperandType::ConstantPoolRef, "ConstantPoolRef" },
    { MirOperandType::Double, "Double" },         { MirOperandType::Integer, "Integer" },
    { MirOperandType::Reference, "Reference" },   { MirOperandType::Register, "Register" },
    { MirOperandType::FrameIndex, "FrameIndex" }, { MirOperandType::Memory, "Memory" }
};

class MirOperand
{
  public:
    // Force all derived operands to initialize the type
    explicit MirOperand(MirType *type) : m_type(type) {}

    /**
     * Returns the type of this operand.
     */
    virtual MirOperandType getType() const = 0;

    /**
     * Returns the MIR type associated with this operand.
     */
    MirType *getMirType() const { return m_type; }

    /**
     * Centralized implementation: gets the size from the attached MirType.
     * @return The size in BYTES of the operand.
     */
    virtual size_t getSizeInBytes() const { return m_type ? m_type->getTotalSizeInBytes() : 0; }

    /**
     * Returns a string representation of the operand.
     * @return std::string
     */
    virtual std::string toString() const = 0;

    template <typename OperandType> bool isOfType() const { return getType() == OperandType::OpKind; }

    template <typename OperandType> OperandType *get()
    {
        if (isOfType<OperandType>())
        {
            return static_cast<OperandType *>(this);
        }
        return nullptr;
    }

    template <typename OperandType> const OperandType *get() const
    {
        if (isOfType<OperandType>())
        {
            return static_cast<const OperandType *>(this);
        }
        return nullptr;
    }

  private:
    MirType *m_type{ nullptr }; // Centralized type tracking
};

#endif // EZPACKER_MIROPERAND_H