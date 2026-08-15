#ifndef EZPACKER_EZTESTTRIPLETARGETDESC_H
#define EZPACKER_EZTESTTRIPLETARGETDESC_H

#include "EzTestTripleCommon.h"
#include "CallingConvs/EzTestTripleCallingConv.h"
#include "FrameLowerer/EzTestTripleFrameLowerer.h"
#include "InstructionSelector/EzTestTripleInstructionSelector.h"
#include "Legalizer/EzTestTripleExpansionRegistry.h"
#include "Legalizer/EzTestTripleLegalizer.h"
#include "RegisterAllocator/EzTestTripleRegisterAllocator.h"
#include "RegisterBanks/EzTestTripleRegisterBanks.h"
#include "Type/EzTestTripleTypeLayout.h"

class EzTestTripleTargetDesc : public TargetDesc
{
  public:
    /**
     * Creates an empty target desc, using it straight generates UB and will surely crash the program. Before using the
     * object ensure the call to the parametrized constructor has been made.
     */
    EzTestTripleTargetDesc();
    EzTestTripleTargetDesc(MirBuilderContext *ctx, std::pmr::memory_resource *alloc);

    const char *getName() const override { return "EzTestTriple"; }

    IMirTargetTypeLayout *getTypeLayout() override { return &m_typeLayout; }
    MirExpansionRuleRegistry *getExpansionRegistry() override { return m_expansionRegistry; }
    MirFrameLowerer *getFrameLowerer() override { return m_frameLowerer; }
    MirInstructionSelector *getInstructionSelector() override { return m_instructionSelector; }
    MirLegalizer *getLegalizer() override { return m_legalizer; }
    MirRegisterAllocator *getRegisterAllocator() override { return m_registerAllocator; }

    MirType *getMemOperandDisplacementType() override;
    MirType *getNearestLegalType(MirType *type) override;
    size_t getStackSlotSize() const override { return 8; }

    void initialize() override;

    std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() override { return m_callingConvs; }
    std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() override { return m_registerBanks; }

  private:
    MirBuilderContext *m_ctx;
    std::pmr::memory_resource *m_alloc;

    std::pmr::vector<MirRegisterBank *> m_registerBanks;
    std::pmr::vector<CallingConvDesc *> m_callingConvs;

    EzTestTripleTypeLayout m_typeLayout;
    EzTestTripleFrameLowerer *m_frameLowerer{ nullptr };
    EzTestTripleRegisterAllocator *m_registerAllocator{ nullptr };
    MirExpansionRuleRegistry *m_expansionRegistry{ nullptr };
    MirLegalizer *m_legalizer{ nullptr };
    MirInstructionSelector *m_instructionSelector{ nullptr };
};
#endif