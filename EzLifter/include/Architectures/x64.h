#ifndef EZPACKER_X64_H
#define EZPACKER_X64_H

#include "IArchitecture.h"

class x64 : public IArchitecture
{
  public:
    /**
     * Returns the type of the architecture.
     * @return ArchitectureType.
     */
    ArchitectureType getType() override;
    
    /**
     * Returns the name of the Architecture.
     * @return const char*
     */
    const char *getName() override;
    
    /**
     * Returns word's size of the architecture in bits.
     * @return size_t
     */
    size_t getWordSize() override;
};

#endif // EZPACKER_X64_H
