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
     * @brief Construct a new Arg Location object for a physical register.
     * @return bool True if the location is a physical register, false otherwise.
     */
    bool isPhysicalReg() const;

    /**
     * @brief Construct a new Arg Location object for a split register.
     * @return bool True if the location is a split register, false otherwise.
     */
    bool isSplit() const;

    /**
     * @brief Construct a new Arg Location object for a stack location.
     * @return bool True if the location is a stack location, false otherwise.
     */
    bool isStack() const;

    /**
     * @brief Get the physical register location of the argument.
     * @return bool True if the location is a physical register, false otherwise.
     */
    const PhysicalRegLocation &getPhysicalLoc() const;

    /**
     * @brief Get the split register location of the argument.
     * @return bool True if the location is a split register, false otherwise.
     */
    const SplitLocation &getSplitLoc() const;

    /**
     * @brief Get the stack location of the argument.
     * @return bool True if the location is a stack location, false otherwise.
     */
    const StackLocation &getStackLoc() const;

    /**
     * @brief Construct a new Arg Location object with a physical register location.
     * @param loc
     */
    void setLoc(const std::variant<PhysicalRegLocation, SplitLocation, StackLocation> &loc);

  private:
    std::variant<PhysicalRegLocation, SplitLocation, StackLocation> m_location;
};

#endif // EZPACKER_ARGLOCATION_H
