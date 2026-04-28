#ifndef EZPACKER_EZASTLOWERER_H
#define EZPACKER_EZASTLOWERER_H

#include "EzAstLowererCommon.h"

// ── AST-to-MIR lowerers (public lowering entry points and helpers) ──────────
#include "AstLowererVisitor.h"
#include "AstLoweringContext.h"
#include "Lowerers/BreakLowerer.h"
#include "Lowerers/CodeScopeLowerer.h"
#include "Lowerers/ConditionLowerer.h"
#include "Lowerers/ContinueLowerer.h"
#include "Lowerers/ForLowerer.h"
#include "GenericAstLowerer.h"
#include "Lowerers/IfLowerer.h"
#include "Lowerers/ImmediateLowerer.h"
#include "Lowerers/InstructionLowerer.h"
#include "Lowerers/LabelLowerer.h"
#include "Lowerers/ModuleLowerer.h"
#include "Lowerers/SwitchLowerer.h"
#include "Lowerers/TypeLowerer.h"
#include "Lowerers/VariableLowerer.h"
#include "Lowerers/WhileLowerer.h"

#endif // EZPACKER_EZASTLOWERER_H
