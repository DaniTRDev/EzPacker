#include "TargetDesc.h"

TargetDesc::TargetDesc(ABIDesc *abi, const std::string &targetName) : m_abi(abi), m_targetName(targetName) {}

ABIDesc *TargetDesc::getABI() const { return m_abi; }

const std::string &TargetDesc::getTargetName() const { return m_targetName; }
