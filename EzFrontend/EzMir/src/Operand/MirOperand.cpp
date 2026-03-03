#include "Operand/MirOperand.h"

MirOperand::MirOperand(const MirOperand &copy) : m_data(copy.m_data) {}

MirOperand::MirOperand(const MirOperand &&other) { m_data = std::move(other.m_data); }

MirOperandType MirOperand::getType() const { return static_cast<MirOperandType>(m_data.index()); }

MirBigInteger *MirOperand::getBigInteger() { return std::get_if<MirBigInteger>(&m_data); }
const MirBigInteger *MirOperand::getBigInteger() const { return std::get_if<MirBigInteger>(&m_data); }

MirDouble *MirOperand::getDouble() { return std::get_if<MirDouble>(&m_data); }
const MirDouble *MirOperand::getDouble() const { return std::get_if<MirDouble>(&m_data); }

MirInteger *MirOperand::getInteger() { return std::get_if<MirInteger>(&m_data); }
const MirInteger *MirOperand::getInteger() const { return std::get_if<MirInteger>(&m_data); }

MirMemory *MirOperand::getMemory() { return std::get_if<MirMemory>(&m_data); }
const MirMemory *MirOperand::getMemory() const { return std::get_if<MirMemory>(&m_data); }

MirReference *MirOperand::getReference() { return std::get_if<MirReference>(&m_data); }
const MirReference *MirOperand::getReference() const { return std::get_if<MirReference>(&m_data); }

MirRegister *MirOperand::getRegister() { return std::get_if<MirRegister>(&m_data); }
const MirRegister *MirOperand::getRegister() const { return std::get_if<MirRegister>(&m_data); }

MirOperand::VariantType &MirOperand::getVariant() { return m_data; }

const MirOperand::VariantType &MirOperand::getVariant() const { return m_data; }
