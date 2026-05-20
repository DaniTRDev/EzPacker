#ifndef EZPACKER_MIRPRINTER_H
#define EZPACKER_MIRPRINTER_H

#include "EzMirCommon.h"
#include "Function/MirFunction.h"

class MirPrinter
{
  public:
    /**
     * Prints all the information about a function, including the stack frame, parameters, blocks and instructions.
     * @param function
     * @return
     */
    std::string printToString(MirFunction *function) const;

    std::string printToString(MirBlock *function) const;

    std::string printToString(MirInstruction *function) const;

  private:
};

#endif // EZPACKER_MIRPRINTER_H
