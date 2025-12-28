#include "SemanticVisitor.h"

SemanticVisitor::SemanticVisitor(std::shared_ptr<struct ISemanticAnalyzerContext> ctx) : m_ctx(std::move(ctx)) {}
