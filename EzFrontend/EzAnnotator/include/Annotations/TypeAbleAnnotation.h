#ifndef EZPACKER_TYPEABLEANNOTATION_H
#define EZPACKER_TYPEABLEANNOTATION_H

#include "EzAnnotatorCommon.h"

/**
 * Another utility class that can be used in inheritance so the upper class can have a type. This class has been created
 * for the same reason as SymbolAbleAnnotation.
 */
class TypeAbleAnnotation : public virtual IAstNodeAnnotation
{
  public:
    /**
     * Returns the type id.
     * @return size_t
     */
    size_t getTypeId() const;

    /**
     * Sets the type id.
     * @param typeId
     */
    void setTypeId(size_t typeId);

  private:
    size_t m_typeId;
};

#endif // EZPACKER_TYPEABLEANNOTATION_H
