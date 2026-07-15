#include "Class/MirClassBuilder.h"

MirClassBuilder::MirClassBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

MirClass *MirClassBuilder::build(const std::pmr::string &name,
                                 const std::pmr::vector<MirClassField> &fields,
                                 const std::pmr::vector<MirFunction *> &methods,
                                 SourceReference *sourceRef)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirClass> alloc;

    std::pmr::vector<MirClassField> finalFields(arena);
    finalFields.insert(finalFields.begin(), fields.begin(), fields.end());
    finalFields.insert(finalFields.begin(), m_fields.begin(), m_fields.end());

    std::pmr::vector<MirFunction *> finalVTable(arena);
    finalVTable.insert(finalVTable.begin(), m_vTable.begin(), m_vTable.end());
    finalVTable.insert(finalVTable.begin(), methods.begin(), methods.end());

    std::pmr::vector<MirType *> fieldTypes(arena);
    for (auto &field : finalFields)
    {
        fieldTypes.push_back(field.m_type);
    }

    MirType *type = m_ctx->getTypeTable()->getClass(fieldTypes, name);

    MirClass *_class = alloc.new_object<MirClass>(nullptr,
                                                  m_ctx->createId(),
                                                  type,
                                                  name,
                                                  std::move(finalFields),
                                                  std::move(finalVTable),
                                                  sourceRef);

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirClassBuilder");
    diagBuilder << std::pmr::string(std::format("Built derived class with id: {}", _class->getId()));
    diagBuilder.appendNote(std::pmr::string(MirPrinter::printToString(_class, MirPrinterDetail::Detailed)), sourceRef);

    return _class;
}

MirClass *MirClassBuilder::buildDerived(MirClass *parent,
                                        const std::pmr::string &name,
                                        const std::pmr::vector<MirClassField> &fields,
                                        const std::pmr::vector<MirFunction *> &methods,
                                        SourceReference *sourceRef)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirClass> alloc;

    std::pmr::vector<MirClassField> finalFields(arena);
    finalFields.insert(finalFields.begin(), parent->getFields().begin(), parent->getFields().end());
    finalFields.insert(finalFields.begin(), fields.begin(), fields.end());
    finalFields.insert(finalFields.begin(), m_fields.begin(), m_fields.end());

    std::pmr::vector<MirFunction *> finalVTable(arena);
    finalVTable.insert(finalVTable.begin(), parent->getVTable().begin(), parent->getVTable().end());
    finalVTable.insert(finalVTable.begin(), m_vTable.begin(), m_vTable.end());
    finalVTable.insert(finalVTable.begin(), methods.begin(), methods.end());

    std::pmr::vector<MirType *> fieldTypes(arena);
    for (auto &field : finalFields)
    {
        fieldTypes.push_back(field.m_type);
    }

    MirType *type = m_ctx->getTypeTable()->getClass(fieldTypes, name);

    MirClass *_class = alloc.new_object<MirClass>(parent,
                                                  m_ctx->createId(),
                                                  type,
                                                  name,
                                                  std::move(finalFields),
                                                  std::move(finalVTable),
                                                  sourceRef);

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Trace, "MirClassBuilder");
    diagBuilder << sourceRef << std::pmr::string(std::format("Built class with id: {}", _class->getId()));
    diagBuilder.appendNote(std::format("Parent: {}", parent->getName()).c_str(), parent->getSourceRef());
    diagBuilder.appendNote(std::pmr::string(MirPrinter::printToString(_class, MirPrinterDetail::Detailed)), sourceRef);

    return _class;
}

MirFunctionBuilder MirClassBuilder::methodBuilder()
{
    MirClass *obj = getBuiltObj();

    if (!obj)
    {
        m_ctx->getDiagCollector()->builder(Diag_Error, "MirClassBuilder")
                << "Can't create method builder from non-built class";
        return MirFunctionBuilder(nullptr);
    }

    MirFunctionBuilder builder(m_ctx, &m_vTable);
    builder.buildParam(obj->getType(), "this");

    return builder;
}

void MirClassBuilder::appendField(MirType *type, const std::pmr::string &name)
{
    m_fields.push_back(MirClassField{ .m_type = type, .m_offset = -1, .m_name = name });
}