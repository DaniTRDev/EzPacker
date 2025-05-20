#ifndef EZPACKER_ARCHITECTURE_H
#define EZPACKER_ARCHITECTURE_H

#include "EzArchitectureBuilderCommon.h"
#include "semantics/SemanticRule.h"

class Architecture
{
  public:
    /**
     * Creates the object with default values.
     */
    Architecture();
    
    /**
     * Returns true if the architecture uses big endian.
     * @return bool
     */
    bool isBigEndian() const;
    
    /**
     * Returns true if the architecture uses little endian.
     * @return bool
     */
    bool isLittleEndian() const;
    
    /**
     * Returns the architecture's word size.
     * @return size_t
     */
    size_t getWordSize() const;
    
    /**
     * Returns the name of the architecture.
     * @return
     */
    const std::string &getName() const;
    
    /**
     * Returns the semantic rules of the architecture.
     * @return const std::vector<SemanticRule> &
     */
    const std::vector<SemanticRule> &getSemanticRules();
    
  private:
    bool m_isLittleEndian;
    size_t m_wordSize;
    std::string m_name;
    std::vector<SemanticRule> m_rules;
};

#endif // EZPACKER_ARCHITECTURE_H
