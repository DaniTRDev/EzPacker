#ifndef EZPACKER_ARGLOCATION_H
#define EZPACKER_ARGLOCATION_H

#include "EzABICommon.h"

using PhysicalRegId = size_t;
using StackOffset = int64_t;

struct PhysicalRegLocation
{
    PhysicalRegId m_id;
    size_t m_sizeInBytes;
};

struct SplitLocation
{
    std::vector<PhysicalRegLocation> m_regs;
};

struct StackLocation
{
    StackOffset m_offset;
};

class ArgLocation
{
  public:
    /**
     * @brief Checks if the location represents a physical register.
     * @returns True if the location is a physical register, false otherwise.
     */
    bool isPhysicalReg() const;

    /**
     * @brief Checks if the location represents a split register (multiple registers).
     * @returns True if the location is a split register, false otherwise.
     */
    bool isSplit() const;

    /**
     * @brief Checks if the location represents a position on the stack.
     * @returns True if the location is on the stack, false otherwise.
     */
    bool isStack() const;

    /**
     * @brief Retrieves the physical register location details.
     * @returns The physical register location.
     */
    const PhysicalRegLocation &getPhysicalLoc() const;

    /**
     * @brief Retrieves the split register location details.
     * @returns The split register location.
     */
    const SplitLocation &getSplitLoc() const;

    /**
     * @brief Retrieves the stack location details.
     * @returns The stack location.
     */
    const StackLocation &getStackLoc() const;

    /**
     * @brief Sets the internal location variant.
     * @param loc The new location (physical, split, or stack).
     */
    void setLoc(const std::variant<PhysicalRegLocation, SplitLocation, StackLocation> &loc);

  private:
    std::variant<PhysicalRegLocation, SplitLocation, StackLocation> m_location;
};

#endif // EZPACKER_ARGLOCATION_H
