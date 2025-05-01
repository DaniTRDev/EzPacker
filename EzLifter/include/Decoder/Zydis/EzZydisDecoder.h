#ifndef EZPACKER_EZZYDISDECODER_H
#define EZPACKER_EZZYDISDECODER_H

#include "Decoder/IDecoder.h"
#include "EzLifterCommon.h"
#include "EzZydisDecodedOperand.h"
#include "ZydisDecodedInstructionParser.h"

/**
 * Used EZ prefix not to use Zydis' types.
 */
class EzZydisDecoder : public IDecoder, public LogSink
{
  public:
    /**
     * Creates the object and everything needed for the decoder.
     */
    EzZydisDecoder();

    /**
     * Destroys the object and frees resources.
     */
    ~EzZydisDecoder() override;

    /**
     * Initializes the decoder with the current architecture and returns true if succeeded.
     * @return bool
     */
    bool initialize(std::shared_ptr<IArchitecture> arch) override;

    /**
     * Decodes, parses the instruction and returns it. It also returns decoded operands (don't need parsing). If
     * operands is not clear when calling, it will be clear by this function. Will increment address with instruction's
     * size.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param operands
     * @return std::shared_ptr<ParsedDecodedInstruction>
     */
    std::shared_ptr<ParsedDecodedInstruction> decodeInstruction(
        char *buffer, size_t &address, size_t bufferSize,
        std::vector<std::shared_ptr<IDecodedOperand>> &operands) override;

  private:
    std::unique_ptr<ZydisDecoder> m_decoder;
    std::unique_ptr<ZydisDecoderContext> m_context;
    std::unique_ptr<ZydisFormatter> m_formatter;
};

#endif // EZPACKER_ZYDISDECODER_H
