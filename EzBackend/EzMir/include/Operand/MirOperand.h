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

enum class MirReferenceType : uint8_t
{
    Invalid = 0,
    Block,
    DataEntry,
    Function
};

struct MirBigInteger
{
    size_t m_constantId{ 0 }; // ID of a global data entry containing the big integer payload.
    size_t m_size{ 0 };
};
struct MirDouble
{
    double m_value{ 0.0 }; // Immediate floating-point literal.
};
struct MirInteger
{
    int64_t m_value{ 0 }; // Immediate signed integer literal.
    size_t m_size{ 0 };
};
struct MirReference
{
    /**
     * Returns `true` if this reference points to a block.
     * @return
     */
    bool isBlock() const;

    /**
     * Returns `true` if this reference points to a global data entry.
     * @return
     */
    bool isDataEntry() const;

    /**
     * Returns `true` if this reference points to a function.
     * @return
     */
    bool isFunction() const;

    /**
     * Returns `true` if this reference is invalid (i.e. has type `MirReferenceType::Invalid`).
     * @return
     */
    bool isInvalid() const;

    MirReferenceType m_type;
    size_t m_refId{ 0 }; // Generic MIR reference ID (block, function, data entry, ...).
};
struct MirRegister
{
    bool m_virtual{ true }; // Whether this is a virtual register (true) or a physical register (false).
    size_t m_id{ 0 };       // Unique register ID.
    size_t m_size{ 0 };     // Register size in bytes.

    /**
     * Returns `true` if this register is a virtual register.
     * @param other
     * @return
     */
    bool operator==(const MirRegister &other) const;
};

enum class MirOperandType : size_t
{
    BigInteger = 0,
    Double,
    Integer,
    Reference,
    Register
};

inline std::map<MirOperandType, std::string> g_MirOperandType2Str = {
    { MirOperandType::BigInteger, "BigInteger" }, { MirOperandType::Double, "Double" },
    { MirOperandType::Integer, "Integer" },       { MirOperandType::Reference, "Reference" },
    { MirOperandType::Register, "Register" },
};

class MirOperand
{
  public:
    /**
     * Underlying variant used to store the active operand payload.
     */
    using VariantType = std::variant<MirBigInteger, MirDouble, MirInteger, MirReference, MirRegister>;

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

    /** Returns the active `MirReference` payload, or `nullptr` if not active. */
    MirReference *getReference();
    /** Returns the active `MirReference` payload, or `nullptr` if not active. */
    const MirReference *getReference() const;

    /** Returns the active `MirRegister` payload, or `nullptr` if not active. */
    MirRegister *getRegister();
    /** Returns the active `MirRegister` payload, or `nullptr` if not active. */
    const MirRegister *getRegister() const;

    /**
     * @return The size in BYTES of the operand.
     */
    size_t getSizeInBytes() const;

    /**
     * Returns direct mutable access to the underlying variant.
     */
    VariantType &getVariant();

    /**
     * Returns direct read-only access to the underlying variant.
     */
    const VariantType &getVariant() const;

    /**
     * This should only be used by internal functions. This swaps m_data with another value. The caller must ensure
     * that every reference to m_data is already freed, if it isn't the case, they won't be updated and will cause UB.
     */
    void swapData(VariantType other);

    /**
     * Returns a string representation of the operand.
     * @return std::string
     */
    std::string toString() const;

  private:
    VariantType m_data;
};

#endif // EZPACKER_MIROPERAND_H
