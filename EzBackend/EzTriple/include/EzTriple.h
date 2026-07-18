#ifndef EZPACKER_EZTRIPLE_H
#define EZPACKER_EZTRIPLE_H

#include "EzTripleCommon.h"

#include "AbiLowerer/ReturnAbiLowererPass.h"

#include "DefaultLegalizerActions/ExpandScalarAction.h"
#include "DefaultLegalizerActions/LegalizeCallAction.h"
#include "DefaultLegalizerActions/LegalizeReturnAction.h"
#include "DefaultLegalizerActions/PromoteScalarAction.h"

#include "Descriptors/ABIDesc.h"
#include "Descriptors/TargetDesc.h"

#include "ExpansionRecipe/ExpansionRecipe.h"

#include "Legalizer/LegalizeAction.h"
#include "Legalizer/MirLegalizer.h"
#include "Legalizer/MirBlockLegalizerPass.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"

#endif // EZPACKER_EZTRIPLE_H
