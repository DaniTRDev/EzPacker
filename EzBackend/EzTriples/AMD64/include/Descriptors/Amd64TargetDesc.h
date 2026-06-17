#ifndef EZPACKER_AMD64TARGETDESC_H
#define EZPACKER_AMD64TARGETDESC_H

#include "Amd64.h"

class Amd64TargetDesc : public TargetDesc
{
  public:
    /**
     * Creates an Amd64 targed description linked to the given mir builder context.
     * @param ctx
     */
    Amd64TargetDesc(MirBuilderContext *ctx);

    /**
     * Returns 'Amd64'.
     * @return
     */
    const char *getName() const override;

    /**
     * Returns the nearest compatible type for the given type. If the type is already legal, it is returned as-is. If no
     * type can be used, nullptr will be returned.
     *
     * For integers:
     *  - <= 8 bits, i8 is returned.
     *  - <= 16 bits && > 8 bits, i16 is returned.
     *  - <= 32 bits && > 16 bits, i32 is returned.
     *  - <= 64 bits && > 32 bits, i64 is returned.
     *  - > 64, an error is returned.
     *
     *  For floating-point values:
     *  - <= 32 bits, f32 is returned.
     *  - <= 64 bits, f64 is returned.
     * @param type
     * @return
     */
    MirType *getNearestLegalType(MirType *type) override;

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_AMD64TARGETDESC_H
