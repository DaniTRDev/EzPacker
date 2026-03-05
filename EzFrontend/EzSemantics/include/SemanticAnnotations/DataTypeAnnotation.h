/**
 * @file DataTypeAnnotation.h
 * @brief Annotation that attaches a resolved Type to an AST node.
 *
 * DataTypeAnnotation is used on nodes whose concrete type has been
 * determined by the SymbolAndTypeResolverVisitor — for example, an
 * ImmediateOperand whose `i16` prefix has been resolved to the
 * corresponding Type object.  This is distinct from SymbolAnnotation,
 * which links a node to its declaring symbol.
 */
#ifndef EZPACKER_DATATYPEANNOTATION_H
#define EZPACKER_DATATYPEANNOTATION_H

#include "EzSemanticsCommon.h"
#include "Scope/TypeTable.h"

/**
 * Class used to tell that an AstNode has a type (DIFFERENT FROM SYMBOL).
 */
class DataTypeAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates the annotation with the given type.
     * @param type
     */
    DataTypeAnnotation(Type *type);

    /**
     * Returns the data type.
     * @return
     */
    Type *getDataType() const;

    /**
     * Returns "DataTypeAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;

  private:
    Type *m_type;
};

#endif // EZPACKER_TYPEANNOTATION_H
