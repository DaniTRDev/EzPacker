#ifndef EZPARCHITECTURE_SEMANTICS_H
#define EZPARCHITECTURE_SEMANTICS_H

#include "EzArchitectureBuilderCommon.h"

/**
 * This class represents a semantic rule. It contains information about the type of the group (instruction-level,,
 * operand-level, module-level or file-level). Its expected value is an uint64_t. This is a design conclusion, it allows
 * a flag-like (enums with static_casts) API and simplifies a lot the code.
 */
class SemanticRule
{
  public:
    /**
     * Creates the rule with the given group and expected value.
     */
    SemanticRule(uint64_t expectedValue);
    
    /**
     * Returns true if given value matches the expected.
     * @param value
     * @return bool
     */
    bool match(uint64_t value) const;
    
    /**
     * Returns the expected value for this rule.
     * @return uint64_t
     */
    uint64_t getExpectedValue() const;
    
    /**
     * Sets the expected value for the rule.
     * @param value
     */
    void setExpectedValue(uint64_t value);
    
  private:
    uint64_t m_expectedValue;
};

#endif // EZPARCHITECTURE_SEMANTICS_H
