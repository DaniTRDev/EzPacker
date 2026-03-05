/**
 * @file TypeCastAnnotation.h
 * @brief Annotation for variables used with a type different from their declared type.
 *
 * When a variable declared as `i64` is used in a context that expects `i32`
 * (e.g. `add i32 %myVar, 2;`), the TypeCheckVisitor replaces the plain
 * SymbolAnnotation with a TypeCastAnnotation that records both the original
 * symbol and the target (casted) Type.  The lowerer later reads this to
 * emit the correct cast instruction (TRUNC, ZEXT, SEXT, BITCAST, …).
 *
 * Convenience query methods (isExpansion, isTruncation, isIntegerToDouble,
 * etc.) help downstream code decide which cast opcode to use.
 */
#ifndef EZPACKER_TYPECASTANNOTATION_H
#define EZPACKER_TYPECASTANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/Symbol.h"
#include "Scope/TypeTable.h"
#include "SemanticAnnotations/SymbolAnnotation.h"

/**
 * A type cast annotation is used in places where variables declared with a certain type are used with other types:
 *
 * create i64 %myVar;
 * add i32 %myVar, 2; <- The resulting variable node in this instruction contains a TypeCastAnnotation with the
 * original symbol and the casted type.
 */
class TypeCastAnnotation : public SymbolAnnotation
{
  public:
    /**
     * @brief Construct a new Type Cast Annotation object
     * @param originalSymbol
     * @param castedDataType
     */
    TypeCastAnnotation(Symbol *originalSymbol, Type *castedDataType);

    /**
     * Returns true if the cast is from double to float.
     * @return bool.
     */
    bool isDoubleToFloat() const;

    /**
     * Returns true if the cast is from double to integer.
     * @return bool.
     */
    bool isDoubleToInteger() const;

    /**
     * Returns true if cast is performed to get a bigger value than the original.
     * @return bool
     */
    bool isExpansion() const;

    /**
     * Returns true if the cast is from float to double.
     * @return bool.
     */
    bool isFloatToDouble() const;

    /**
     * Returns true if the cast is from float to integer.
     * @return bool.
     */
    bool isFloatToInteger() const;

    /**
     * Returns true if cast is from integer to double.
     * @return bool
     */
    bool isIntegerToDouble() const;

    /**
     * Returns true if the cast is from integer to float.
     * @return bool.
     */
    bool isIntegerToFloat() const;

    /**
     * Returns true if the cast is from integer to integer.
     * @return bool.
     */
    bool isIntegerToInteger() const;

    /**
     * Returns true if cast is performed to get a smaller value than the original.
     * @return bool
     */
    bool isTruncation() const;

    /**
     * Returns "TypeCastAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;
    /**
     * Returns the casted type of this symbol.
     * @return Type
     */
    Type *getCastedDataType() const;

  private:
    Type *m_castedDataType;
};

#endif // EZPACKER_TYPECASTANNOTATION_H
