#ifndef EZPACKER_EZTRIPLETESTLEGALIZER_H
#define EZPACKER_EZTRIPLETESTLEGALIZER_H

#include "Legalizer/LegalizeRuleBuilder.h"

class EzTripleTestLegalizer
{
  public:
    /**
     * Appends the legalization rules in a given created legalizer.
     */
    static void create(MirBuilderContext *ctx, MirLegalizer *legalizer);

  private:
};

#endif // EZPACKER_EZTRIPLETESTLEGALIZER_H