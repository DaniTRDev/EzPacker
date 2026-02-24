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
    TypeCastAnnotation(const std::shared_ptr<Symbol> &originalSymbol, const std::shared_ptr<Type> &castedDataType);

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
     * @return SymbolDataType
     */
    const std::shared_ptr<Type> &getCastedDataType() const;
    
  private:
    std::shared_ptr<Type> m_castedDataType;
};

#endif // EZPACKER_TYPECASTANNOTATION_H
