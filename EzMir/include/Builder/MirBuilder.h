#ifndef EZMIR_MIR_BUILDER_H
#define EZMIR_MIR_BUILDER_H

/*
 * Interface used to abstract away common details about builders.
 */
template <typename T> class MirBuilder
{
  public:
    virtual ~MirBuilder(){};

    bool isBuilt() const { return m_builtObj != nullptr; }

    /**
     * Returns the built object.
     */
    T *getBuiltObj() { return m_builtObj; }

    /**
     * Sets the built result object.
     */
    void setBuildResult(T *obj) { m_builtObj = obj; }

  private:
    T *m_builtObj;
};

#endif // EZMIR_MIR_BUILDER_H
