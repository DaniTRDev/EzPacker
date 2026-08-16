#ifndef EZMIR_MIR_PRINTER_H
#define EZMIR_MIR_PRINTER_H

#include "EzMirCommon.h"

enum MirPrinterDetail : uint8_t
{
    General, // Prints regular information
    Detailed // Prints detailed information.
};

class MirPrinter
{
  public:
    /**
     * Prints all the information about a block.
     * If detail is set to General, only block id and instruction count is shown.
     * If detail is set to Detailed, instructions will also be printed.
     */
    static std::string printToString(class MirBlock *block, MirPrinterDetail detail);

    /**
     * Prints all the information about a class, including its parent type (if any).
     * If detail is set to General, method names and field will be printed.
     * If detail is set to Detailed, method signatures and fields will be printed.
     */
    static std::string printToString(class MirClass *_class, MirPrinterDetail detail);

    /**
     * Prints all the information about a function, including the stack frame, parameters, blocks and instructions.
     * If detail is set to General, only the header, param count, stack frame obj count and block count will be printed.
     * If detail is set to Detailed, the entire function will be printed including blocks and instructions inside
     * blocks.
     */
    static std::string printToString(class MirFunction *function, MirPrinterDetail detail);

    /**
     * Prints all the information about a global variable, including its linkage, type and constness.
     * If detail is set to General, initialization data is skipped (only emptyness or filled will be shown).
     * If detail is set to Detailed, the init data is printed in HEX format.
     */
    static std::string printToString(class MirGlobalVar *var, MirPrinterDetail detail);

    /**
     * Prints information about an instruction.
     * If detail is set to General, only OPCODE will be shown.
     * If detail is set to Detailed, operands will also be printed.
     */
    static std::string printToString(class MirInstruction *instr, MirPrinterDetail detail);

    /**
     * Prints the information of an operand.
     */
    static std::string printToString(class MirOperand *operand);

    /**
     * Prints the information of a reference to a register.
     */
    static std::string printToString(const class MirRegisterRef &ref);

    /**
     * Prints the information of a stack frame object.
     */
    static std::string printToString(const class StackFrameObject *obj);
};

#endif // EZMIR_MIR_PRINTER_H
