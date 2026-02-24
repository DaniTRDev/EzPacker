#include "SemanticVisitor.h"

void SemanticVisitor::setSemanticContext(const std::shared_ptr<class BasicSemanticContext> &ctx) { m_ctx = ctx; }

const std::shared_ptr<struct BasicSemanticContext> &SemanticVisitor::getSemanticContext() const { return m_ctx; }
