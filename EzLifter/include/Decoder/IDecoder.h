#ifndef EZPACKER_IDECODER_H
#define EZPACKER_IDECODER_H

#include "Architectures/IArchitecture.h"
#include "EzLifterCommon.h"
#include "IDecodedInstructionParser.h"
#include "IDecodedOperand.h"
#include "ParsedDecodedInstruction.h"

/**
 * Interface for a basic decoder.
 */
class IDecoder
{
  public:
    virtual ~IDecoder() = default;

    /**
     * Initializes the decoder with the current architecture and returns true if succeeded.
     * @return bool
     */
    virtual bool initialize(std::shared_ptr<IArchitecture> arch) = 0;

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
    virtual std::shared_ptr<ParsedDecodedInstruction> decodeInstruction(
        char *buffer, size_t &address, size_t bufferSize, std::vector<std::shared_ptr<IDecodedOperand>> &operands) = 0;
};

#endif // EZPACKER_IDECODER_H
