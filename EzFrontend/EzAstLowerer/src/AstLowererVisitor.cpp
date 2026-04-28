#include "AstLowererVisitor.h"
#include "Lowerers/CodeScopeLowerer.h"
#include "Lowerers/ConditionLowerer.h"
#include "Lowerers/ForLowerer.h"
#include "Lowerers/IfLowerer.h"
#include "Lowerers/ImmediateLowerer.h"
#include "Lowerers/InstructionLowerer.h"
#include "Lowerers/LabelLowerer.h"
#include "Lowerers/ModuleLowerer.h"
#include "Lowerers/SwitchLowerer.h"
#include "Lowerers/VariableLowerer.h"
#include "Lowerers/WhileLowerer.h"

AstLowererVisitor::AstLowererVisitor(const std::shared_ptr<AstLoweringContext> &loweringCtx) : m_loweringCtx(loweringCtx)
{
}

bool AstLowererVisitor::visit(CodeScope *scope) { return CodeScopeLowerer().lower(scope, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ConditionAstNode *cond) { return ConditionLowerer().lower(cond, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ForAstNode *_for) { return ForLowerer().lower(_for, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(IfAstNode *ifNode) { return IfLowerer().lower(ifNode, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ImmediateOperand *imm) { return ImmediateLowerer().lower(imm, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Instruction *instr) { return InstructionLowerer().lower(instr, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Label *label) { return LabelLowerer().lower(label, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Module *module) { return ModuleLowerer().lower(module, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(ModuleHeader *header) { return ModuleHeaderLowerer().lower(header, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(SwitchAstNode *_switch) { return SwitchLowerer().lower(_switch, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(Variable *var) { return VariableLowerer().lower(var, m_loweringCtx.get()); }

bool AstLowererVisitor::visit(WhileAstNode *whileNode) { return WhileLowerer().lower(whileNode, m_loweringCtx.get()); }
