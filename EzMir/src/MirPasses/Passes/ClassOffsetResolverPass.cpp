#include "Builder/MirBuilderContext.h"
#include "Class/MirClass.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "MirPasses/Passes/ClassOffsetResolverPass.h"
#include "MirPasses/MirPassManager.h"
#include "Printer/MirPrinter.h"
#include "SourceManager/SourceManager.h"
#include "Type/IMirTargetTypeLayout.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"

ClassOffsetResolverPass::ClassOffsetResolverPass(MirBuilderContext *ctx) : m_ctx(ctx) {}

const char *ClassOffsetResolverPass::getName() const { return "ClassOffsetResolverPass"; }

MirPassIterationPlace ClassOffsetResolverPass::getIterationPlace() const { return MirPassIterationPlace::Class; }

MirPassResult ClassOffsetResolverPass::run(MirClass *_class, MirPassManager *passManager)
{
    MirPassResult res{ .m_modifiedMir = false, .m_executed = true, .m_succeeded = true };
    if (!_class)
    {
        return res;
    }

    m_ctx->getDiagCollector()->builder(Diag_Trace, "ClassOffsetResolverPass")
            << _class->getSourceRef() << "Running on class: " << _class->getName();

    SourceReference *sourceRef = _class->getSourceRef();
    IMirTargetTypeLayout *typeLayout = m_ctx->getTypeTable()->getTargetTypeLayout();

    size_t currentOffsetInBytes = 0;
    size_t maxAlignmentInBytes = 1;

    // Resolve fields offsets under target alignment boundaries
    for (auto *field : _class->getFields())
    {
        // If the field was already resolved (inherited from Parent), skip calculation
        // but make sure it updates our max alignment check.
        size_t fieldAlignment = typeLayout->getTypeAlignmentInBytes(field->m_type);
        if (fieldAlignment > maxAlignmentInBytes)
        {
            maxAlignmentInBytes = fieldAlignment;
        }

        // Align the current tracking offset for new fields
        if (currentOffsetInBytes % fieldAlignment != 0)
        {
            size_t padding = fieldAlignment - (currentOffsetInBytes % fieldAlignment);
            currentOffsetInBytes += padding;
        }

        // Assign calculated byte boundary offset to field metadata
        field->m_offset = static_cast<int64_t>(currentOffsetInBytes);
        currentOffsetInBytes += typeLayout->getTypeSizeInBytes(field->m_type);
        res.m_modifiedMir = true;
    }

    // Structural tail-padding: Align final structure size to its largest scalar element multiple
    if (currentOffsetInBytes % maxAlignmentInBytes != 0)
    {
        size_t padding = maxAlignmentInBytes - (currentOffsetInBytes % maxAlignmentInBytes);
        currentOffsetInBytes += padding;
    }

    // Trace-register dynamic VTable method slot offsets
    size_t pointerSize = typeLayout->getPointerSizeInBytes();
    auto &vTable = _class->getVTable();
    for (size_t slotIndex = 0; slotIndex < vTable.size(); ++slotIndex)
    {
        MirClassMethod *method = vTable[slotIndex];
        int64_t expectedOffset = static_cast<int64_t>(slotIndex * pointerSize);

        if (method->m_offset != expectedOffset)
        {
            method->m_offset = expectedOffset;
            res.m_modifiedMir = true;
        }
    }

    // Stash the class reference for printer visibility
    m_resolvedClasses[_class->getId()] = _class;
    return res;
}

void ClassOffsetResolverPass::printResult()
{
    auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, "MirBlockLegalizerPass");
    log << std::format("Printing class offser resolver result:").c_str();

    for (auto &[id, _class] : m_resolvedClasses)
    {
        std::string str = MirPrinter::printToString(_class, MirPrinterDetail::Detailed);
        log.appendNote(str.c_str(), _class->getSourceRef());
    }
}