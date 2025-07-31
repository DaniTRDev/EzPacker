#ifndef EZPACKER_MEMORYREFANNOTATION_H
#define EZPACKER_MEMORYREFANNOTATION_H

#include "EzAnnotatorCommon.h"
#include "ConstantAnnotation.h"
#include "TypeAbleAnnotation.h"

/**
 * Types a memory reference can be. This is useful to guide the backend, for ex:a reference is of type base displ but
 * has displ = 0, backend can decide whether to emit a Base or BaseDispl reference.
 */
enum class MemoryReferenceType : uint8_t
{
    Invalid = 0,
    Base,                // (%base)
    BaseDispl,           // (%base, displacement)
    BaseIndexScaleDispl, // (%base, %index, scale, displacement)
    Direct,              // (address)
    IndexScale           // (%index, scale)
};

/**
 * This annotation saves information about a memory reference:
 *  - The type of the memory reference (MemoryReferenceType)
 *  - The type of the memory being referenced (TypeAbleAnnotation).
 *  - Reference parameters: base, index, scale and displacement.
 *
 *  Unlike instructions, a memory node WON'T have any children, and everything will be saved in its annotation. This
 *  is a design decision so the AST is flattener.
 *
 *  IMPORTANT: In the case of Direct references, displacement holds the address!
 */
class MemoryRefAnnotation : public TypeAbleAnnotation
{
  public:
    /**
     * Creates the reference annotation with the given displacement, baseId, indexId and scaling factor.
     * @param refType
     * @param baseId
     * @param indexId
     * @param typeId
     * @param displ
     * @param scale
     */
    MemoryRefAnnotation(MemoryReferenceType refType,
                        size_t baseId,
                        size_t indexId,
                        size_t typeId,
                        const std::shared_ptr<ConstantAnnotation> &displ,
                        const std::shared_ptr<ConstantAnnotation> &scale);

    /**
     * Returns the name of the annotation "getAnnotationName".
     * @return const char*
     */
    const char *getAnnotationName() override;

    /**
     * Returns the type of this memory reference. This is the type that was parsed from the input code.
     * @return MemoryReferenceType
     */
    MemoryReferenceType getRefType() const;

    /**
     * Returns the ID of the virtual variable that acts as base.
     * @return size_t
     */
    size_t getBaseId() const;

    /**
     * Returns the ID of the virtual variable that acts as index.
     * @return size_t
     */
    size_t getIndexId() const;

    /**
     * Returns the displacement value.
     * @return const std::shared_ptr<ConstantAnnotation> &
     */
    const std::shared_ptr<ConstantAnnotation> &getDispl() const;

    /**
     * Returns the scaling factor
     * @return const std::shared_ptr<ConstantAnnotation> &
     */
    const std::shared_ptr<ConstantAnnotation> &getScale() const;

  private:
    MemoryReferenceType m_type;
    size_t m_baseId;
    size_t m_indexId;
    std::shared_ptr<ConstantAnnotation> m_displ;
    std::shared_ptr<ConstantAnnotation> m_scale;
};

#endif // EZPACKER_MEMORYREFANNOTATION_H
