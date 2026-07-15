#ifndef EZPACKER_MIRCLASSBUILDER_H
#define EZPACKER_MIRCLASSBUILDER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilder.h"
#include "Builder/MirBuilderContext.h"
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
     * Builds a class WITHOUT a parent (non-derived class). It will set appended fields (through appendField) and
     * methods (through methodBuilder) FIRST, fields and methods given in parameters will be appended AFTER.
     * @param name
     * @param fields
     * @param methods
     * @param sourceRef
     * @return
     */
    MirClass *build(const std::pmr::string &name,
                    const std::pmr::vector<MirClassField> &fields,
                    const std::pmr::vector<MirFunction *> &methods,
                    SourceReference *sourceRef = nullptr);

    /**
     * Builds a class WITH a parent (derived class). It will set parent fields and methods FIRST, appended fields
     * (through appendField) and methods (through methodBuilder) AFTER, fields and methods given in parameters will be
     * appended AFTER.
     * @param parent
     * @param name
     * @param fields
     * @param methods
     * @param sourceRef
     * @return
     */
    MirClass *buildDerived(MirClass *parent,
                           const std::pmr::string &name,
                           const std::pmr::vector<MirClassField> &fields,
                           const std::pmr::vector<MirFunction *> &methods,
                           SourceReference *sourceRef = nullptr);

    /**
     * Returns a function builder that is attached to this class. This means:
     *  - Resulting function will be pushed into this class's vTable.
     *  - First argument will be set as a this pointer.
     *
     *  To call this method, the class MUST have been built. If it wasn't, an error is set to the diag collector and
     *  an INVALID function builder is returned.
     * @return
     */
    MirFunctionBuilder methodBuilder();

    /**
     * Appends a field to the class.
     * @param type
     * @param name
     */
    void appendField(MirType *type, const std::pmr::string &name);

  private:
    MirBuilderContext *m_ctx;
    std::pmr::vector<MirClassField> m_fields;
    std::pmr::list<MirFunction *> m_vTable;
};

#endif // EZPACKER_MIRCLASSBUILDER_H
