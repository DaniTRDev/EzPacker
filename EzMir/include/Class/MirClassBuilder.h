#ifndef EZPACKER_MIRCLASSBUILDER_H
#define EZPACKER_MIRCLASSBUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"

class MirClassBuilder : public MirBuilder<MirClass>
{
  public:
    /**
     * Creates the class builder with the given context.
     * @param ctx
     */
    MirClassBuilder(MirBuilderContext *ctx);

    /**
     * Builds a class WITH or WITHOUT a parent (derived class). It will set parent fields and methods FIRST, appended
     * fields (through appendField) and methods (through methodBuilder) AFTER.
     * @param parent
     * @param name
     * @param sourceRef
     * @return
     */
    MirClass *build(MirClass *parent, const std::pmr::string &name, SourceReference *sourceRef = nullptr);

    /**
     * Appends a field to the class.
     * @param type
     * @param name
     */
    void appendField(MirType *type, const std::pmr::string &name);

    /**
     * Appends a method to the class vTable. It is responsible for the caller to setup a THIS pointer to the class.
     */
    void appendMethod(MirFunction *method);

    /**
     * Sets the constructor of this class. If called multiple times, the constructor will be overridden.
     * @param constructor
     */
    void setConstructor(MirFunction *constructor);

  private:
    MirBuilderContext *m_ctx;
    MirClassMethod *m_constructor;
    std::pmr::vector<MirClassField *> m_fields;
    std::pmr::vector<MirClassMethod *> m_vTable;
};

#endif // EZPACKER_MIRCLASSBUILDER_H
