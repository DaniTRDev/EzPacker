#ifndef EZPACKER_IARCHITECTURE_H
#define EZPACKER_IARCHITECTURE_H

#include "EzLifterCommon.h"

enum class ArchitectureType : uint8_t
{
    Invalid,
    x64 // Currently, it's the only supported architecture.
};

class IArchitecture
{
  public:
    virtual ~IArchitecture() = default;
    
    /**
     * Returns the type of the architecture.
     * @return ArchitectureType.
     */
    virtual ArchitectureType getType() = 0;
    
    /**
     * Returns the name of the Architecture.
     * @return const char*
     */
    virtual const char *getName() = 0;

    /**
     * Returns word's size of the architecture in bits.
     * @return size_t
     */
    virtual size_t getWordSize() = 0;
    
    /**
     * Initialized by decoder. First is register ID, used by decoder and decoded instruction structures.
     * Second is the size in bits.
     */
    std::map<int16_t, uint8_t> m_registerMap;
};

#endif // EZPACKER_IARCHITECTURE_H
