#ifndef EZMIR_MIR_CLASS_BUILDER_H
#define EZMIR_MIR_CLASS_BUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"

class MirClassBuilder : public MirBuilder<class MirClass>
{
  public:
    /**
     * Creates the class builder with the given context.
     * @param ctx
     */
    MirClassBuilder(class MirBuilderContext *ctx);

    /**
     * Builds a class WITH or WITHOUT a parent (derived class). It will set parent fields and methods FIRST, appended
     * fields (through appendField) and methods (through methodBuilder) AFTER.
     */
    MirClass *build(class MirClass *parent, const std::pmr::string &name, class SourceReference *sourceRef = nullptr);

    /**
     * Appends a field to the class.
     */
    void appendField(class MirType *type, const std::pmr::string &name);

    /**
     * Appends a method to the class vTable. It is responsible for the caller to setup a THIS pointer to the class.
     */
    void appendMethod(class MirFunction *method);

    /**
     * Sets the constructor of this class. If called multiple times, the constructor will be overridden.
     */
    void setConstructor(class MirFunction *constructor);

  private:
    MirBuilderContext *m_ctx;
    class MirClassMethod *m_constructor;
    std::pmr::vector<class MirClassField *> m_fields;
    std::pmr::vector<class MirClassMethod *> m_vTable;
};

#endif // EZMIR_MIR_CLASS_BUILDER_H
