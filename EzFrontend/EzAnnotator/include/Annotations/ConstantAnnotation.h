#ifndef EZPACKER_CONSTANTANNOTATION_H
#define EZPACKER_CONSTANTANNOTATION_H

#include "EzAnnotatorCommon.h"
#include "TypeAbleAnnotation.h"

/**
 * A constant annotation represents what its name says: a constant value that is known at time of compilation. This
 * annotation can only hold one of the 3 types:
 *  - Integer
 *  - String
 *  - Floating points value. Reduced, at the time only floats are supported. Will be updated in a future with a
 *   custom type for big floating-point values.
 */
class ConstantAnnotation : public TypeAbleAnnotation
{
  public:
    /**
     * Creates the annotation with the given integer.
     * @param integer
     */
    explicit ConstantAnnotation(size_t typeId, const std::shared_ptr<mp_int> &integer);

    /**
     * Creates the annotation with the given string.
     * @param str
     */
    explicit ConstantAnnotation(size_t typeId, const std::string &str);

    /**
     * Creates the annotation with the given floating-point value.
     * @param float
     */
    explicit ConstantAnnotation(size_t typeId, float _float);

    /**
     * Destroys the object and frees resources.
     */
    ~ConstantAnnotation() override;

    /**
     * Returns true if this constant is a float.
     * @return bool
     */
    bool isFloat() const;

    /**
     * Returns true if this constant is an integer.
     * @return bool
     */
    bool isInteger() const;

    /**
     * Returns true if this constant is a string.
     * @return bool
     */
    bool isString() const;

    /**
     * Returns "Constant".
     * @return const char *
     */
    const char *getAnnotationName() override;

    /**
     * Returns the float value of the annotation.
     * @return float
     */
    float getFloat() const;

    /**
     * Returns the integer of the annotation.
     * @return const std::shared_ptr<mp_int> &
     */
    const std::shared_ptr<mp_int> &getInt() const;

    /**
     * Returns the string of the annotation.
     * @return const std::string &
     */
    const std::string &getString() const;

  private:
    bool m_isFloat;
    bool m_isInteger;
    bool m_isString;
    float m_float;
    std::string m_string;
    std::shared_ptr<mp_int> m_integer;
};

#endif // EZPACKER_CONSTANTANNOTATION_H
