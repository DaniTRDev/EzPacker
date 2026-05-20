#ifndef EZPACKER_TARGETDESC_H
#define EZPACKER_TARGETDESC_H

#include "EzTargetCommon.h"

enum class TargetEndianness
{
    LittleEndian,
    BigEndian
};

class TargetDesc
{
  public:
    /**
     * Creates the target with the given ABI, endianness and name.
     * @param abi
     * @param targetName
     */
    TargetDesc(ABIDesc *abi, TargetEndianness endianness, const std::string &targetName);

    /**
     * Returns the ABI descriptor associated with this target.
     * @return
     */
    ABIDesc *getABI() const;

    /**
     * Returns the endianness of the target.
     * @return
     */
    TargetEndianness getEndianness() const;

    /**
     * Returns the strict ABI alignment required for the given type.
     * Automatically delegates to the active ABIDesc.
     * @return
     */
    size_t getAbiAlignment(MirType *type) const;

    /**
     * Returns the name of the target architecture.
     * @return
     */
    const std::string &getTargetName() const;

  private:
    ABIDesc *m_abi;
    TargetEndianness m_endianness;

    std::string m_targetName;
};

#endif // EZPACKER_TARGETDESC_H