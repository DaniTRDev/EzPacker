#include "Decoder/Zydis/EzZydisDecoder.h"

EzZydisDecoder::EzZydisDecoder()
    : m_decoder(std::make_unique<ZydisDecoder>()), m_context(std::make_unique<ZydisDecoderContext>()), IDecoder(),
      m_formatter(std::make_unique<ZydisFormatter>()), LogSink(g_logger.get(), LogSegment("ZYDIS_DECODER"))
{
}

EzZydisDecoder::~EzZydisDecoder()
{
    m_decoder.reset();
    m_context.reset();
    m_formatter.reset();
}

bool EzZydisDecoder::initialize(std::shared_ptr<IArchitecture> arch)
{
    if (!arch)
    {
        LogSink::pushLog(LogMessage("Could not initialize because ARCH is invalid").colorize(Colors::red));
        return false;
    }

    bool validWidth = false;
    bool validArch = false;

    ZydisStackWidth width = ZYDIS_STACK_WIDTH_REQUIRED_BITS; // Invalid.
    switch (arch->getWordSize())
    {
    case 16: {
        validWidth = true;
        width = ZYDIS_STACK_WIDTH_16;
        break;
    }
    case 32: {
        validWidth = true;
        width = ZYDIS_STACK_WIDTH_32;
        break;
    }
    case 64: {
        validWidth = true;
        width = ZYDIS_STACK_WIDTH_64;
        break;
    }
    }

    ZydisMachineMode mode = ZYDIS_MACHINE_MODE_REQUIRED_BITS; // Invalid.
    switch (arch->getType())
    {
    case ArchitectureType::x64: {
        validArch = true;
        mode = ZYDIS_MACHINE_MODE_LONG_64;
        break;
    }
    }

    if (!validWidth)
    {
        LogSink::pushLog(LogMessage("Could not initialize because WORD-SIZE is invalid").colorize(Colors::red));
        return false;
    }
    if (!validArch)
    {
        LogSink::pushLog(LogMessage("Could not initialize because ARCH-TYPE is invalid").colorize(Colors::red));
        return false;
    }

    if (ZydisDecoderInit(m_decoder.get(), mode, width) != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(LogMessage("There was an error initializing Zydis Decoder").colorize(Colors::red));
        return false;
    }

    if (ZydisFormatterInit(m_formatter.get(), ZYDIS_FORMATTER_STYLE_INTEL) != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(LogMessage("There was an error initializing Zydis Formatter").colorize(Colors::red));
        return false;
    }

    for (int16_t regId = 1; regId <= static_cast<int16_t>(ZYDIS_REGISTER_MAX_VALUE); regId++)
    {
        uint8_t registerSize = uint8_t(ZydisRegisterGetWidth(mode, static_cast<ZydisRegister>(regId)) / 8);
        arch->m_registerMap[regId] = registerSize; // Insert.
    }

    LogSink::pushLog(LogMessage("Initialized").colorize(Colors::magenta));
    return true;
}

std::shared_ptr<ParsedDecodedInstruction> EzZydisDecoder::decodeInstruction(
    char *buffer, size_t &address, size_t bufferSize, std::vector<std::shared_ptr<IDecodedOperand>> &operands)
{
    operands.clear();
    if (buffer == nullptr || address >= bufferSize)
    {
        LogSink::pushLog(LogMessage("")
                             .add("Could not decode instruction because buffer or address is invalid")
                             .colorize(Colors::red));
        return nullptr;
    }

    // Decode instruction.

    char formattedInstruction[256];
    ZydisDecodedOperand decodedOperands[ZYDIS_MAX_OPERAND_COUNT];
    std::shared_ptr<ZydisDecodedInstruction> decodedInstruction = std::make_unique<ZydisDecodedInstruction>();
    ZyanStatus status = ZydisDecoderDecodeInstruction(m_decoder.get(), m_context.get(), buffer + address,
                                                      bufferSize - address, decodedInstruction.get());
    if (status != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(
            LogMessage("Could not decode instruction (error: 0x{:x})", status).colorize(Colors::red));
        return nullptr;
    }

    address += decodedInstruction->length; // Increment length.
    status = ZydisDecoderDecodeOperands(m_decoder.get(), m_context.get(), decodedInstruction.get(), decodedOperands,
                                        decodedInstruction->operand_count_visible);

    if (status != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(LogMessage("")
                             .add("Could not decode operands because of Zydis (error: 0x{:x})", status)
                             .colorize(Colors::red));
        return nullptr;
    }

    status = ZydisFormatterFormatInstruction(m_formatter.get(), decodedInstruction.get(), decodedOperands,
                                             decodedInstruction->operand_count_visible, formattedInstruction,
                                             sizeof(formattedInstruction), ZYDIS_RUNTIME_ADDRESS_NONE, nullptr);

    if (status != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(LogMessage("")
                             .add("Could not format instruction because of Zydis (error: 0x{:x})", status)
                             .colorize(Colors::red));
        return nullptr;
    }
    
    for (size_t i = 0; i < decodedInstruction->operand_count_visible; i++)
    {
        // Make a copy of the decoded zydis operand and pass it to our operand type.
        operands.push_back(
            std::make_shared<EzZydisDecodedOperand>(std::make_shared<ZydisDecodedOperand>(decodedOperands[i])));
    }

    std::shared_ptr<ParsedDecodedInstruction> parsed = std::make_shared<ParsedDecodedInstruction>();
    parsed->getFromParser(std::make_unique<ZydisDecodedInstructionParser>(decodedInstruction), operands);

    return parsed;
}
