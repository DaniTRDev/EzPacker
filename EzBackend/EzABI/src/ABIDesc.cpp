#include "ABIDesc.h"

ABIDesc::ABIDesc() :
    m_endianess(LittleEndian), m_returnValueLoc(), m_stackLayout(0, 0), m_calleeSavedRegs(), m_callerSavedRegs()
{
}

const AbiEndianness &ABIDesc::getEndianness() const { return m_endianess; }

const ArgLocation &ABIDesc::getReturnValueLoc() const { return m_returnValueLoc; }

PhysicalRegId ABIDesc::getStackFrameReg() const { return m_stackFrame; }

PhysicalRegId ABIDesc::getStackReg() const { return m_stackReg; }

size_t ABIDesc::getRegSizeInBits() const { return m_regSizeInBits; }

size_t ABIDesc::getStackOffsetSizeInBits() { return m_stackOffsetSizeInBits; }

const StackLayout &ABIDesc::getStackLayout() const { return m_stackLayout; }

void ABIDesc::setArgRegs(const std::vector<PhysicalRegId> &regs)
{
    m_argRegs = regs;
}

void ABIDesc::setEndianness(AbiEndianness endianness) { m_endianess = endianness; }

void ABIDesc::setReturnValueLoc(const ArgLocation &loc) { m_returnValueLoc = loc; }

void ABIDesc::setStackLayout(const StackLayout &layout) { m_stackLayout = layout; }

void ABIDesc::setCalleeSavedRegs(const std::vector<PhysicalRegId> &regs) { m_calleeSavedRegs = regs; }

void ABIDesc::setCallerSavedRegs(const std::vector<PhysicalRegId> &regs) { m_callerSavedRegs = regs; }

void ABIDesc::setRegSizeInBits(size_t sizeInBits) { m_regSizeInBits = sizeInBits; }

void ABIDesc::setStackFrame(PhysicalRegId stackFrame) { m_stackFrame = stackFrame; }

void ABIDesc::setStackOffsetSize(size_t sizeInBits) { m_stackOffsetSizeInBits = sizeInBits; }

void ABIDesc::setStackReg(PhysicalRegId stackReg) {}

const std::vector<PhysicalRegId> &ABIDesc::getCalleeSavedRegs() const { return m_calleeSavedRegs; }

const std::vector<PhysicalRegId> &ABIDesc::getCallerSavedRegs() const { return m_callerSavedRegs; }

const std::vector<PhysicalRegId> &ABIDesc::getArgRegs() const { return m_argRegs; }
