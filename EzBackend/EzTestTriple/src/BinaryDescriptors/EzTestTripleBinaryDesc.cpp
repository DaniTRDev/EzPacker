#include "BinaryDescriptors/EzTestTripleBinaryDesc.h"

EzTestTripleBinaryDesc::EzTestTripleBinaryDesc(const Options &options) : m_options(options) {}

bool EzTestTripleBinaryDesc::isLittleEndian() const { return true; }

bool EzTestTripleBinaryDesc::isPositionIndependent() const { return m_options.m_isPIC; }

const char *EzTestTripleBinaryDesc::getName() const { return "EzTestTripleBinaryDesc"; }

CodeModel EzTestTripleBinaryDesc::getCodeModel() const { return m_options.m_codeModel; }

ObjectFormat EzTestTripleBinaryDesc::getObjectFormat() const { return m_options.m_objectFormat; }

size_t EzTestTripleBinaryDesc::getFunctionAlignment() const
{
    // 16-byte boundary aligns with standard AMD64 ABI cache-line fetch windows
    return 16;
}

size_t EzTestTripleBinaryDesc::getLoopAlignment() const { return 16; }

size_t EzTestTripleBinaryDesc::getSectionAlignment(std::string_view sectionName) const
{
    if (sectionName == ".text" || sectionName == "text")
    {
        return 16;
    }
    if (sectionName == ".rodata" || sectionName == "rodata")
    {
        // 16-byte alignment accommodates vectorized constants and floating-point literals
        return 16;
    }
    if (sectionName == ".data" || sectionName == "data" || sectionName == ".bss" || sectionName == "bss")
    {
        return 8;
    }

    // Default fallback alignment for custom sections
    return 8;
}