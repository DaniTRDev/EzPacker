#include "ArgLocation.h"

bool ArgLocation::isPhysicalReg() const { return std::holds_alternative<PhysicalRegLocation>(m_location); }

bool ArgLocation::isSplit() const { return std::holds_alternative<SplitLocation>(m_location); }

bool ArgLocation::isStack() const { return std::holds_alternative<StackLocation>(m_location); }

const PhysicalRegLocation &ArgLocation::getPhysicalLoc() const { return get<PhysicalRegLocation>(m_location); }

const SplitLocation &ArgLocation::getSplitLoc() const { return get<SplitLocation>(m_location); }

const StackLocation &ArgLocation::getStackLoc() const { return get<StackLocation>(m_location); }

void ArgLocation::setLoc(const std::variant<PhysicalRegLocation, SplitLocation, StackLocation> &loc)
{
    m_location = loc;
}
