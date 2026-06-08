#include "MirPasses/MirPass.h"

MirPassResult *MirPass::getResult() { return &m_result; }

void MirPass::setResult(MirPassResult *result) { m_result = *result; }

