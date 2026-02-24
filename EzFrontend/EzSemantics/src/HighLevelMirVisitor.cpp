#include "HighLevelMirVisitor.h"

void HighLevelMirVisitor::setSemanticContext(const std::shared_ptr<struct BasicSemanticContext> &ctx) { m_ctx = ctx; }

const std::shared_ptr<struct BasicSemanticContext> &HighLevelMirVisitor::getSemanticContext() const { return m_ctx; }
