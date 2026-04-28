#include "StackLayout.h"

StackLayout::StackLayout(size_t alignment, size_t shadowSpace) : m_alignment(alignment), m_shadowSpace(shadowSpace) {}

size_t StackLayout::alignAddress(size_t addr) const { return (addr + m_alignment - 1) & ~(m_alignment - 1); }

size_t StackLayout::getAlignment() const { return m_alignment; }

size_t StackLayout::getShadowSpace() const { return m_shadowSpace; }
