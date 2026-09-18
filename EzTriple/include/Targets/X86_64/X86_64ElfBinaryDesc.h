#ifndef EZTRIPLE_X86_64_ELF_BINARY_DESC_H
#define EZTRIPLE_X86_64_ELF_BINARY_DESC_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetBinaryDesc.h"
#include "CodeSection.h"
#include <memory_resource>

namespace EzTriple
{

class X86_64ElfBinaryDesc : public TargetBinaryDesc
{
  public:
    explicit X86_64ElfBinaryDesc(std::pmr::memory_resource *alloc, bool isPic = false);
    ~X86_64ElfBinaryDesc() override = default;

    bool isLittleEndian() const override { return true; }
    bool isPositionIndependent() const override { return m_isPic; }
    const char *getName() const override { return "x86_64-elf"; }
    CodeSection *getSection(SectionType type) override;
    TargetCodeModel getCodeModel() const override { return TargetCodeModel::Small; }
    TargetObjectFormat getObjectFormat() const override { return TargetObjectFormat::ELF; }
    size_t getFunctionAlignment() const override { return 16; }
    size_t getLoopAlignment() const override { return 16; }
    void initialize() override;
    const std::pmr::unordered_map<SectionType, CodeSection *> &getSections() const override { return m_sections; }

  private:
    std::pmr::memory_resource *m_alloc;
    bool m_isPic{ false };
    std::pmr::unordered_map<SectionType, CodeSection *> m_sections;
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_ELF_BINARY_DESC_H
