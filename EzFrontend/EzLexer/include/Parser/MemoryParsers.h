#ifndef EZPACKER_MEMORYPARSERS_H
#define EZPACKER_MEMORYPARSERS_H

#include "PrimitiveParsers.h"

namespace NodeParsers
{
using ParsingRule = std::shared_ptr<Rule>;
extern ParsingRule &BaseMemory();      // Parses a memory reference ('(%virtual variable)')
extern ParsingRule &BaseDisplMemory(); // Parses a memory reference ('(%virtual variable, integer)')
extern ParsingRule &
BaseIndexScaleDisplMemory();            // Parses a memory reference ('(%virtual variable, %virtual var, int, int)')
extern ParsingRule &DirectMemory();     // Parses a memory reference ('(%integer)')
extern ParsingRule &IndexScaleMemory(); // Parses a memory reference ('(, %virtual variable, integer)')
extern ParsingRule &MemoryReference();  // Parses a memory reference (any of the ones above).
} // namespace NodeParsers

#endif // EZPACKER_MEMORYPARSERS_H
