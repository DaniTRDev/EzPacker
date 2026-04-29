#include "MirLegalizerContext.h"

MirLegalizerContext::MirLegalizerContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                         const std::shared_ptr<SourceManager> &sourceManager) :
    ErrorEmitter(errorCollector, sourceManager)
{
}

void MirLegalizerContext::setEmitter(MirEmitter *emitter) { m_emitter = emitter; }

void MirLegalizerContext::setAbiDesc(ABIDesc *abiDesc) { m_abiDesc = abiDesc; }

void MirLegalizerContext::setPassManager(MirPassManager *passManager) { m_passManager = passManager; }

MirEmitter *MirLegalizerContext::getEmitter() const { return m_emitter; }

ABIDesc *MirLegalizerContext::getAbiDesc() const { return m_abiDesc; }

MirPassManager *MirLegalizerContext::getPassManager() const { return m_passManager; }
