#include "Class/MirClassBuilder.h"

MirClassBuilder::MirClassBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

MirClass *MirClassBuilder::build(MirClass *parent, const std::pmr::string &name, SourceReference *sourceRef)
{
    std::pmr::memory_resource *arena = m_ctx->getGlobalAllocator();
    std::pmr::polymorphic_allocator<MirClass> alloc(arena);

    // Keep track of the methods declared explicitly on this builder session
    std::pmr::vector<MirClassMethod *> newMethods = std::move(m_vTable);
    if (parent)
    {
        // Inherit fields and methods from parent.
        auto startingParentFieldIt = parent->getFields().begin();

        if (!parent->getVTable().empty())
        {
            // We don't want to include the vTable ptr of the parent.
            std::advance(startingParentFieldIt, 1);
        }

        m_fields.insert(m_fields.begin(), startingParentFieldIt, parent->getFields().end());
        m_vTable.insert(m_vTable.begin(), parent->getVTable().begin(), parent->getVTable().end());
    }

    // Merge new methods into the VTable and handle overrides.
    for (auto *newMethod : newMethods)
    {
        MirFunction *newFunc = newMethod->m_func;
        bool isOverride = false;

        // Search the inherited VTable for an exact signature match
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
                    diagBuilder << m_vTable[i]->m_func->getSourceRef() << "Overriden class method";
                    diagBuilder.appendNote(std::format("New method: {}", newFunc->getName()).c_str(),
                                           newFunc->getSourceRef());

                    m_vTable[i]->m_func = newFunc;
                    isOverride = true;

                    break;
                }
            }
        }

        // If it didn't match any parent method, it's a brand new virtual function
        if (!isOverride)
        {
            m_vTable.push_back(newMethod);
        }
    }

    if (!m_vTable.empty())
    {
        const auto &t = m_ctx->getTypeTable();
        const auto &methodPtr = t->getPtr(t->getVoidType());

        MirType *typeArray = m_ctx->getTypeTable()->getArray(methodPtr, m_vTable.size());
        appendField(typeArray, "vTable");

        // Ensure "vTable" field is placed at front of m_fields (Offset 0)
        MirClassField *vTable = m_fields.back();
        m_fields.pop_back();

        m_fields.insert(m_fields.begin(), vTable);
    }

    std::pmr::vector<MirType *> fieldTypes(arena);
    std::pmr::map<std::pmr::string, MirClassField *> fieldNameToField(arena);
    for (auto &field : m_fields)
    {
        fieldTypes.push_back(field->m_type);
        fieldNameToField[field->m_name] = field;
    }

    MirType *type = m_ctx->getTypeTable()->getClass(fieldTypes, name);
    MirClass *_class = alloc.new_object<MirClass>(parent,
                                                  m_ctx->createId(),
                                                  type,
                                                  name,
                                                  std::move(fieldNameToField),
                                                  std::move(m_fields),
                                                  std::move(m_vTable),
                                                  sourceRef);

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
            MirClassMethod{ .m_owner = nullptr, .m_func = method, .m_id = m_vTable.size() }));
}