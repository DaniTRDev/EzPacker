#include "MirPasses/Passes/RelativeReferenceLowerer.h"

RelativeReferenceLowerer::RelativeReferenceLowerer(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *RelativeReferenceLowerer::getName() const { return "RelativeReferenceLowererPass"; }

MirPassIterationPlace RelativeReferenceLowerer::getIterationPlace() const { return MirPassIterationPlace::Instruction; }

MirPassResult RelativeReferenceLowerer::run(std::pmr::list<MirInstruction *> &instrList,
                                            std::pmr::list<MirInstruction *>::iterator it,
                                            class MirPassManager *passManager)
{
    MirInstruction *instr = *it;
    MirPassResult res{ .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
    MirOperandBuilder opBuilder(m_ctx);
    auto &operandList = instr->getOperands();

    for (size_t i = 0; i < operandList.size(); i++)
    {
        MirOperand *op = operandList[i];

        if (!op->isOfType<MirReference>())
            continue;

        MirReference *ref = op->get<MirReference>();
        SourceReference *sourceRef = ref->getSourceRef();
        MirMemory *newOp = nullptr;

        switch (ref->getRefType())
        {
            case MirReferenceType::ClassField:
            {
                MirRegister *classPtr = m_ctx->getRegisterById(ref->getRefId());
                if (!classPtr)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "RelativeReferenceLowererPass")
                            << sourceRef << "Invalid class pointer";
                    res.m_succeeded = false;
                    return res;
                }

                MirClass *_class = m_ctx->getClassByTypeId(classPtr->getMirType()->getPointedType()->getId());
                if (!_class)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "RelativeReferenceLowererPass")
                            << sourceRef << "Invalid referenced class";
                    res.m_succeeded = false;
                    return res;
                }

                MirClassField *field = _class->getFieldById(ref->getOffset());
                if (!field || field->m_offset == -1)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "RelativeReferenceLowererPass")
                            << sourceRef << "Invalid referenced class field";
                    res.m_succeeded = false;
                    return res;
                }

                newOp = opBuilder.buildMem(field->m_type, classPtr, FlexInt(field->m_offset), ref->getSourceRef());
                break;
            }
            case MirReferenceType::ClassMethod:
            {
                MirRegister *classPtr = m_ctx->getRegisterById(ref->getRefId());
                if (!classPtr)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "RelativeReferenceLowererPass")
                            << sourceRef << "Invalid class pointer";
                    res.m_succeeded = false;
                    return res;
                }

                MirClass *_class = m_ctx->getClassByTypeId(classPtr->getMirType()->getPointedType()->getId());
                if (!_class)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "RelativeReferenceLowererPass")
                            << sourceRef << "Invalid referenced class";
                    res.m_succeeded = false;
                    return res;
                }

                MirClassMethod *method = _class->getMethodById(ref->getOffset());
                if (!method || method->m_offset == -1)
                {
                    m_ctx->getDiagCollector()->builder(Diag_Error, "RelativeReferenceLowererPass")
                            << sourceRef << "Invalid referenced class method";
                    res.m_succeeded = false;
                    return res;
                }

                MirType *ptrType = m_ctx->getTypeTable()->getPtr(method->m_func->getType());
                newOp = opBuilder.buildMem(ptrType, classPtr, FlexInt(method->m_offset), ref->getSourceRef());
                break;
            }
            case MirReferenceType::ConstantArrayElement:
            {
                MirRegister *arrayPtr = m_ctx->getRegisterById(ref->getRefId());
                MirType *arrayElem = arrayPtr->getMirType()->getPointedType()->getArrayElementType();

                int64_t offset = arrayElem->getTotalSizeInBytes() * ref->getOffset();
                newOp = opBuilder.buildMem(arrayElem, arrayPtr, FlexInt(offset), ref->getSourceRef());
                break;
            }

            default:
            {
                m_ctx->getDiagCollector()->builder(Diag_Trace, "RelativeReferenceLowererPass")
                        << sourceRef << "Skipping invalid reference: " << ref->toString().c_str();
                break;
            }
        }

        if (newOp != nullptr)
        {
            auto diag = m_ctx->getDiagCollector()->builder(Diag_Trace, "RelativeReferenceLowererPass");
            diag << sourceRef << "Lowered relative reference: " << ref->toString().c_str();
            diag.appendNote(MirPrinter::printToString(newOp).c_str(), sourceRef);

            operandList[i] = newOp; // Switch the operand in-place.
            res.m_modifiedMir = true;
        }
    }

    return res;
}

void RelativeReferenceLowerer::printResult() const {}