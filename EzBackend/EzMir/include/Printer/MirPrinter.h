#ifndef EZPACKER_MIRPRINTER_H
#define EZPACKER_MIRPRINTER_H

#include "EzMirCommon.h"
#include "Function/MirFunction.h"
#include "Builder/MirBuilderContext.h"

enum MirPrinterDetail : uint8_t
{
    General, // Prints regular information
    Detailed // Prints detailed information.
};

class MirPrinter
{
  public:
    /**
     * Prints all the information about a function, including the stack frame, parameters, blocks and instructions.
     * If detail is set to General, only the header, param count, stack frame obj count and block count will be printed.
     * If detail is set to detailed, the entire function will be printed including blocks and instructions inside
     * blocks.
     * @param function
     * @param detail
     * @return
     */
    static std::string printToString(MirFunction *function, MirPrinterDetail detail);

    /**
     * Prints all the information about a block. If detail is set to general, only block id and instruction count is
     * shown. If detail is set to detailed, instructions will also be printed.
     * @param block
     * @param detail
     * @return
     */
    static std::string printToString(MirBlock *block, MirPrinterDetail detail);

    /**
     * Prints information about an instruction. If detail is set to general, only OPCODE will be shown. If detail is set
     * to detailed, operands will also be printed.
     * @param instr
     * @param detail
     * @return
     */
    static std::string printToString(MirInstruction *instr, MirPrinterDetail detail);

    /**
     * Prints the information of an operand.
     * @param operand
     * @return
     */
    static std::string printToString(MirOperand *operand);
};

#endif // EZPACKER_MIRPRINTER_H
