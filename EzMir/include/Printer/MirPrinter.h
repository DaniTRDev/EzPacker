#ifndef EZMIR_MIR_PRINTER_H
#define EZMIR_MIR_PRINTER_H

#include "EzMirCommon.h"

/**
 * Level of verbosity for MIR textual disassembly and formatting.
 */
enum MirPrinterDetail : uint8_t
{
    General, // Prints high-level summary information
    Detailed // Prints complete detailed instructions, operands, and payload hex
};

/**
 * Formatting mode for MIR textual emission.
 */
enum class MirPrinterMode : uint8_t
{
    Diagnostic, // Human inspection with Tier, Opcode column alignment, and unselected tags
    Parseable   // Strict, round-trip parseable valid .mir text
};

/**
 * Textual formatter and disassembler for MIR data structures.
 * Produces readable representations of blocks, functions, global variables, instructions, operands, and stack slots.
 */
class MirPrinter
{
  public:
    /**
     * Formats an entire MIR compilation context module into a string representation.
     */
    static std::string printModule(class MirBuilderContext *ctx, MirPrinterMode mode = MirPrinterMode::Parseable);

    /**
     * Formats a MirFunction into a string representation according to the specified mode.
     */
    static std::string printFunction(class MirFunction *function, MirPrinterMode mode = MirPrinterMode::Parseable);

    /**
     * Formats a MirBlock into a string representation according to the specified mode.
     */
    static std::string printBlock(class MirBlock *block, MirPrinterMode mode = MirPrinterMode::Parseable);

    /**
     * Formats a MirInstruction into a string representation according to the specified mode.
     */
    static std::string printInstruction(class MirInstruction *instr, MirPrinterMode mode = MirPrinterMode::Parseable);

    /**
     * Formats a MirGlobalVar into a string representation according to the specified mode.
     */
    static std::string printGlobalVar(class MirGlobalVar *var, MirPrinterMode mode = MirPrinterMode::Parseable);

    /**
     * Formats a single MirOperand into textual representation according to the specified mode.
     */
    static std::string printOperand(class MirOperand *operand, MirPrinterMode mode = MirPrinterMode::Parseable);

    /**
     * Formats a MirBlock into a string representation.
     * General detail prints block ID and instruction count.
     * Detailed detail prints all contained instructions line-by-line.
     */
    static std::string printToString(class MirBlock *block, MirPrinterDetail detail);

    /**
     * Formats a MirFunction into a string representation.
     * General detail prints function signature, parameter count, and frame metrics.
     * Detailed detail prints the full CFG including all basic blocks and instructions.
     */
    static std::string printToString(class MirFunction *function, MirPrinterDetail detail);

    /**
     * Formats a MirGlobalVar into a string representation.
     * General detail prints linkage and type.
     * Detailed detail prints full hexadecimal payload data.
     */
    static std::string printToString(class MirGlobalVar *var, MirPrinterDetail detail);

    /**
     * Formats a MirInstruction into a string representation.
     * General detail prints opcode name.
     * Detailed detail prints opcode with destination and source operands.
     */
    static std::string printToString(class MirInstruction *instr, MirPrinterDetail detail);

    /**
     * Formats a single MirOperand into textual representation.
     */
    static std::string printToString(class MirOperand *operand);

    /**
     * Formats a MirRegisterRef into its class and register identifier text.
     */
    static std::string printToString(const class MirRegisterRef &ref);

    /**
     * Formats a StackFrameObject into its offset, size, and source origin text.
     */
    static std::string printToString(const class StackFrameObject *obj);
};

#endif // EZMIR_MIR_PRINTER_H
