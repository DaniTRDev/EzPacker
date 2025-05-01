#ifndef EZPACKER_EZLIFTER_H
#define EZPACKER_EZLIFTER_H

#include "Decoder/IDecoder.h"
#include "EzLifterCommon.h"
#include "InstructionLifters/IInstructionLifter.h"
#include "InstructionLifters/InstructionLifters.h"

/**
 * This class represents the lifter. Which will translate our parsed decoded instructions to LLVM's IR. It only supports
 * lifting whole functions.
 */
class EzLifter : public LogSink
{
  public:
    /**
     * Creates the object with the given architecture and decoder.
     * @param arch
     * @param decoder
     */
    EzLifter(std::unique_ptr<IArchitecture> arch, std::unique_ptr<IDecoder> decoder);

    /**
     * Destroys the object and free resources.
     */
    ~EzLifter();

    /**
     * Initializes the lifter. Creating everything that's needed to lift a function.
     * @return
     */
    bool initialize();

    /**
     * Lifts function at buffer+startAddress to LLVM's IR. It handles relative addressing thanks to baseAddress,
     * which represents the base address in which the ENTIRE buffer would be loaded.
     *
     * It's assumed that buffer contains the entire application to be lifted. Other ways memory references might not be
     * present in the given piece of code.
     *
     * Important: Every time a function is lifted, initialize MUST be called first!.
     * @param buffer
     * @param baseAddress
     * @param bufferSize
     * @param startAddress
     * @return llvm::Function*
     */
    llvm::Function *lift(char *buffer, size_t baseAddress, size_t bufferSize, size_t startAddress);

  private:
    llvm::Function *m_function;
    std::shared_ptr<IArchitecture> m_architecture; // Target // Source architecture.
    std::unique_ptr<IDecoder> m_decoder;           // Decoder used by this lifter.
    std::unique_ptr<llvm::LLVMContext> m_context;
    std::unique_ptr<llvm::Module> m_module;
};

#endif // EZPACKER_EZLIFTER_H
