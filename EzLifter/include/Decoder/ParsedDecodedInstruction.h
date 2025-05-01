#ifndef EZPACKER_PARSEDDECODEDINSTRUCTION_H
#define EZPACKER_PARSEDDECODEDINSTRUCTION_H

#include "EzLifterCommon.h"
#include "IDecodedInstructionParser.h"

struct ParsedInstructionData
{
    bool m_isBranch;
    bool m_isCall;
    bool m_isJump;
    bool m_isLoad;           // Does the instruction read from memory into a register?
    bool m_isMovRegister;    // Does the instruction move a register to another?
    bool m_isMovRegisterImm; // Does the instruction move an immediate to a register?
    bool m_isStore;          // Does the instruction write memory from a register?
    bool m_isRet;
    bool m_usesMemory; // Does the instruction use memory?

    ConditionType m_conditionType; // Does it check for ZF = 1, ZF = 0, ....

    DecodedInstructionType m_instrType;  // Type of the instruction.
    MemoryReferenceType m_memoryRefType; // Is the memory reference by an absolute value? A combination of base + displ?

    uint8_t m_readFlags;    // Does the instruction read any CPU flag?
    uint8_t m_writtenFlags; // Does the instruction write any CPU flag?
};

class ParsedDecodedInstruction : public LogSink
{
  public:
    /**
     * Creates the object.
     */
    ParsedDecodedInstruction();

    /**
     * Destroys the object and frees resources.
     */
    ~ParsedDecodedInstruction();

    /**
     * Gets parsed instruction data from given parser and operands and returns true if succeeded.
     * @param parser
     * @param operands
     * @return bool
     */
    bool getFromParser(std::unique_ptr<IDecodedInstructionParser> parser,
                       const std::vector<std::shared_ptr<IDecodedOperand>> &operands);

    /**
     * Returns parsed data, if parsed. Other ways it returns nullptr.
     * @return std::shared_ptr<ParsedInstructionData>
     */
    std::shared_ptr<ParsedInstructionData> getData();

  private:
    std::shared_ptr<ParsedInstructionData> m_parsedData;
};

#endif // EZPACKER_PARSEDDECODEDINSTRUCTION_H
