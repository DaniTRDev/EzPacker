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
    DataTypeAnnotation(const std::shared_ptr<Type> &type);

    /**
     * Returns "DataTypeAnnotation".
     * @return const char*
     */
    const char *getAnnotationName() const override;
    
    /**
     * Returns the data type.
     * @return
     */
    const std::shared_ptr<Type> &getDataType() const;

  private:
    std::shared_ptr<Type> m_type;
};

#endif // EZPACKER_TYPEANNOTATION_H
