#ifndef EZPACKER_TARGETDESC_H
#define EZPACKER_TARGETDESC_H

#include "EzTargetCommon.h"

class TargetDesc
{
  public:
    /**
     * Creates the target with the given ABI and name.
     * @param abi
     * @param targetName
     */
    TargetDesc(ABIDesc *abi, const std::string &targetName);
    
    /**
     * Returns the ABI descriptor associated with this target.
     * @return
     */
    ABIDesc *getABI() const;

    /**
     * Returns the name of the target architecture.
     * @return
     */
    const std::string &getTargetName() const;

  private:
    ABIDesc *m_abi;
    std::string m_targetName;
};

#endif // EZPACKER_TARGETDESC_H
