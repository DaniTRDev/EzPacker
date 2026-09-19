#include "MirPasses/MirPass.h"

/**
 * Returns a mutable pointer to the cached result of the most recent execution.
 */
MirPassResult *MirPass::getResult() { return &m_result; }

/**
 * Overwrites the cached execution result with a copy of the supplied result.
 */
void MirPass::setResult(MirPassResult *result) { m_result = *result; }
