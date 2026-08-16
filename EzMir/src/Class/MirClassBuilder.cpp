#include "Class/MirClassBuilder.h"

MirClassBuilder::MirClassBuilder(MirBuilderContext *ctx) : m_ctx(ctx), m_constructor(nullptr) {}

MirClass *MirClassBuilder::build(MirClass *parent, const std::pmr::string &name, SourceReference *sourceRef)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirClass> classAlloc(arena);
    std::pmr::polymorphic_allocator<MirClassField> fieldAlloc(arena);
    std::pmr::polymorphic_allocator<MirClassMethod> methodAlloc(arena);

    // Track methods and fields declared explicitly on this builder session
    std::pmr::vector<MirClassMethod *> newMethods = std::move(m_vTable);
    std::pmr::vector<MirClassField *> newFields = std::move(m_fields);

    m_vTable.clear();
    m_fields.clear(); // Clear so we can build the correct sequential layout

    if (parent)
    {
        // DEEP COPY Inherited Fields FIRST
        auto startingParentFieldIt = parent->getFields().begin();
        if (!parent->getVTable().empty())
        {
            // Advance past parent's vTable array pointer field
            std::advance(startingParentFieldIt, 1);
        }

        while (startingParentFieldIt != parent->getFields().end())
        {
            const MirClassField *parentField = *startingParentFieldIt;

            MirClassField *clonedField =
                    fieldAlloc.new_object<MirClassField>(MirClassField{ .m_owner = nullptr,
                                                                        .m_type = parentField->m_type,
                                                                        .m_offset = parentField->m_offset,
                                                                        .m_id = m_fields.size(),
                                                                        .m_name = parentField->m_name });

            m_fields.push_back(clonedField);
            ++startingParentFieldIt;
        }

        // DEEP COPY Inherited Methods
        for (const auto *parentMethod : parent->getVTable())
        {
            MirClassMethod *clonedMethod =
                    methodAlloc.new_object<MirClassMethod>(MirClassMethod{ .m_owner = nullptr,
                                                                           .m_func = parentMethod->m_func,
                                                                           .m_offset = parentMethod->m_offset,
                                                                           .m_id = m_vTable.size() });
            m_vTable.push_back(clonedMethod);
        }
    }

    // Append new fields AFTER parent fields
    for (auto *newField : newFields)
    {
        m_fields.push_back(newField);
    }

    if (m_constructor)
    {
        // Push the constructor of THIS class AFTER the parent's fields.
        m_vTable.push_back(std::move(m_constructor));
        m_constructor = nullptr;
    }

    // Merge new methods into the VTable and handle overrides safely.
    for (auto *newMethod : newMethods)
    {
        MirFunction *newFunc = newMethod->m_func;
        bool isOverride = false;

        // Search the deep-copied VTable for an exact signature match
        for (size_t i = 0; i < m_vTable.size(); ++i)
        {
            MirFunction *inheritedFunc = m_vTable[i]->m_func;

            if (inheritedFunc->getName() == newFunc->getName() &&
                inheritedFunc->getReturnType()->getId() == newFunc->getReturnType()->getId())
            {
                const auto &inheritedParams = inheritedFunc->getParameters();
                const auto &newParams = newFunc->getParameters();

                if (inheritedParams.size() != newParams.size())
                {
                    continue;
                }

                bool signatureMatches = true;
                auto inheritedIt = inheritedParams.begin();
                auto newIt = newParams.begin();

                while (inheritedIt != inheritedParams.end() && newIt != newParams.end())
                {
                    if ((*inheritedIt)->getMirType()->getId() != (*newIt)->getMirType()->getId())
                    {
                        signatureMatches = false;
                        break;
                    }
                    ++inheritedIt;
                    ++newIt;
                }

                if (signatureMatches)
                {
                    auto diagBuilder = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirClassBuilder");
                    diagBuilder << m_vTable[i]->m_func->getSourceRef() << "Overridden class method";
                    diagBuilder.appendNote(std::format("New method: {}", newFunc->getName()).c_str(),
                                           newFunc->getSourceRef());

                    m_vTable[i]->m_func = newFunc;
                    isOverride = true;
                    break;
                }
            }
        }

        if (!isOverride)
        {
            m_vTable.push_back(newMethod);
        }
    }

    // Append new vTable field structure for this class layout
    if (!m_vTable.empty())
    {
        const auto &t = m_ctx->getTypeTable();
        const auto &methodPtr = t->getPtr(t->getVoidType());

        MirType *typeArray = t->getArray(methodPtr, m_vTable.size());
        MirType *ptrToArray = t->getPtr(typeArray);

        // Allocate unique vTable field descriptor instance
        MirClassField *vTableField = fieldAlloc.new_object<MirClassField>(MirClassField{ .m_owner = nullptr,
                                                                                         .m_type = ptrToArray,
                                                                                         .m_offset = -1,
                                                                                         .m_id = 0,
                                                                                         .m_name = "vTable" });

        // Ensure "vTable" field is placed at the front of m_fields (Offset 0)
        m_fields.insert(m_fields.begin(), vTableField);
    }

    std::pmr::vector<MirType *> fieldTypes(arena);
    std::pmr::map<std::pmr::string, MirClassField *> fieldNameToField(arena);

    fieldTypes.reserve(m_fields.size());

    for (auto &field : m_fields)
    {
        fieldTypes.push_back(field->m_type);
        fieldNameToField[field->m_name] = field;
    }

    MirType *type = m_ctx->getTypeTable()->getClass(fieldTypes, name);
    MirClass *_class = classAlloc.new_object<MirClass>(parent,
                                                       m_ctx->createId(),
                                                       type,
                                                       name,
                                                       std::move(fieldNameToField),
                                                       std::move(m_fields),
                                                       std::move(m_vTable),
                                                       sourceRef);

    // Finalize owner tracking and assign local context indices
    size_t fieldId = 0;
    for (auto &field : _class->getFields())
    {
        field->m_owner = _class;
        field->m_id = fieldId++;
    }

    size_t methodId = 0;
    for (auto &method : _class->getVTable())
    {
        method->m_owner = _class;
        method->m_id = methodId++;
    }

    auto diagBuilder = m_ctx->getDiagCollector()->builder(DiagnosticMessageType::Diag_Debug, "MirClassBuilder");
    diagBuilder << sourceRef << std::pmr::string(std::format("Built class with id: {}", _class->getId()));

    if (parent)
    {
        diagBuilder.appendNote(std::format("Parent: {}", parent->getName()).c_str(), parent->getSourceRef());
    }

    diagBuilder.appendNote(std::pmr::string(MirPrinter::printToString(_class, MirPrinterDetail::Detailed)), sourceRef);

    if (!m_ctx->appendClass(_class))
    {
        return nullptr;
    }

    setBuildResult(_class);
    return _class;
}

void MirClassBuilder::appendField(MirType *type, const std::pmr::string &name)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirClassField> alloc(arena);

    m_fields.push_back(alloc.new_object<MirClassField>(MirClassField{ .m_owner = nullptr,
                                                                      .m_type = type,
                                                                      .m_offset = -1,
                                                                      .m_id = m_fields.size(),
                                                                      .m_name = name }));
}

void MirClassBuilder::appendMethod(MirFunction *method)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirClassMethod> alloc(arena);

    m_vTable.push_back(alloc.new_object<MirClassMethod>(
            MirClassMethod{ .m_owner = nullptr, .m_func = method, .m_offset = -1, .m_id = m_vTable.size() }));
}

void MirClassBuilder::setConstructor(MirFunction *constructor)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirClassMethod> alloc(arena);

    if (constructor)
    {
        alloc.delete_object(m_constructor);
    }

    m_constructor = alloc.new_object<MirClassMethod>(
            MirClassMethod{ .m_owner = nullptr, .m_func = constructor, .m_offset = -1, .m_id = 0 });
}
