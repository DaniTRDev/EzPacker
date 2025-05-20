#include "Architecture.h"

Architecture::Architecture() : m_isLittleEndian(false), m_wordSize(0), m_name()
{
}

bool Architecture::isBigEndian() const
{
    return !isLittleEndian();
}

bool Architecture::isLittleEndian() const
{
    return m_isLittleEndian;
}

size_t Architecture::getWordSize() const
{
    return m_wordSize;
}

const std::string &Architecture::getName() const
{
    return m_name;
}

const std::vector<SemanticRule> &Architecture::getSemanticRules()
{
    return m_rules;
}
