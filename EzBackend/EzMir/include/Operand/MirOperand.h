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
    Double,     // Immediate double.
    Integer,    // Immediate integer.
    Reference,  // A reference to a block, a function or data.
    Register,   // A physical or virtual register.
    FrameIndex, // Used to reference parameters and objects that are saved in a stack frame.
    Memory,     // Used to dereference addresses.
    MaxOperandType
};

inline std::map<MirOperandType, std::string> g_MirOperandType2Str = {
    { MirOperandType::Invalid, "Invalid" },   { MirOperandType::Double, "Double" },
    { MirOperandType::Integer, "Integer" },   { MirOperandType::Reference, "Reference" },
    { MirOperandType::Register, "Register" }, { MirOperandType::FrameIndex, "FrameIndex" },
    { MirOperandType::Memory, "Memory" },     { MirOperandType::MaxOperandType, "MaxOperandType" }
};

class MirOperand
{
  public:
    virtual ~MirOperand() = default;

    /**
     * Creates the operand with the given type.
     * @param type
     * @param sourceRef
     */
    explicit MirOperand(MirType *type, SourceReference *sourceRef);

    /**
     * Returns the type of this operand.
     */
    virtual MirOperandType getType() const = 0;

    /**
     * Returns the MIR type associated with this operand.
     */
    MirType *getMirType() const;

    /**
     * Centralized implementation: gets the size from the attached MirType.
     * @return The size in BYTES of the operand.
     */
    virtual size_t getSizeInBytes() const;

    /**
     * Returns the source reference of the operand. It MAY BE nullptr if the operand does not have a source reference.
     * @return
     */
    SourceReference *getSourceRef() const;

    /**
     * Returns a string representation of the operand.
     * @return std::string
     */
    virtual std::string toString() const = 0;

    /**
     * Returns true if the operand is of the given type.
     * @tparam OperandType
     * @return
     */
    template <typename OperandType> bool isOfType() const { return getType() == OperandType::OpKind; }

    /**
     * Returns a casted pointer to the operand if is of given time.
     * @tparam OperandType
     * @return
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
     * Returns a const-casted pointer to the operand if is of given time.
     * @tparam OperandType
     * @return
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
    MirType *m_type{ nullptr }; // Centralized type tracking
    SourceReference *m_sourceRef{ nullptr };
};

#endif // EZPACKER_MIROPERAND_H