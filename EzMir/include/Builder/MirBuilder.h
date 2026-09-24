#ifndef EZMIR_MIR_BUILDER_H
#define EZMIR_MIR_BUILDER_H

/**
 * Generic base class for fluent MIR builders producing objects of type T.
 */
template <typename T> class MirBuilder
{
  public:
    virtual ~MirBuilder(){};

    /**
     * Returns true if an object has been successfully built by this builder.
     */
    bool isBuilt() const { return m_builtObj != nullptr; }

    /**
     * Returns a pointer to the built result object, or nullptr if not yet finalized.
     */
    T *getBuiltObj() { return m_builtObj; }

    /**
     * Stores the built result object reference.
     */
    void setBuildResult(T *obj) { m_builtObj = obj; }

  private:
    T *m_builtObj{ nullptr }; // Result produced by the derived builder, or nullptr until finalized.
};

#endif // EZMIR_MIR_BUILDER_H
