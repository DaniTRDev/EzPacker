#ifndef EZPACKER_EZTRIPLE_H
#define EZPACKER_EZTRIPLE_H

#include "EzTripleCommon.h"

#include "AbiLowerer/AbiLowerer.h"
#include "AbiLowerer/FunctionAbiLowererPass.h"

#include "Legalizer/Actions/ExpandScalarAction.h"
#include "Legalizer/Actions/LegalizeCallAction.h"
#include "Legalizer/Actions/LegalizeReturnAction.h"
#include "Legalizer/Actions/PromoteScalarAction.h"
#include "Legalizer/Expand/ExpansionRuleBuilder.h"
#include "Legalizer/Expand/MirExpansionRuleRegistry.h"
#include "Legalizer/LegalizeRuleBuilder.h"
#include "Legalizer/MirLegalizer.h"
#include "Legalizer/MirBlockLegalizerPass.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"

#include "Descriptors/TargetDesc.h"

#include "FrameLowerer/MirFrameLowerer.h"
#include "FrameLowerer/MirFrameLowererPass.h"

#include "InstructionSelector/Actions/ManualSelectAction.h"
#include "InstructionSelector/Actions/SelectRegisterClassAction.h"
#include "InstructionSelector/Actions/SequenceAction.h"
#include "InstructionSelector/InstructionSelectionPredicates.h"
#include "InstructionSelector/InstructionSelectionRuleBuilder.h"
#include "InstructionSelector/MirInstructionSelector.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"

#include "RegisterAllocator/MirRegisterAllocator.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"

#endif // EZPACKER_EZTRIPLE_H
