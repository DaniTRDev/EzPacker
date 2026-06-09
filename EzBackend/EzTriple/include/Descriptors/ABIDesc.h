#ifndef EZPACKER_ABIDESC_H
#define EZPACKER_ABIDESC_H

#include "EzTripleCommon.h"

/**
 * Class used to define target-dependant restrictions enforced by the OS or the system.
 */
class ABIDesc
{
  public:
    virtual ~ABIDesc() = default;

    /**
     * Returns the name of the ABI description.
     * @return
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the type size that is legal in this ABI.
     * @param type
     * @return
     */
    virtual size_t getTypeSizeInBytes(MirType *type) const = 0;

    /**
     * Returns the alignment of the given type in this ABI.
     * @param type
     * @return
     */
    virtual size_t getTypeAlignment(MirType *type) const = 0;

    /**
     * Returns the stack alignment needed BEFORE a call.
     * @return
     */
    virtual size_t getStackAlignment() const = 0;

    /**
     * Returns the shadown space needed BEFORE a call.
     * @return
     */
    virtual size_t getShadowSpaceSize() const = 0;
};

#endif // EZPACKER_ABIDESC_H
