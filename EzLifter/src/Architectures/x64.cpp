#include "Architectures/x64.h"

ArchitectureType x64::getType()
{
    return ArchitectureType::x64;
}

const char *x64::getName()
{
    return "AMD_X64";
}

size_t x64::getWordSize()
{
    return 64;
}
