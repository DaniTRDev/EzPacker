#ifndef EZPACKER_MIRBUILDER_H
#define EZPACKER_MIRBUILDER_H

#include "EzCoreCommon.h"

/*
 * Interface used to abstract away common details about builders.
 */
template <typename T> class MirBuilder
{
  public:
    virtual ~MirBuilder() { flush(); };

    bool isBuilt() const { return m_builtObj != nullptr; }

    /**
     * Returns the built object.
     * @return
     */
    T *getBuiltObj() { return m_builtObj; }

    /**
     * Flushes the builder and sets the built obj to nullptr.
     */
    virtual void flush() { m_builtObj = nullptr; }

    void setBuildResult(T *obj) { m_builtObj = obj; }

  private:
    T *m_builtObj;
};

#endif // EZPACKER_MIRBUILDER_H
