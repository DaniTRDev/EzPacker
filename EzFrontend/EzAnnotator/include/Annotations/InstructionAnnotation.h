#ifndef EZPACKER_INSTRUCTIONANNOTATION_H
#define EZPACKER_INSTRUCTIONANNOTATION_H

#include "EzAnnotatorCommon.h"

/**
 * This annotation holds internal information about the instruction. At the time it only saves the instruction ID, but
 * might be expanded in a future. This annotation DOESN'T have anything related to operands itself, as they are children
 * of the instruction node that contains this annotation, and each instruction is annotated on its own.
 */
class InstructionAnnotation : public IAstNodeAnnotation
{
  public:
    /**
     * Creates the annotation with the given instruction id.
     * @param id
     */
    InstructionAnnotation(size_t id);

    /**
     * Returns "Instruction".
     * @return const char*
     */
    const char *getAnnotationName() override;

    /**
     * Returns the internal ID of the instruction.
     * @return size_t
     */
    size_t getInstrId() const;

  private:
    size_t m_id; // Internal ID of the instruction.
};

#endif // EZPACKER_INSTRUCTIONANNOTATION_H
