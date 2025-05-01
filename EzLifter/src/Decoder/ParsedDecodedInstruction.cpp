#include "Decoder/ParsedDecodedInstruction.h"

ParsedDecodedInstruction::ParsedDecodedInstruction()
    : m_parsedData(nullptr), LogSink(g_logger.get(), LogSegment("ParsedDecodedInstruction").colorize(Colors::cyan))
{
}

ParsedDecodedInstruction::~ParsedDecodedInstruction()
{
    m_parsedData.reset();
}

bool ParsedDecodedInstruction::getFromParser(std::unique_ptr<IDecodedInstructionParser> parser, const std::vector<std::shared_ptr<IDecodedOperand>> &operands)
{
    if (!parser)
    {
        LogSink::pushLog(
            LogMessage("").add("Could not getFromParser instruction because parser is invalid").colorize(Colors::red));
        return false;
    }

    if (operands.empty())
    {
        LogSink::pushLog(
            LogMessage("").add("Could not getFromParser instruction because operands are invalid").colorize(Colors::red));
        return false;
    }
    
    std::shared_ptr<ParsedInstructionData> result;
    result->m_isBranch = parser->isBranch();
    result->m_isCall = parser->isCall();
    result->m_isJump = parser->isJump();
    result->m_isLoad = parser->isLoad(operands);
    result->m_isMovRegister = parser->isMovRegister(operands);
    result->m_isMovRegisterImm = parser->isMovRegisterImm(operands);
    result->m_isStore = parser->isStore(operands);
    result->m_isRet = parser->isRet();
    result->m_usesMemory = parser->usesMemory(operands);
    result->m_conditionType = parser->getConditionType();
    result->m_instrType = parser->getType();
    result->m_memoryRefType = parser->getMemoryRefType(operands);
    result->m_readFlags = parser->getReadFlags();
    result->m_writtenFlags = parser->getWrittenFlags();
    
    m_parsedData = result;
    return true;
}

std::shared_ptr<ParsedInstructionData> ParsedDecodedInstruction::getData()
{
    return m_parsedData;
}
