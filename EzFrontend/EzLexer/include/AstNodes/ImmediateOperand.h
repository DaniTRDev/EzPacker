#ifndef EZPACKER_IMMEDIATE_H
#define EZPACKER_IMMEDIATE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
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
};

/**
 * Class that defines an Immediate of type integer (since we are using a big-int library, it can be of any fixed length)
 */
class IntegerImmediate : public ImmediateOperand
{
  public:
    /**
     * Creates the object with the given intenger
     */
    explicit IntegerImmediate(mp_int integer);

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
     * @return const mp_int *
     */
    const mp_int *getInteger() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. For integers, their type
     * and value are shown no matter the mode.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    mp_int m_integer;
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
    explicit StringImmediate(std::string str);

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
     * @return
     */
    const std::string &getStr() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. For string values, their value is
     * shown no matter the mode.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    std::string m_str;
};

#endif // EZPACKER_IMMEDIATE_H
