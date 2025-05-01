#ifndef EZPACKER_EZZYDISDECODEDOPERAND_H
#define EZPACKER_EZZYDISDECODEDOPERAND_H

#include "EzLifterCommon.h"
#include "Decoder/IDecodedOperand.h"

/**
 * Added Ez prefix not to use Zydis' types. ZYDIS_OPERAND_TYPE_POINTER not supported at the moment since it's only used
 * in 16-bit-legacy mode and BIOS instructions.
 */
class EzZydisDecodedOperand : public IDecodedOperand
{
  public:
    /**
     * Creates the object with the given operand.
     * @param zydisOperand
     * @throw assert if zydisOperand == nullptr.
     */
    EzZydisDecodedOperand(std::shared_ptr<ZydisDecodedOperand> zydisOperand);
    
    /**
     * Destroys the object and free resources.
     */
    ~EzZydisDecodedOperand() override;
    
    /**
     * Returns true if: isMemory = true && memory address contains displacement.
     * @return bool
     */
    bool hasDisplacement() const override;
    
    /**
     * Returns true if this operand is a destination operand.
     * @return bool
     */
    bool isDestinationOperand() const override;

    /**
     * Returns true if this operand is an immediate value. Returns false if isMemory = true.
     * @return bool
     */
    bool isImmediate() const override;

    /**
     * Returns true if this operand is a memory operand. Remember a memory operand is composed of
     * at least 1 register and 1 offset.
     * @return bool
     */
    bool isMemory() const override;

    /**
     * Returns true if this operand is read.
     * @return bool.
     */
    bool isRead() const override;

    /**
     * Returns true if this operand is a register. Returns false if isMemory = true.
     * @return bool
     */
    bool isRegister() const override;

    /**
     * Returns true if: isMemory = true && memory address if formed with Instruction Pointer (IP) and an offset.
     * @return bool
     */
    bool isRelative() const override;
    
    /**
     * Returns true if this operand has a signed value. Make sure to only use this value if isImmediate = true.
     * @return bool
     */
    bool isSigned() const override;
    
    /**
     * Returns true if this operand is a Source operand.
     * @return bool
     */
    bool isSourceOperand() const override;

    /**
     * Returns true if this operand is stack pointer register.
     * @return bool.
     */
    bool isStackRegister() const override;
    
    /**
     * Returns true if this operand is written.
     * @return bool.
     */
    bool isWritten() const override;

    /**
     * Returns the register ID. If -1 is returned, this operand is not a register.
     * @return int16_t
     */
    int16_t getRegister() const override;
    
    /**
     * Returns the ID of the base register. If -1 is returned, base register is not used.
     * @return int16_t
     */
    int16_t getBaseRegisterId() const override;
    
    /**
     * Returns the ID of the index register or -1 if it's not used.
     * @return int16_t
     */
    int16_t getIndexRegisterId() const override;
    
    /**
     * Returns the displacement for this operand. If it's 0, displacement is not used.
     * @return int64_t
     */
    int64_t getDisplacement() const override;
    
    /**
     * Returns the immediate value as an SIGNED value. Ensure isSigned = true for correct usage.
     * @return uint64_t
     */
    uint64_t getImmediateS() const override;
    
    /**
     * Returns the immediate value as an UNSIGNED value. Ensure isSigned = false for correct usage.
     * @return uint64_t
     */
    uint64_t getImmediateU() const override;
    
    /**
     * Returns the scale for this operand. If it's 0, scale is not used.
     * @return uint8_t
     */
    uint8_t getScale() const override;
    
    /**
     * Returns a pointer to the internal decoded operand. Not safe to use / modify outside Decoder.
     * @return std::shared_ptr<ZydisDecodedOperand>
     */
    std::shared_ptr<ZydisDecodedOperand> getZydisDecodedOperand();
    
  private:
    std::shared_ptr<ZydisDecodedOperand> m_zydisOperand;
};

#endif // EZPACKER_EZZYDISDECODEDOPERAND_H
