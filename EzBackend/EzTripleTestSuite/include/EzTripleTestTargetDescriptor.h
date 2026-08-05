#ifndef EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H
#define EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H

#include "Descriptors/TargetDesc.h"
#include "EzTripleTestExpansionRecipe.h"
#include "EzTripleTestFrameLowerer.h"

/**
 * This class acts a simple target descriptor that is already defined. Its purpose is just to acts as an already-defined
 * descriptor for tests.
 *
 * Defined registers:
 *  GPRs: { 1, 2, 3 }
 *  FPRs (Volatile): { 4, 5 }
 *
 *  FRAME: { 6 (FP), 7 (SP) }
 *
 *  FrameLowerer: EzTripleTestFrameLowerer
 */
class EzTripleTestTargetDesc : public TargetDesc
{
  public:
    /**
     * Creates the target descriptor with the given context.
     */
    EzTripleTestTargetDesc(MirBuilderContext *ctx);

    /**
     * Returns "EzTripleTargetDesc"
     */
    const char *getName() const override;

    /**
     * Returns the expansion recipes for this target.
     */
    const ExpansionRecipe *getExpansionRecipes() override;

    /**
     * Returns the expansion recipe for the given instruction in this target.
     */
    const ExpansionRecipe *const getExpansionRecipeForInstr(MirInstructionOpCode opcode) override;

    /**
     * Returns the frame lowerer for this target.
     */
    MirFrameLowerer *getFrameLowerer() override;

    /**
     * Returns i32.
     */
    MirType *getMemOperandDisplacementType() override;

    /**
     * This function mimics the x64 target description.
     */
    MirType *getNearestLegalType(MirType *type) override;

    /**
     * Returns the size of the expansion recipe array.
     */
    size_t getExpansionRecipesSize() override;

    /**
     * Returns the size in bytes of a standard stack slot (4).
     */
    size_t getStackSlotSize() const override;

    /**
     * Returns the available registers for the given class.
     */
    std::pmr::vector<RegisterRef> getAvailableRegisters(RegisterRefClass refClass) override;

  private:
    MirBuilderContext *m_ctx;
    std::shared_ptr<EzTripleTestFrameLowerer> m_frameLowerer;
};

#endif // EZPACKER_EZTRIPLETESTTARGETDESCRIPTOR_H
