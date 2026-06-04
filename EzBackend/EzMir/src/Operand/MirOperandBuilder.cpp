#include "Operand/MirOperandBuilder.h"

MirOperandBuilder::MirOperandBuilder(MirBuilderContext *ctx) : m_ctx(ctx) {}

MirOperandBuilder::~MirOperandBuilder() { MirBuilder::flush(); }
