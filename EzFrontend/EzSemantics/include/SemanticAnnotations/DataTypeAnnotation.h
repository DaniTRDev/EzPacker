/**
 * @file DataTypeAnnotation.h
 * @brief Annotation that stores a resolved semantic `Type` on an AST node.
 *
 * This annotation is used when a node has an explicit or inferred data type
 * that is not merely "look it up from the referenced symbol". Typical examples
 * are immediates and memory operands after `SymbolAndTypeResolverVisitor`
 * resolves their type names.
 *
 * The annotation is later consumed by `TypeCheckVisitor` and lowering code.
 */
#ifndef EZPACKER_DATATYPEANNOTATION_H
#define EZPACKER_DATATYPEANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/TypeTable.h"

/**
 * Annotation that stores a resolved semantic data type for a node.
 */
class DataTypeAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates the annotation with the resolved semantic type.
     */
    DataTypeAnnotation(Type *type);

    /**
     * Returns the semantic type attached to the node.
     */
    Type *getDataType() const;

    /**
     * Returns the runtime annotation kind name: `"DataTypeAnnotation"`.
     */
    const char *getAnnotationName() const override;

  private:
    Type *m_type;
};

#endif // EZPACKER_TYPEANNOTATION_H
