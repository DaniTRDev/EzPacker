/**
 * @file TypeCastAnnotation.h
 * @brief Annotation that records how a value must be viewed or converted.
 *
 * `TypeCheckVisitor` attaches this annotation when a node is semantically
 * valid but lowering needs more information than a plain `SymbolAnnotation`
 * or `DataTypeAnnotation` can provide.
 *
 * Two common cases in the current implementation are:
 *   - a variable use explicitly requests a type different from the symbol's
 *     declared type;
 *   - an immediate literal must be emitted using the destination type inferred
 *     from its surrounding instruction or switch statement.
 *
 * For variable uses, the annotation keeps both the original symbol and the
 * destination type. For immediate literals, the original symbol may be null and
 * only the destination type is relevant.
 */
#ifndef EZPACKER_TYPECASTANNOTATION_H
#define EZPACKER_TYPECASTANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/Symbol.h"
#include "Scope/TypeTable.h"
#include "SemanticAnnotations/SymbolAnnotation.h"
#include "SemanticAnnotations/DataTypeAnnotation.h"

/**
 * Annotation that describes the destination type expected during lowering.
 */
class TypeCastAnnotation : public SymbolAnnotation, public DataTypeAnnotation
{
  public:
    /**
     * Creates a cast annotation.
     *
     * `originalSymbol` may be null for nodes such as immediates, where only
     * the destination type matters.
     */
    TypeCastAnnotation(Symbol *originalSymbol, Type *castedDataType);

    /**
     * Returns true when the cast is from `double` to `float`.
     */
    bool isDoubleToFloat() const;

    /**
     * Returns true when the cast is from `double` to an integer type.
     */
    bool isDoubleToInteger() const;

    /**
     * Returns true when the destination type is wider than the original type.
     */
    bool isExpansion() const;

    /**
     * Returns true when the cast is from `float` to `double`.
     */
    bool isFloatToDouble() const;

    /**
     * Returns true when the cast is from `float` to an integer type.
     */
    bool isFloatToInteger() const;

    /**
     * Returns true when the cast is from an integer type to `double`.
     */
    bool isIntegerToDouble() const;

    /**
     * Returns true when the cast is from an integer type to `float`.
     */
    bool isIntegerToFloat() const;

    /**
     * Returns true when both source and destination are integer types.
     */
    bool isIntegerToInteger() const;

    /**
     * Returns true when the destination type is narrower than the original
     * type.
     */
    bool isTruncation() const;

    /**
     * Returns the runtime annotation kind name: `"TypeCastAnnotation"`.
     */
    const char *getAnnotationName() const override;
};

#endif // EZPACKER_TYPECASTANNOTATION_H
