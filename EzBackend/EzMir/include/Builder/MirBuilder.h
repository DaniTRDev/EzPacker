#ifndef EZPACKER_MIRBUILDER_H
#define EZPACKER_MIRBUILDER_H

#include "EzMirCommon.h"

/*
 * Interface used to abstract away common details about builders.
 */
template <typename T> class MirBuilder
{
  public:
    virtual ~MirBuilder() {};

    bool isBuilt() const { return m_builtObj != nullptr; }

    /**
     * Returns the built object.
     * @return
     */
    T *getBuiltObj() { return m_builtObj; }

    /**
     * Sets the built result object.
     * @param obj
     */
    void setBuildResult(T *obj) { m_builtObj = obj; }

  private:
    T *m_builtObj;
};

#endif // EZPACKER_MIRBUILDER_H
