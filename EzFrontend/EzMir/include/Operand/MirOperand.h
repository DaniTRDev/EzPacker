/**
 * @file MirOperand.h
 * @brief Variant type representing any operand an MIR instruction can use.
 *
 * A `MirOperand` is a thin wrapper around `std::variant` used by
 * `MirInstruction` to store instruction arguments in a uniform way. Each
 * instance holds exactly one of the payload structs declared in this header.
 * Accessors such as `getRegister()` and `getMemory()` return a typed pointer
 * to the active payload, or `nullptr` if the operand currently stores a
 * different kind.
 *
 * The class is intentionally lightweight and copyable so emitters can build
 * operands inline and append them into arena-backed instruction operand lists.
 */
#ifndef EZPACKER_MIROPERAND_H
#define EZPACKER_MIROPERAND_H

#include "EzMirCommon.h"

struct MirBigInteger
{
    size_t m_constantId{ 0 }; // ID of a global data entry containing the big integer payload.
};
struct MirDouble
{
    double m_value{ 0.0 }; // Immediate floating-point literal.
};
struct MirInteger
{
    int64_t m_value{ 0 }; // Immediate signed integer literal.
};
struct MirMemory
{
    size_t m_baseRegId{ 0 };  // Base register ID.
    size_t m_indexRegId{ 0 }; // Optional index register ID; `0` means none.
    int8_t m_scale{ 0 };      // Scale applied to the index register.
    int64_t m_offset{ 0 };    // Constant displacement added to the address.
};
struct MirReference
{
    size_t m_refId{ 0 }; // Generic MIR reference ID (block, function, data entry, ...).
};
struct MirRegister
{
    size_t m_id{ 0 };   // Unique virtual-register ID.
    size_t m_size{ 0 }; // Register size in bytes.
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
    /**
     * Underlying variant used to store the active operand payload.
     */
    using VariantType = std::variant<MirBigInteger, MirDouble, MirInteger, MirMemory, MirReference, MirRegister>;

    /**
     * Creates a default operand whose active payload is `MirBigInteger{}`.
     *
     * This follows the default-construction rules of `std::variant`, where the
     * first alternative becomes active.
     */
    explicit MirOperand() = default;

    /**
     * Copies the active payload from another operand.
     */
    MirOperand(const MirOperand &copy);

    /**
     * Moves the active payload from another operand.
     */
    MirOperand(const MirOperand &&other);

    /**
     * Implicit converting constructor. Allows expressions such as
     * `MirOperand op = MirRegister{5, 8};`.
     */
    template <typename T>
        requires(!std::is_same_v<std::remove_cvref_t<T>, MirOperand>)
    MirOperand(T &&val) : m_data(std::forward<T>(val))
    {
    }

    /**
     * Returns the enum tag that matches the currently active variant member.
     */
    MirOperandType getType() const;

    /** Returns the active `MirBigInteger` payload, or `nullptr` if not active. */
    MirBigInteger *getBigInteger();
    /** Returns the active `MirBigInteger` payload, or `nullptr` if not active. */
    const MirBigInteger *getBigInteger() const;

    /** Returns the active `MirDouble` payload, or `nullptr` if not active. */
    MirDouble *getDouble();
    /** Returns the active `MirDouble` payload, or `nullptr` if not active. */
    const MirDouble *getDouble() const;

    /** Returns the active `MirInteger` payload, or `nullptr` if not active. */
    MirInteger *getInteger();
    /** Returns the active `MirInteger` payload, or `nullptr` if not active. */
    const MirInteger *getInteger() const;

    /** Returns the active `MirMemory` payload, or `nullptr` if not active. */
    MirMemory *getMemory();
    /** Returns the active `MirMemory` payload, or `nullptr` if not active. */
    const MirMemory *getMemory() const;

    /** Returns the active `MirReference` payload, or `nullptr` if not active. */
    MirReference *getReference();
    /** Returns the active `MirReference` payload, or `nullptr` if not active. */
    const MirReference *getReference() const;

    /** Returns the active `MirRegister` payload, or `nullptr` if not active. */
    MirRegister *getRegister();
    /** Returns the active `MirRegister` payload, or `nullptr` if not active. */
    const MirRegister *getRegister() const;

    /**
     * Returns direct mutable access to the underlying variant.
     */
    VariantType &getVariant();

    /**
     * Returns direct read-only access to the underlying variant.
     */
    const VariantType &getVariant() const;

  private:
    VariantType m_data;
};

#endif // EZPACKER_MIROPERAND_H
