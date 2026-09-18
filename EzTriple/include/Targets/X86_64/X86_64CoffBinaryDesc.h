#ifndef EZTRIPLE_X86_64_COFF_BINARY_DESC_H
#define EZTRIPLE_X86_64_COFF_BINARY_DESC_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "CodeSection.h"
#include <memory_resource>

namespace EzTriple
{

class X86_64CoffBinaryDesc : public TargetBinaryDesc
{
  public:
    explicit X86_64CoffBinaryDesc(std::pmr::memory_resource *alloc);
    ~X86_64CoffBinaryDesc() override = default;

    bool isLittleEndian() const override { return true; }
    bool isPositionIndependent() const override { return false; }
    const char *getName() const override { return "x86_64-coff"; }
    CodeSection *getSection(SectionType type) override;
    TargetCodeModel getCodeModel() const override { return TargetCodeModel::Small; }
    TargetObjectFormat getObjectFormat() const override { return TargetObjectFormat::COFF; }
    size_t getFunctionAlignment() const override { return 16; }
    size_t getLoopAlignment() const override { return 16; }
    void initialize() override;
    const std::pmr::unordered_map<SectionType, CodeSection *> &getSections() const override { return m_sections; }

  private:
    std::pmr::memory_resource *m_alloc;
    std::pmr::unordered_map<SectionType, CodeSection *> m_sections;
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_COFF_BINARY_DESC_H
