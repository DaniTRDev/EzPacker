#ifndef EZPACKER_EZTARGET_H
#define EZPACKER_EZTARGET_H

#include "EzTargetCommon.h"

#include "TargetDesc.h"

#include "TargetAbiLowerer/TargetAbiLowererContext.h"
#include "TargetAbiLowerer/TargetAbiLowererPass.h"

#include "TargetInstructionSelector/InstructionSelectionTable.h"
#include "TargetInstructionSelector/InstructionSelectorContext.h"
#include "TargetInstructionSelector/InstructionSelectorPass.h"

#include "TargetLegalizer/AbiLegalizerPass.h"
#include "TargetLegalizer/LegalizerActionList.h"
#include "TargetLegalizer/LegalizerContext.h"
#include "TargetLegalizer/LegalizerHandlerList.h"
#include "TargetLegalizer/TypeLegalizerPass.h"

#include "TargetRegisterAllocator/RegisterAllocatorContext.h"
#include "TargetRegisterAllocator/RegisterAllocatorPass.h"

#include "TargetStackFrameLowerer/StackFrameLowerer.h"
#include "TargetStackFrameLowerer/StackFrameLowererContext.h"

#include "TargetLegalizer/StandardLegalizers/ExpandTypeLegalizer.h"
#include "TargetLegalizer/StandardLegalizers/PromoteTypeLegalizer.h"

#endif // EZPACKER_EZTARGET_H
