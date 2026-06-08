#ifndef EZPACKER_MIRPRINTER_H
#define EZPACKER_MIRPRINTER_H

#include "EzMirCommon.h"
#include "Function/MirFunction.h"
#include "Builder/MirBuilderContext.h"

class MirPrinter
{
  public:
    /**
     * Prints all the information about a function, including the stack frame, parameters, blocks and instructions.
     * @param function
     * @return
     */
    static std::string printToString(MirFunction *function);

    static std::string printToString(MirBlock *function);

    static std::string printToString(MirInstruction *function);

    static std::string printToString(MirOperand *operand);
};

#endif // EZPACKER_MIRPRINTER_H
