#include "Decoder/Decoder.h"

Decoder::Decoder(std::shared_ptr<IArchitecture> arch)
    : m_architecture(arch), LogSink(g_logger.get(), LogSegment("LIFTER").colorize(Colors::cyan))
{
}

Decoder::~Decoder()
{
    m_architecture.reset();
}

bool Decoder::initialize()
{
    ZydisMachineMode mode;
    ZydisStackWidth stackWidth;
    m_architecture->configureDecoder(mode, stackWidth);

    ZyanStatus status = ZydisDecoderInit(&m_decoder, mode, stackWidth);
    if (status != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(LogMessage("Could not initialize Decoder!").colorize(Colors::red));
        return false;
    }

    LogSink::pushLog(LogMessage("Decoder initialized"));
    return true;
}

const IArchitecture *Decoder::getArch() const
{
    return m_architecture.get();
}

std::shared_ptr<ZydisDecodedInstruction> Decoder::decodeInstruction(const char *buffer, size_t address,
                                                                    size_t bufferSize,
                                                                    std::shared_ptr<ZydisDecoderContext> &context)
{
    if (!buffer || bufferSize == 0)
    {
        LogSink::pushLog(LogMessage("")
                             .add("Could not decode instruction because buffer or buffer size is null!")
                             .colorize(Colors::red));
        return nullptr;
    }
    if (address >= bufferSize)
    {
        LogSink::pushLog(
            LogMessage("Could not decode instruction because address is invalid!").colorize(Colors::red));
        return nullptr;
    }
    if (!context)
    {
        LogSink::pushLog(
            LogMessage("Could not decode instruction because CONTEXT is null!").colorize(Colors::red));
        return nullptr;
    }

    std::shared_ptr<ZydisDecodedInstruction> decoded = std::make_shared<ZydisDecodedInstruction>();
    ZyanStatus status =
        ZydisDecoderDecodeInstruction(&m_decoder, context.get(), buffer + address, bufferSize - address, decoded.get());

    if (status != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(LogMessage("Could not decode instruction!").colorize(Colors::red));
        return nullptr;
    }

    return decoded;
}

std::vector<ZydisDecodedOperand> Decoder::decodeOperands(
    const std::shared_ptr<ZydisDecodedInstruction> &decodedInstruction, std::shared_ptr<ZydisDecoderContext> &context)
{
    if (!decodedInstruction || !context)
    {
        LogSink::pushLog(LogMessage("")
                             .add("Could not decode instruction operands because context or instruction is invalid!")
                             .colorize(Colors::red));
        return {};
    }

    std::vector<ZydisDecodedOperand> result;
    result.resize(decodedInstruction->operand_count);
    // Ensure .size & .data are synced at this moment, we will modify .data straight.

    ZyanStatus status = ZydisDecoderDecodeOperands(&m_decoder, context.get(), decodedInstruction.get(),
                                                   (ZydisDecodedOperand *)result.data(), result.size());
    if (status != ZYAN_STATUS_SUCCESS)
    {
        LogSink::pushLog(LogMessage("Could not decode instruction operands!").colorize(Colors::red));
        return {};
    }

    return result;
}
