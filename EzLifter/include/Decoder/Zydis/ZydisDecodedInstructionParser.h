#ifndef EZPACKER_ZYDISDECODEDINSTRUCTIONPARSER_H
#define EZPACKER_ZYDISDECODEDINSTRUCTIONPARSER_H

#include "Decoder/IDecodedInstructionParser.h"
#include "EzLifterCommon.h"
#include "ZydisTranslator.h"

/**
 * Added Ez prefix not to use Zydis' types.
 */
class ZydisDecodedInstructionParser : public IDecodedInstructionParser
{
  public:
    /**
     * Constructs the object with the given Zydis decoded instruction.
     * @param zydisInstruction
     */
    ZydisDecodedInstructionParser(std::shared_ptr<ZydisDecodedInstruction> zydisInstruction);

    /**
     * Destroys the object.
     */
    ~ZydisDecodedInstructionParser() override;
    
    /**
     * Returns true if the instruction is a branch (conditional / short JUMPs).
     * @return bool
     */
    bool isBranch() const override;

    /**
     * Returns true if this instruction is a call.
     * @return bool
     */
    bool isCall() const override;

    /**
     * Returns true if the instruction is a JUMP instruction. Remember jump and branch instructions are different.
     * @return bool
     */
    bool isJump() const override;

    /**
     * Returns true if the instruction is a LOAD (moves data from memory to a register).
     * @param operands
     * @return bool
     */
    bool isLoad(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const override;

    /**
     * Returns if this instruction moves a register to another.
     * @param operands
     * @return bool.
     */
    bool isMovRegister(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const override;
    
    /**
     * Returns if this instruction moves an immediate to a register.
     * @param operands
     * @return bool.
     */
    bool isMovRegisterImm(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const override;
    
    /**
     * Returns true if this instruction is a return.
     * @return bool
     */
    bool isRet() const override;

    /**
     * Returns true if this instruction is a STORE (moves data from register to memory).
     * @param operands
     * @return bool
     */
    bool isStore(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const override;

    /**
     * Does the instruction use memory?
     * @param operands
     * @return bool
     */
    bool usesMemory(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const override;

    /**
     * Returns the condition type of the instruction: How does it interact with read flags.
     * @return ConditionType
     */
    ConditionType getConditionType() const override;
    
    /**
     * Returns the type of the instruction.
     * @return DecodedInstructionType
     */
    DecodedInstructionType getType() const override;
    
    /**
     * Returns the memory reference type for this instruction. If !usesMemory, Invalid is returned.
     * @param operands
     * @return const MemoryReferenceType &
     */
    MemoryReferenceType getMemoryRefType(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const override;

    /**
     * Returns flags that are READ.
     * @return uint8_t
     */
    uint8_t getReadFlags() const override;

    /**
     * Returns flags that are WRITTEN by this instruction.
     * @return uint8_t
     */
    uint8_t getWrittenFlags() const override;

    /**
     * Returns a pointer to the internal decoded instruction. Not safe to use / modify outside Decoder.
     * @return std::shared_ptr<ZydisDecodedInstruction>
     */
    std::shared_ptr<ZydisDecodedInstruction> getZydisDecodedInstruction();

  private:
    std::shared_ptr<ZydisDecodedInstruction> m_zydisInstruction;
};

#endif // EZPACKER_ZYDISDECODEDINSTRUCTIONPARSER_H
