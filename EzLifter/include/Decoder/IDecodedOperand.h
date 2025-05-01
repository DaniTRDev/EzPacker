#ifndef EZPACKER_IDECODEDOPERAND_H
#define EZPACKER_IDECODEDOPERAND_H

#include "EzLifterCommon.h"

class IDecodedOperand
{
  public:
    virtual ~IDecodedOperand() = default;

    /**
     * Returns true if: isMemory = true && memory address contains displacement.
     * @return bool
     */
    virtual bool hasDisplacement() const = 0;
    
    /**
     * Returns true if this operand is a destination operand.
     * @return bool
     */
    virtual bool isDestinationOperand() const = 0;

    /**
     * Returns true if this operand is an immediate value.
     * @return bool
     */
    virtual bool isImmediate() const = 0;

    /**
     * Returns true if this operand is a memory operand. Remember a memory operand is composed of
     * at least 1 register and 1 offset.
     * @return bool
     */
    virtual bool isMemory() const = 0;

    /**
     * Returns true if this operand is read.
     * @return bool.
     */
    virtual bool isRead() const = 0;

    /**
     * Returns true if this operand is a register.
     * @return bool
     */
    virtual bool isRegister() const = 0;

    /**
     * Returns true if: isMemory = true && memory address if formed with Instruction Pointer (IP) and an offset.
     * @return bool
     */
    virtual bool isRelative() const = 0;

    /**
     * Returns true if this operand has a signed value. Make sure to only use this value if isImmediate = true.
     * @return bool
     */
    virtual bool isSigned() const = 0;
    
    /**
     * Returns true if this operand is a Source operand.
     * @return bool
     */
    virtual bool isSourceOperand() const = 0;

    /**
     * Returns true if this operand is stack pointer register. This method will be of help for the obfuscator.
     * @return bool.
     */
    virtual bool isStackRegister() const = 0;
    
    /**
     * Returns true if this operand is written.
     * @return bool.
     */
    virtual bool isWritten() const = 0;

    /**
     * Returns the register ID. If -1 is returned, this operand is not a register.
     * @return int16_t
     */
    virtual int16_t getRegister() const = 0;
    
    /**
     * Returns the ID of the base register. If -1 is returned, base register is not used.
     * @return int16_t
     */
    virtual int16_t getBaseRegisterId() const = 0;

    /**
     * Returns the ID of the index register or -1 if it's not used.
     * @return int16_t
     */
    virtual int16_t getIndexRegisterId() const = 0;

    /**
     * Returns the displacement for this operand. If it's 0, displacement is not used.
     * @return int64_t
     */
    virtual int64_t getDisplacement() const = 0;

    /**
     * Returns the immediate value as an SIGNED value. Ensure isSigned = true for correct usage.
     * @return uint64_t
     */
    virtual uint64_t getImmediateS() const = 0;
    
    /**
     * Returns the immediate value as an UNSIGNED value. Ensure isSigned = false for correct usage.
     * @return uint64_t
     */
    virtual uint64_t getImmediateU() const = 0;
    
    /**
     * Returns the scale for this operand. If it's 0, scale is not used.
     * @return uint8_t
     */
    virtual uint8_t getScale() const = 0;
};

#endif // EZPACKER_IDECODEDOPERAND_H
