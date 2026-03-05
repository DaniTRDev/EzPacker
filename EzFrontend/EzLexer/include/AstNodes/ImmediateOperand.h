/**
 * @file ImmediateOperand.h
 * @brief AST nodes for compile-time constant operands (integers, floats, strings).
 *
 * ImmediateOperand is the abstract base; concrete subclasses are:
 *   - IntegerImmediate  — arbitrary-precision integer via libtommath.
 *   - FloatImmediate    — double-precision floating-point value.
 *   - StringImmediate   — UTF-8 string literal (stored in the string pool).
 *
 * An optional data-type prefix (e.g. `i16 0xFF`) is recorded so the
 * semantic pass can resolve the correct type.
 */
#ifndef EZPACKER_IMMEDIATE_H
#define EZPACKER_IMMEDIATE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"
#include <tommath.h>

enum class ImmediateType
{
    Invalid = 0,
    Integer,
    FloatingPoint,
    String
};

/**
 * This class represents an Immediate operand: a type of operand whose exact value is known at compile time.
 */
class ImmediateOperand : public AstNode
{
  public:
    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;

    /**
     * Returns the name of this AstNode.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the name of the immediate type.
     * @return const char*
     */
    virtual const char *getImmediateTypeName() const = 0;

    /**
     * Returns the type of the immediate.
     * @return ImmediateType
     */
    virtual ImmediateType getImmediateType() const = 0;

    /**
     * Sets the data type of the immediate.
     * @param dataType
     */
    void setDataType(const std::string_view &dataType);

    /**
     * Returns the data type this immediate is casted to. See m_dataType.
     * @return const std::string &
     */
    const std::string_view &getDataType();

  private:
    std::string_view m_dataType; // Only set for floats and integers, used to cast values: i16 0xFF.
};

/**
 * Class that defines an Immediate of type integer (since we are using a big-int library, it can be of any fixed length)
 */
class IntegerImmediate : public ImmediateOperand
{
  public:
    /**
     * Creates the object with the given integer
     * @param integer
     */
    explicit IntegerImmediate(mp_int *integer);

    /**
     * Returns true if this integer is signed.
     * @return bool
     */
    bool isSigned() const;

    /**
     * Returns "Integer".
     * @return const char*
     */
    const char *getImmediateTypeName() const override;

    /**
     * Returns ImmediateType::Integer.
     * @return ImmediateType
     */
    ImmediateType getImmediateType() const override;

    /**
     * Returns the contained integer.
     * @return mp_int
     */
    mp_int *getInteger();

    /**
     * Copies current number into destination. If there's no valid number, destination an exception will be thrown.
     */
    void copy(mp_int *destination);

    /**
     * Returns this integer encoded in LittleEndian. If the number is signed, the Two's complement is automatically
     * applied.
     * @return std::string
     */
    std::string getAsBin() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. For integers, their type
     * and value are shown no matter the mode.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    bool m_signed;
    mp_int *m_integer;
};

/**
 * A floating-point immediate. Unlike integers, this operand can't be of any fixed length; it's limited to
 * 64bits (double) (currently, WIP).
 */
class FloatImmediate : public ImmediateOperand
{
  public:
    /**
     * Creates the object with the given 64-bits-floating-point value.
     * @param floatingValue
     */
    explicit FloatImmediate(double floatingValue);

    /**
     * Returns the "FloatingPoint".
     * @return const char*
     */
    const char *getImmediateTypeName() const override;

    /**
     * Returns ImmediateType::FloatingPoint.
     * @return ImmediateType
     */
    ImmediateType getImmediateType() const override;

    /**
     * Returns the floating-point contained in this immediate.
     * @return double
     */
    double getFloatingValue() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. For floating values, their value is
     * shown no matter the mode.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    double m_floatingValue;
};

/**
 * The operand is a string immediate.
 */
class StringImmediate : public ImmediateOperand
{
  public:
    /**
     * Creates the string immediate with the given str.
     * @param str
     */
    explicit StringImmediate(std::string_view str);

    /**
     * Returns the name of the immediate type.
     * @return const char*
     */
    const char *getImmediateTypeName() const override;

    /**
     * Returns the type of the immediate.
     * @return ImmediateType
     */
    ImmediateType getImmediateType() const override;

    /**
     * Returns the string of this immediate.
     * @return const std::string_view &
     */
    const std::string_view &getStr() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. For string values, their value is
     * shown no matter the mode.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    std::string_view m_str;
};

#endif // EZPACKER_IMMEDIATE_H
