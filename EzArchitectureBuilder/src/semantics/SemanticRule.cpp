#include "semantics/SemanticRule.h"

SemanticRule::SemanticRule(uint64_t expectedValue) : m_expectedValue(expectedValue)
{
}

bool SemanticRule::match(uint64_t value) const
{
    return m_expectedValue == value;
}

void SemanticRule::setExpectedValue(uint64_t value)
{
    m_expectedValue = value;
}

uint64_t SemanticRule::getExpectedValue() const
{
    return m_expectedValue;
}
