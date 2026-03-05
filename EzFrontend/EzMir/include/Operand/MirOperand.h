/**
 * @file MirOperand.h
 * @brief Variant type representing any operand an MIR instruction can use.
 *
 * A MirOperand is a type-safe union (std::variant) that can hold one of:
 *   - MirRegister    — a virtual register (ID + byte-size).
 *   - MirInteger     — a 64-bit signed integer immediate.
 *   - MirBigInteger  — a reference to an arbitrary-precision integer stored
 *                       in the global data section.
 *   - MirDouble      — a 64-bit floating-point immediate.
 *   - MirMemory      — base + index*scale + offset addressing.
 *   - MirReference   — an ID that refers to another MIR entity (block,
 *                       function, global data entry, …).
 *
 * MirOperand supports implicit construction from any of its member types
 * (e.g. `MirOperand op = MirRegister{5, 4};`), copy/move semantics, and
 * typed accessors (getRegister(), getInteger(), …).
 */
#ifndef EZPACKER_MIROPERAND_H
#define EZPACKER_MIROPERAND_H

#include "EzMirCommon.h"

struct MirBigInteger
{
    size_t m_constantId{ 0 };
};
struct MirDouble
{
    double m_value{ 0.0 };
};
struct MirInteger
{
    int64_t m_value{ 0 };
};
struct MirMemory
{
    size_t m_baseRegId{ 0 };
    size_t m_indexRegId{ 0 };
    int8_t m_scale{ 0 };
    int64_t m_offset{ 0 };
};
struct MirReference
{
    size_t m_refId{ 0 };
};
struct MirRegister
{
    size_t m_id{ 0 };
    size_t m_size{ 0 };
};

enum class MirOperandType : size_t
{
    BigInteger = 0,
    Double,
    Integer,
    Memory,
    Reference,
    Register
};

class MirOperand
{
  public:
    // Expose the variant type for advanced template matching
    using VariantType = std::variant<MirBigInteger, MirDouble, MirInteger, MirMemory, MirReference, MirRegister>;

    /**
     * Needed because the constructor below will invalidate the default constructor.
     */
    explicit MirOperand() = default;

    /**
     * Define the copy constructor explicitly.
     * @param copy
     */
    MirOperand(const MirOperand &copy);

    /**
     * Define the move constructor explicitly.
     * @param other
     */
    MirOperand(const MirOperand &&other);

    /**
     * Implicit converting constructor. Allows MirOperand op = MirRegister{5};
     * @tparam T
     * @param val
     */
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, MirOperand>)
    MirOperand(T &&val) : m_data(std::forward<T>(val))
    {
    }

    /**
     * Dynamically resolves the type enum from the variant's internal tag.
     * @return MirOperandType
     */
    MirOperandType getType() const;

    MirBigInteger *getBigInteger();
    const MirBigInteger *getBigInteger() const;

    MirDouble *getDouble();
    const MirDouble *getDouble() const;

    MirInteger *getInteger();
    const MirInteger *getInteger() const;

    MirMemory *getMemory();
    const MirMemory *getMemory() const;

    MirReference *getReference();
    const MirReference *getReference() const;

    MirRegister *getRegister();
    const MirRegister *getRegister() const;

    VariantType &getVariant();
    const VariantType &getVariant() const;

  private:
    VariantType m_data;
};

#endif // EZPACKER_MIROPERAND_H
